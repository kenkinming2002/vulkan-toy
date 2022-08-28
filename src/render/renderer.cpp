#include "renderer.hpp"

namespace vulkan
{
  struct Renderer
  {
    context_t context;

    Pipeline2  pipeline;
    const Frame *current_frame;
  };

  static VkShaderStageFlags to_vulkan_stage_flags(ShaderStage stage)
  {
    switch(stage)
    {
    case ShaderStage::VERTEX:   return VK_SHADER_STAGE_VERTEX_BIT;
    case ShaderStage::FRAGMENT: return VK_SHADER_STAGE_FRAGMENT_BIT;
    default: assert(false && "Unreachable");
    }
  }

  renderer_t renderer_create(context_t context, const RenderTarget& render_target,
    mesh_layout_t mesh_layout,
    material_layout_t material_layout,
    const char *vertex_shader_file_name,
    const char *fragment_shader_file_name,
    PushConstantInput push_constant_input)
  {
    renderer_t renderer = new Renderer {};

    get(context);
    renderer->context = context;

    shader_t vertex_shader   = shader_load(context, vertex_shader_file_name);
    shader_t fragment_shader = shader_load(context, fragment_shader_file_name);

    init_pipeline2(context, PipelineCreateInfo2{
      .render_pass         = render_target.render_pass,
      .mesh_layout         = mesh_layout,
      .material_layout     = material_layout,
      .vertex_shader       = vertex_shader,
      .fragment_shader     = fragment_shader,
      .push_constant_input = push_constant_input,
    }, renderer->pipeline);

    put(vertex_shader);
    put(fragment_shader);

    return renderer;
  }


  void renderer_destroy(renderer_t renderer)
  {
    deinit_pipeline2(renderer->context, renderer->pipeline);
    put(renderer->context);
    delete renderer;
  }

  void renderer_begin_render(renderer_t renderer, const Frame *frame)
  {
    renderer->current_frame = frame;

    VkCommandBuffer handle = command_buffer_get_handle(renderer->current_frame->command_buffer);
    vkCmdBindPipeline(handle, VK_PIPELINE_BIND_POINT_GRAPHICS, renderer->pipeline.handle);
  }

  void renderer_end_render(renderer_t renderer)
  {
    renderer->current_frame = nullptr;
  }

  void renderer_push_constant(renderer_t renderer, ShaderStage shader_stage, void *data, size_t offset, size_t size)
  {
    VkCommandBuffer handle = command_buffer_get_handle(renderer->current_frame->command_buffer);
    vkCmdPushConstants(handle, renderer->pipeline.pipeline_layout, to_vulkan_stage_flags(shader_stage), offset, size, data);
  }

  void renderer_set_viewport_and_scissor(renderer_t renderer, VkExtent2D extent)
  {
    VkCommandBuffer handle = command_buffer_get_handle(renderer->current_frame->command_buffer);

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

  void renderer_draw(renderer_t renderer, material_t material, mesh_t mesh)
  {
    VkCommandBuffer handle = command_buffer_get_handle(renderer->current_frame->command_buffer);

    VkDescriptorSet descriptor_set = material_get_descriptor_set(material);
    vkCmdBindDescriptorSets(handle, VK_PIPELINE_BIND_POINT_GRAPHICS, renderer->pipeline.pipeline_layout, 0, 1, &descriptor_set, 0, nullptr);

    size_t vertex_buffer_count;
    buffer_t *vertex_buffers = mesh_get_vertex_buffers(mesh, vertex_buffer_count);
    for(size_t i=0; i<vertex_buffer_count; ++i)
    {
      VkDeviceSize offsets[] = {0};

      buffer_t vertex_buffer        = vertex_buffers[i];
      VkBuffer vertex_buffer_handle = buffer_get_handle(vertex_buffer);
      vkCmdBindVertexBuffers(handle, i, 1, &vertex_buffer_handle, offsets);
      command_buffer_use(renderer->current_frame->command_buffer, as_ref(vertex_buffer));
    }

    buffer_t index_buffer = mesh_get_index_buffer(mesh);
    VkBuffer index_buffer_handle = buffer_get_handle(index_buffer);
    vkCmdBindIndexBuffer(handle, index_buffer_handle, 0, VK_INDEX_TYPE_UINT32);
    command_buffer_use(renderer->current_frame->command_buffer, as_ref(index_buffer));

    // Index buffers
    size_t index_count = mesh_get_index_count(mesh);
    vkCmdDrawIndexed(handle, index_count, 1, 0, 0, 0);
  }
}
