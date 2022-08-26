#include "material.hpp"

#include "vk_check.hpp"

#include "stb_image.h"
#include "assert.h"

#include <algorithm>
#include <bit>

namespace vulkan
{
  struct MaterialLayout
  {
    Ref ref;

    context_t context;

    const MaterialLayoutDescription *description;

    VkDescriptorSetLayout descriptor_set_layout;
  };
  REF_DEFINE(MaterialLayout, material_layout_t, ref);

  static void material_layout_free(ref_t ref)
  {
    material_layout_t material_layout = container_of(ref, MaterialLayout, ref);

    put(material_layout->context);
    VkDevice device = context_get_device_handle(material_layout->context);
    vkDestroyDescriptorSetLayout(device, material_layout->descriptor_set_layout, nullptr);

    delete material_layout;
  }

  material_layout_t material_layout_compile(context_t context, const MaterialLayoutDescription *material_layout_description)
  {
    material_layout_t material_layout = new MaterialLayout;
    material_layout->ref.count = 1;
    material_layout->ref.free  = material_layout_free;

    get(context);
    material_layout->context = context;

    material_layout->description = material_layout_description;

    VkDescriptorSetLayoutBinding descriptor_set_layout_binding = {};
    descriptor_set_layout_binding.binding            = 0;
    descriptor_set_layout_binding.descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptor_set_layout_binding.descriptorCount    = 1;
    descriptor_set_layout_binding.stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT;
    descriptor_set_layout_binding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info = {};
    descriptor_set_layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptor_set_layout_create_info.pBindings    = &descriptor_set_layout_binding;
    descriptor_set_layout_create_info.bindingCount = 1;

    VkDevice device = context_get_device_handle(material_layout->context);
    VK_CHECK(vkCreateDescriptorSetLayout(device, &descriptor_set_layout_create_info, nullptr, &material_layout->descriptor_set_layout));

    return material_layout;
  }

  static constexpr MaterialLayoutDescription MATERIAL_LAYOUT_DESCRIPTION = {
  };

  material_layout_t material_layout_create_default(context_t context)
  {
    return material_layout_compile(context, &MATERIAL_LAYOUT_DESCRIPTION);
  }

  VkDescriptorSetLayout material_layout_get_descriptor_set_layout(material_layout_t material_layout)
  {
    return material_layout->descriptor_set_layout;
  }

  struct Material
  {
    Ref ref;

    context_t context;

    material_layout_t layout;

    image_t      image;
    image_view_t image_view;
    sampler_t    sampler;

    VkDescriptorPool descriptor_pool; // TODO: Figure out a way to shared descriptor pool across multiple material
    VkDescriptorSet  descriptor_set;
  };
  REF_DEFINE(Material, material_t, ref);

  static void material_free(ref_t ref)
  {
    material_t material = container_of(ref, Material, ref);

    VkDevice device = context_get_device_handle(material->context);
    vkDestroyDescriptorPool(device, material->descriptor_pool, nullptr);

    put(material->context);
    put(material->layout);
    put(material->image);
    put(material->image_view);
    put(material->sampler);

    delete material;
  }

  material_t material_create(context_t context, material_layout_t material_layout, image_t image, image_view_t image_view, sampler_t sampler)
  {
    material_t material = new Material;
    material->ref.count = 1;
    material->ref.free  = material_free;

    get(context);
    material->context = context;

    get(material_layout);
    material->layout = material_layout;

    get(image);
    material->image = image;

    get(image_view);
    material->image_view = image_view;

    get(sampler);
    material->sampler = sampler;

    VkDevice device = context_get_device_handle(material->context);

    // Create the descriptor pool
    VkDescriptorPoolSize descriptor_pool_size = {};
    descriptor_pool_size.type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptor_pool_size.descriptorCount = 1;

    VkDescriptorPoolCreateInfo descriptor_pool_create_info = {};
    descriptor_pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptor_pool_create_info.poolSizeCount = 1;
    descriptor_pool_create_info.pPoolSizes    = &descriptor_pool_size;
    descriptor_pool_create_info.maxSets       = 1;
    VK_CHECK(vkCreateDescriptorPool(device, &descriptor_pool_create_info, nullptr, &material->descriptor_pool));

    // Create the descriptor set
    VkDescriptorSetAllocateInfo descriptor_set_alloc_info = {};
    descriptor_set_alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptor_set_alloc_info.descriptorPool     = material->descriptor_pool;
    descriptor_set_alloc_info.descriptorSetCount = 1;
    descriptor_set_alloc_info.pSetLayouts        = &material->layout->descriptor_set_layout;
    VK_CHECK(vkAllocateDescriptorSets(device, &descriptor_set_alloc_info, &material->descriptor_set));

    // Write descriptor set
    VkDescriptorImageInfo descriptor_image_info = {};
    descriptor_image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    descriptor_image_info.imageView   = image_view_get_handle(material->image_view);
    descriptor_image_info.sampler     = sampler_get_handle(material->sampler);

    VkWriteDescriptorSet write_descriptor_set = {};
    write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_descriptor_set.dstSet          = material->descriptor_set;
    write_descriptor_set.dstBinding      = 0;
    write_descriptor_set.dstArrayElement = 0;
    write_descriptor_set.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write_descriptor_set.descriptorCount = 1;
    write_descriptor_set.pImageInfo      = &descriptor_image_info;
    vkUpdateDescriptorSets(device, 1, &write_descriptor_set, 0, nullptr);

    return material;
  }

  material_t material_load(command_buffer_t command_buffer, context_t context, allocator_t allocator, const char *file_name)
  {
    int x, y, n;
    unsigned char *data = stbi_load(file_name, &x, &y, &n, STBI_rgb_alpha);
    assert(data);

    assert(x>=0 && y>=0);
    unsigned width = x, height = y;
    size_t mip_level = std::bit_width(std::bit_floor(std::max(width, height)));

    image_t      image      = image_create(context, allocator, ImageType::TEXTURE, VK_FORMAT_R8G8B8A8_SRGB, width, height, mip_level);
    image_view_t image_view = image_view_create(context, ImageViewType::COLOR, VK_FORMAT_R8G8B8A8_SRGB, mip_level, image);
    sampler_t    sampler    = sampler_create_simple(context);

    image_write(command_buffer, image, data, width, height, width * height * 4);

    stbi_image_free(data);

    material_layout_t material_layout = material_layout_create_default(context);
    material_t        material        = material_create(context, material_layout, image, image_view, sampler);

    put(image);
    put(image_view);
    put(sampler);
    put(material_layout);

    return material;
  }

  VkDescriptorSet material_get_descriptor_set(material_t material)
  {
    return material->descriptor_set;
  }
}