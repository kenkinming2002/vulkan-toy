#pragma once

#include "context.hpp"
#include "descriptor_set_layout.hpp"

#include <vulkan/vulkan.h>

namespace vulkan
{
  struct PushConstantInfo
  {
    uint32_t offset;
    uint32_t size;
    ShaderStage stage;
  };

  struct PipelineLayoutCreateInfo
  {
    DescriptorSetLayout descriptor_set_layout;

    const PushConstantInfo *push_constants;
    uint32_t                push_constant_count;
  };

  struct PipelineLayout
  {
    VkPipelineLayout handle;
  };

  void init_pipeline_layout(const Context& context, PipelineLayoutCreateInfo create_info, PipelineLayout& pipeline_layout);
  void deinit_pipeline_layout(const Context& context, PipelineLayout& pipeline_layout);
}
