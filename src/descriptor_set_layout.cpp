#include "descriptor_set_layout.hpp"

#include "utils.hpp"
#include "vk_check.hpp"

#include <assert.h>

namespace vulkan
{
  static VkDescriptorType to_vulkan_descriptor_type(DescriptorType type)
  {
    switch(type)
    {
    case DescriptorType::UNIFORM_BUFFER: return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    case DescriptorType::SAMPLER:        return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    default: assert(false && "Unreachable");
    }
  }

  static VkShaderStageFlags to_vulkan_stage_flags(ShaderStage stage)
  {
    switch(stage)
    {
    case ShaderStage::VERTEX:   return VK_SHADER_STAGE_VERTEX_BIT;
    case ShaderStage::FRAGMENT: return VK_SHADER_STAGE_FRAGMENT_BIT;
    default: assert(false && "Unreachable");
    }
  }

  void init_descriptor_set_layout(const Context& context, DescriptorSetLayoutCreateInfo create_info, DescriptorSetLayout& descriptor_set_layout)
  {
    dynarray<VkDescriptorSetLayoutBinding> layout_bindings = create_dynarray<VkDescriptorSetLayoutBinding>(create_info.descriptor_input.descriptor_count);
    for(uint32_t i=0; i<create_info.descriptor_input.descriptor_count; ++i)
    {
      VkDescriptorSetLayoutBinding layout_binding = {};
      layout_binding.binding            = i;
      layout_binding.descriptorType     = to_vulkan_descriptor_type(create_info.descriptor_input.descriptors[i].type);
      layout_binding.descriptorCount    = 1;
      layout_binding.stageFlags         = to_vulkan_stage_flags(create_info.descriptor_input.descriptors[i].stage);
      layout_binding.pImmutableSamplers = nullptr;
      layout_bindings[i] = layout_binding;
    }

    VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info = {};
    descriptor_set_layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptor_set_layout_create_info.bindingCount = size(layout_bindings);
    descriptor_set_layout_create_info.pBindings    = data(layout_bindings);
    VK_CHECK(vkCreateDescriptorSetLayout(context.device, &descriptor_set_layout_create_info, nullptr, &descriptor_set_layout.handle));

    destroy_dynarray(layout_bindings);
  }

  void deinit_descriptor_set_layout(const Context& context, DescriptorSetLayout& descriptor_set_layout)
  {
    vkDestroyDescriptorSetLayout(context.device, descriptor_set_layout.handle, nullptr);
  }
}
