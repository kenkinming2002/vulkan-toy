#pragma once

#include "core/context.hpp"
#include "render_pass.hpp"
#include "resources/mesh.hpp"
#include "resources/material.hpp"
#include "shader.hpp"

namespace vulkan
{
  enum class ShaderStage { VERTEX, FRAGMENT };

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

    shader_t vertex_shader;
    shader_t fragment_shader;

    PushConstantInput push_constant_input;
  };

  // TODO: Use pipeline cache
  void init_pipeline2(context_t context, PipelineCreateInfo2 create_info, Pipeline2& pipeline);
  void deinit_pipeline2(context_t context, Pipeline2& pipeline);
}
