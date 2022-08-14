#pragma once

#include <stddef.h>
#include <stdint.h>

namespace vulkan
{
  struct VertexAttribute
  {
    enum class Type
    {
      FLOAT1, FLOAT2, FLOAT3, FLOAT4 // Which maniac who not use float as vertex input anyway?
    };

    size_t offset;
    Type type;
  };

  // You need multiple binding description if you use multiple buffer
  struct VertexBinding
  {
    size_t stride;

    const VertexAttribute *attributes;
    size_t                 attribute_count;
  };

  struct VertexInput
  {
    const VertexBinding *bindings;
    size_t               binding_count;
  };

  enum class DescriptorType { UNIFORM_BUFFER, SAMPLER };

  enum class ShaderStage { VERTEX, FRAGMENT };

  struct DescriptorInfo
  {
    DescriptorType type;
    ShaderStage stage;
  };

  struct DescriptorInput
  {
    const DescriptorInfo *descriptors;
    uint32_t              descriptor_count;
  };

}
