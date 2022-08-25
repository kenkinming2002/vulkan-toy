#include "renderer.hpp"

namespace vulkan
{
  static VkShaderStageFlags to_vulkan_stage_flags(ShaderStage stage)
  {
    switch(stage)
    {
    case ShaderStage::VERTEX:   return VK_SHADER_STAGE_VERTEX_BIT;
    case ShaderStage::FRAGMENT: return VK_SHADER_STAGE_FRAGMENT_BIT;
    default: assert(false && "Unreachable");
    }
  }

  void renderer_init(context_t context, const RenderTarget& render_target, RendererCreateInfo create_info, Renderer& renderer)
  {
    Shader vertex_shader = {};
    Shader fragment_shader = {};

    load_shader(context, create_info.vertex_shader_file_name, vertex_shader);
    load_shader(context, create_info.fragment_shader_file_name, fragment_shader);

    init_pipeline2(context, PipelineCreateInfo2{
      .render_pass         = render_target.render_pass,
      .mesh_layout         = create_info.mesh_layout,
      .material_layout     = create_info.material_layout,
      .vertex_shader       = vertex_shader,
      .fragment_shader     = fragment_shader,
      .push_constant_input = create_info.push_constant_input,
    }, renderer.pipeline);

    deinit_shader(context, vertex_shader);
    deinit_shader(context, fragment_shader);
  }

  void renderer_deinit(context_t context, Renderer& renderer)
  {
    vulkan::deinit_pipeline2(context, renderer.pipeline);
  }

  void renderer_begin_render(Renderer& renderer, const Frame& frame)
  {
    VkCommandBuffer handle = command_buffer_get_handle(frame.command_buffer);
    vkCmdBindPipeline(handle, VK_PIPELINE_BIND_POINT_GRAPHICS, renderer.pipeline.handle);
    renderer.current_frame = &frame;
  }

  void renderer_end_render(Renderer& renderer)
  {
    renderer.current_frame = nullptr;
  }

  void renderer_push_constant(Renderer& renderer, ShaderStage shader_stage, void *data, size_t offset, size_t size)
  {
    assert(renderer.current_frame);

    VkCommandBuffer handle = command_buffer_get_handle(renderer.current_frame->command_buffer);
    vkCmdPushConstants(handle, renderer.pipeline.pipeline_layout, to_vulkan_stage_flags(shader_stage), offset, size, data);
  }

  void renderer_bind_descriptor_set(Renderer& renderer, DescriptorSet descriptor_set)
  {
    assert(renderer.current_frame);

    VkCommandBuffer handle = command_buffer_get_handle(renderer.current_frame->command_buffer);
    vkCmdBindDescriptorSets(handle, VK_PIPELINE_BIND_POINT_GRAPHICS, renderer.pipeline.pipeline_layout, 0, 1, &descriptor_set.handle, 0, nullptr);
  }

  void renderer_set_viewport_and_scissor(Renderer& renderer, VkExtent2D extent)
  {
    assert(renderer.current_frame);

    VkCommandBuffer handle = command_buffer_get_handle(renderer.current_frame->command_buffer);

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width  = extent.width;
    viewport.height = extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(handle, 0, 1, &viewport);

    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = extent;
    vkCmdSetScissor(handle, 0, 1, &scissor);
  }

  void renderer_use_material(Renderer& renderer, material_t material)
  {
    assert(renderer.current_frame);

    VkCommandBuffer handle = command_buffer_get_handle(renderer.current_frame->command_buffer);

    VkDescriptorSet descriptor_set = material_get_descriptor_set(material);
    vkCmdBindDescriptorSets(handle, VK_PIPELINE_BIND_POINT_GRAPHICS, renderer.pipeline.pipeline_layout, 0, 1, &descriptor_set, 0, nullptr);
  }
}
