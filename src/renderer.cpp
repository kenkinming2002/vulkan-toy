#include "renderer.hpp"

namespace vulkan
{
  void renderer_init(const Context& context, const RenderTarget& render_target, RendererCreateInfo create_info, Renderer& renderer)
  {
    Shader vertex_shader = {};
    Shader fragment_shader = {};

    load_shader(context, create_info.vertex_shader_file_name, vertex_shader);
    load_shader(context, create_info.fragment_shader_file_name, fragment_shader);

    init_pipeline2(context, PipelineCreateInfo2{
      .render_pass         = render_target.render_pass,
      .vertex_shader       = vertex_shader,
      .fragment_shader     = fragment_shader,
      .vertex_input        = create_info.vertex_input,
      .descriptor_input    = create_info.descriptor_input,
      .push_constant_input = create_info.push_constant_input,
    }, renderer.pipeline);

    deinit_shader(context, vertex_shader);
    deinit_shader(context, fragment_shader);
  }

  void renderer_deinit(const Context& context, Renderer& renderer)
  {
    vulkan::deinit_pipeline2(context, renderer.pipeline);
  }

  void renderer_begin_render(Renderer& renderer, const Frame& frame)
  {
    vkCmdBindPipeline(frame.command_buffer.handle, VK_PIPELINE_BIND_POINT_GRAPHICS, renderer.pipeline.handle);
  }

  void renderer_end_render(Renderer& renderer, const Frame& frame)
  {
    (void)renderer;
    (void)frame;
  }
}
