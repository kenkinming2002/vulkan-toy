#pragma once

#include "core/context.hpp"
#include "input.hpp"
#include "render_pass.hpp"
#include "resources/mesh.hpp"
#include "resources/material.hpp"
#include "shader.hpp"

namespace vulkan
{
  struct Pipeline2
  {
    VkPipelineLayout pipeline_layout;
    VkPipeline       handle;
  };

  struct PipelineCreateInfo2
  {
    RenderPass render_pass;

    mesh_layout_t     mesh_layout;
    material_layout_t material_layout;

    Shader vertex_shader;
    Shader fragment_shader;

    PushConstantInput push_constant_input;
  };

  // TODO: Use pipeline cache
  void init_pipeline2(context_t context, PipelineCreateInfo2 create_info, Pipeline2& pipeline);
  void deinit_pipeline2(context_t context, Pipeline2& pipeline);
}
