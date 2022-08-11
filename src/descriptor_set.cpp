#include "descriptor_set.hpp"

#include "vk_check.hpp"

#include <assert.h>
#include <vulkan/vulkan_core.h>

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

  void init_descriptor_pool(const Context& context, DescriptorPoolCreateInfo create_info, DescriptorPool& descriptor_pool)
  {
    VkDescriptorPoolSize *pool_sizes = new VkDescriptorPoolSize[create_info.descriptor_count];
    for(uint32_t i=0; i<create_info.descriptor_count; ++i)
    {
      pool_sizes[i].type            = to_vulkan_descriptor_type(create_info.descriptors[i].type);
      pool_sizes[i].descriptorCount = create_info.count;
    }

    VkDescriptorPoolCreateInfo pool_create_info = {};
    pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_create_info.poolSizeCount = create_info.descriptor_count;
    pool_create_info.pPoolSizes    = pool_sizes;
    pool_create_info.maxSets       = create_info.count;
    VK_CHECK(vkCreateDescriptorPool(context.device, &pool_create_info, nullptr, &descriptor_pool.handle));

    delete[] pool_sizes;
  }

  void deinit_descriptor_pool(const Context& context, DescriptorPool& descriptor_pool)
  {
    vkDestroyDescriptorPool(context.device, descriptor_pool.handle, nullptr);
  }

  void allocate_descriptor_set(const Context& context, DescriptorPool descriptor_pool, DescriptorSetLayout descriptor_set_layout, DescriptorSet& descriptor_set)
  {
    VkDescriptorSetAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorPool     = descriptor_pool.handle;
    alloc_info.descriptorSetCount = 1;
    alloc_info.pSetLayouts        = &descriptor_set_layout.handle;
    VK_CHECK(vkAllocateDescriptorSets(context.device, &alloc_info, &descriptor_set.handle));
  }
}
