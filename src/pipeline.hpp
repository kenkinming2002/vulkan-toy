#pragma once

#include "context.hpp"
#include "render_pass.hpp"
#include "shader.hpp"
#include "pipeline_layout.hpp"

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

  struct PipelineCreateInfo
  {
    RenderPass     render_pass;

    Shader vertex_shader;
    Shader fragment_shader;

    VertexInput    vertex_input;
    PipelineLayout pipeline_layout;
  };

  struct Pipeline
  {
    VkPipeline handle;
  };

  void init_pipeline(const Context& context, PipelineCreateInfo create_info, Pipeline& pipeline);
  void deinit_pipeline(const Context& context, Pipeline& pipeline);
}
