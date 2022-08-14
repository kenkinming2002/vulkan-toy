#pragma once

#include "context.hpp"
#include "input.hpp"

namespace vulkan
{
  struct DescriptorSetLayoutCreateInfo
  {
    DescriptorInput descriptor_input;
  };

  struct DescriptorSetLayout
  {
    VkDescriptorSetLayout handle;
  };

  void init_descriptor_set_layout(const Context& context, DescriptorSetLayoutCreateInfo create_info, DescriptorSetLayout& descriptor_set_layout);
  void deinit_descriptor_set_layout(const Context& context, DescriptorSetLayout& descriptor_set_layout);
}
