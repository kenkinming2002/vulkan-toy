#pragma once

#include "context.hpp"

#include <vulkan/vulkan.h>

namespace vulkan
{
  enum class PipelineStage { VERTEX, FRAGMENT };
  enum class DescriptorType { UNIFORM_BUFFER, SAMPLER };

  struct DescriptorInfo
  {
    DescriptorType type;
    PipelineStage stage;
  };

  struct PushConstantInfo
  {
    uint32_t offset;
    uint32_t size;
    PipelineStage stage;
  };

  struct PipelineLayoutCreateInfo
  {
    const DescriptorInfo *descriptors;
    uint32_t              descriptor_count;

    const PushConstantInfo *push_constants;
    uint32_t                push_constant_count;
  };

  struct PipelineLayout
  {
    VkDescriptorSetLayout descriptor_set_layout;
    VkPipelineLayout      pipeline_layout;
  };

  void init_pipeline_layout(const Context& context, PipelineLayoutCreateInfo create_info, PipelineLayout& pipeline_layout);
  void deinit_pipeline_layout(const Context& context, PipelineLayout& pipeline_layout);
}
