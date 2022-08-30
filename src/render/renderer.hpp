#pragma once

#include "camera.hpp"
#include "core/context.hpp"
#include "render_target.hpp"
#include "resources/material.hpp"
#include "resources/mesh.hpp"
#include "shader.hpp"

namespace vulkan
{
  typedef struct Renderer *renderer_t;

  renderer_t renderer_create(context_t context, const RenderTarget& render_target,
    mesh_layout_t mesh_layout,
    material_layout_t material_layout,
    const char *vertex_shader_file_name,
    const char *fragment_shader_file_name);

  void renderer_destroy(renderer_t renderer);

  void renderer_begin_render(renderer_t renderer, const Frame *frame);
  void renderer_end_render(renderer_t renderer);

  void renderer_set_viewport_and_scissor(renderer_t renderer, VkExtent2D extent);
  void renderer_use_camera(renderer_t renderer, const Camera& camera);
  void renderer_draw(renderer_t renderer, material_t material, mesh_t mesh);
}
