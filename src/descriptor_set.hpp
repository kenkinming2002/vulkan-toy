#pragma once

#include "context.hpp"
#include "descriptor_info.hpp"

namespace vulkan
{
  struct DescriptorPoolCreateInfo
  {
    const DescriptorInfo *descriptors;
    uint32_t              descriptor_count;

    uint32_t count;
  };

  struct DescriptorPool
  {
    VkDescriptorPool handle;
  };

  void init_descriptor_pool(const Context& context, DescriptorPoolCreateInfo create_info, DescriptorPool& descriptor_pool);
  void deinit_descriptor_pool(const Context& context, DescriptorPool& descriptor_pool);
}
