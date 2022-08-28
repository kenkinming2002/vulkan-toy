#pragma once

#include "core/context.hpp"
#include "pipeline.hpp"
#include "render_target.hpp"
#include "resources/mesh.hpp"

namespace vulkan
{
  struct RendererCreateInfo
  {
    mesh_layout_t     mesh_layout;
    material_layout_t material_layout;

    const char* vertex_shader_file_name;
    const char* fragment_shader_file_name;

    PushConstantInput push_constant_input;
  };

  struct Renderer
  {
    Pipeline2  pipeline;
    const Frame *current_frame;
  };

  void renderer_init(context_t context, const RenderTarget& render_target, RendererCreateInfo create_info, Renderer& renderer);
  void renderer_deinit(context_t context, Renderer& renderer);

  void renderer_begin_render(Renderer& renderer, const Frame *frame);
  void renderer_end_render(Renderer& renderer);

  void renderer_push_constant(Renderer& renderer, ShaderStage shader_stage, void *data, size_t offset, size_t size);
  void renderer_set_viewport_and_scissor(Renderer& renderer, VkExtent2D extent);

  void renderer_draw(Renderer& renderer, material_t material, mesh_t mesh);
}
