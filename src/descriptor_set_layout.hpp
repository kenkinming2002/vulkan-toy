#pragma once

#include "context.hpp"
#include "descriptor_info.hpp"

namespace vulkan
{
  struct DescriptorSetLayoutCreateInfo
  {
    const DescriptorInfo *descriptors;
    uint32_t              descriptor_count;
  };

  struct DescriptorSetLayout
  {
    VkDescriptorSetLayout handle;
  };

  void init_descriptor_set_layout(const Context& context, DescriptorSetLayoutCreateInfo create_info, DescriptorSetLayout& descriptor_set_layout);
  void deinit_descriptor_set_layout(const Context& context, DescriptorSetLayout& descriptor_set_layout);
}