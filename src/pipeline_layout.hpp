#pragma once

#include "context.hpp"
#include "descriptor_set_layout.hpp"
#include "input.hpp"

#include <vulkan/vulkan.h>

namespace vulkan
{
  struct PipelineLayoutCreateInfo
  {
    DescriptorSetLayout descriptor_set_layout;
    PushConstantInput   push_constant_input;
  };

  struct PipelineLayout
  {
    VkPipelineLayout handle;
  };

  void init_pipeline_layout(const Context& context, PipelineLayoutCreateInfo create_info, PipelineLayout& pipeline_layout);
  void deinit_pipeline_layout(const Context& context, PipelineLayout& pipeline_layout);
}
