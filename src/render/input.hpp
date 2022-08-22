#pragma once

#include <stddef.h>
#include <stdint.h>

namespace vulkan
{
  enum class DescriptorType { UNIFORM_BUFFER, SAMPLER };

  enum class ShaderStage { VERTEX, FRAGMENT };

  struct DescriptorBinding
  {
    DescriptorType type;
    ShaderStage stage;
  };

  struct DescriptorInput
  {
    const DescriptorBinding *bindings;
    uint32_t                 binding_count;
  };

  struct PushConstantRange
  {
    uint32_t offset;
    uint32_t size;
    ShaderStage stage;
  };

  struct PushConstantInput
  {
    const PushConstantRange *ranges;
    uint32_t                 range_count;
  };
}
