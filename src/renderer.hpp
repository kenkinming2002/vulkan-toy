#pragma once

#include "core/context.hpp"
#include "input.hpp"
#include "pipeline.hpp"
#include "descriptor_set.hpp"
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
    const Frame *current_frame;
  };

  void renderer_init(context_t context, const RenderTarget& render_target, RendererCreateInfo create_info, Renderer& renderer);
  void renderer_deinit(context_t context, Renderer& renderer);

  void renderer_begin_render(Renderer& renderer, const Frame& frame);
  void renderer_end_render(Renderer& renderer);

  void renderer_push_constant(Renderer& renderer, ShaderStage shader_stage, void *data, size_t offset, size_t size);
  void renderer_bind_descriptor_set(Renderer& renderer, DescriptorSet descriptor_set);
  void renderer_set_viewport_and_scissor(Renderer& renderer, VkExtent2D extent);
}
