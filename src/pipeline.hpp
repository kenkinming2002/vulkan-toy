#pragma once

#include "context.hpp"
#include "input.hpp"
#include "pipeline_layout.hpp"
#include "render_pass.hpp"
#include "shader.hpp"

namespace vulkan
{
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
