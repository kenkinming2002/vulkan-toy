#pragma once

#include "context.hpp"
#include "input.hpp"
#include "pipeline.hpp"
#include "render_target.hpp"

namespace vulkan
{
  struct RendererCreateInfo
  {
    const char* vertex_shader_file_name;
    const char* fragment_shader_file_name;

    VertexInput       vertex_input;
    DescriptorInput   descriptor_input;
    PushConstantInput push_constant_input;
  };

  struct Renderer
  {
    Pipeline2  pipeline;
  };

  void renderer_init(const Context& context, const RenderTarget& render_target, RendererCreateInfo create_info, Renderer& renderer);
  void renderer_deinit(const Context& context, Renderer& renderer);

  void renderer_begin_render(Renderer& renderer, const Frame& frame);
  void renderer_end_render(Renderer& renderer, const Frame& frame);
}
