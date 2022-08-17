#include "render_context.hpp"

#include "src/image.hpp"
#include "src/image_view.hpp"
#include "vk_check.hpp"

#include <vulkan/vulkan_core.h>

namespace vulkan
{
  void render_target_init(const Context& context, Allocator& allocator, RenderTarget& render_target)
  {
    init_swapchain(context, render_target.swapchain);

    // Render pass
    init_render_pass_simple(context, RenderPassCreateInfoSimple{
        .color_format = render_target.swapchain.surface_format.format,
        .depth_format = VK_FORMAT_D32_SFLOAT,
    }, render_target.render_pass);

    // Depth attachment
    init_image(context, allocator, ImageCreateInfo{
      .type   = ImageType::DEPTH_ATTACHMENT,
      .format = VK_FORMAT_D32_SFLOAT,
      .extent = render_target.swapchain.extent,
    }, render_target.depth_image);
    init_image_view(context, ImageViewCreateInfo{
      .type   = ImageViewType::DEPTH,
      .format = VK_FORMAT_D32_SFLOAT,
      .image  = render_target.depth_image,
    }, render_target.depth_image_view);

    // Framebuffers
    render_target.framebuffers = new Framebuffer[render_target.swapchain.image_count];
    for(uint32_t i=0; i<render_target.swapchain.image_count; ++i)
    {
      const ImageView image_views[] = {
        render_target.swapchain.image_views[i],
        render_target.depth_image_view,
      };
      init_framebuffer(context, FramebufferCreateInfo{
        .render_pass      = render_target.render_pass.handle,
        .extent           = render_target.swapchain.extent,
        .image_views      = image_views,
        .image_view_count = std::size(image_views),
      }, render_target.framebuffers[i]);
    }

    render_target.image_count = render_target.swapchain.image_count;
    render_target.image_index = 0;

    // Frames
    for(size_t i=0; i<MAX_FRAME_IN_FLIGHT; ++i)
    {
      init_command_buffer(context, render_target.frames[i].command_buffer);
      init_fence(context, render_target.frames[i].fence, true);
      init_semaphore(context, render_target.frames[i].semaphore_image_available);
      init_semaphore(context, render_target.frames[i].semaphore_render_finished);
    }
    render_target.frame_index = 0;
  }

  void render_target_deinit(const Context& context, Allocator& allocator, RenderTarget& render_target)
  {
    for(uint32_t i=0; i<render_target.image_count; ++i)
      deinit_framebuffer(context, render_target.framebuffers[i]);

    delete[] render_target.framebuffers;

    deinit_image(context, allocator, render_target.depth_image);
    deinit_image_view(context, render_target.depth_image_view);

    for(size_t i=0; i<MAX_FRAME_IN_FLIGHT; ++i)
    {
      deinit_command_buffer(context, render_target.frames[i].command_buffer);
      deinit_fence(context, render_target.frames[i].fence);
      deinit_semaphore(context, render_target.frames[i].semaphore_image_available);
      deinit_semaphore(context, render_target.frames[i].semaphore_render_finished);
    }

    deinit_render_pass(context, render_target.render_pass);
    deinit_swapchain(context, render_target.swapchain);
  }

  bool render_target_begin_frame(const Context& context, RenderTarget& render_target, Frame& frame)
  {
    // Acquire frame resource
    frame = render_target.frames[render_target.frame_index];
    render_target.frame_index = (render_target.frame_index + 1) % MAX_FRAME_IN_FLIGHT;

    auto result = swapchain_next_image_index(context, render_target.swapchain, frame.semaphore_image_available, render_target.image_index);
    if(result != SwapchainResult::SUCCESS)
      return false;

    // Wait and begin commannd buffer recording
    fence_wait_and_reset(context, frame.fence);
    command_buffer_begin(frame.command_buffer);

    VkRenderPassBeginInfo render_pass_begin_info = {};
    render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_begin_info.renderPass        = render_target.render_pass.handle;
    render_pass_begin_info.framebuffer       = render_target.framebuffers[render_target.image_index].handle;
    render_pass_begin_info.renderArea.offset = {0, 0};
    render_pass_begin_info.renderArea.extent = render_target.swapchain.extent;

    // TODO: Take this as argument
    VkClearValue clear_values[2] = {};
    clear_values[0].color        = {{ 0.0f, 0.0f, 0.0f, 1.0f }};
    clear_values[1].depthStencil = { 1.0f, 0 };

    render_pass_begin_info.clearValueCount = 2;
    render_pass_begin_info.pClearValues = clear_values;

    vkCmdBeginRenderPass(frame.command_buffer.handle, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
    return true;
  }

  bool render_target_end_frame(const Context& context, RenderTarget& render_target, const Frame& frame)
  {
    vkCmdEndRenderPass(frame.command_buffer.handle);

    // End command buffer recording and submit
    command_buffer_end(frame.command_buffer);
    command_buffer_submit(context, frame.command_buffer, frame.fence,
        frame.semaphore_image_available, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        frame.semaphore_render_finished);

    return swapchain_present_image_index(context, render_target.swapchain, frame.semaphore_render_finished, render_target.image_index) == SwapchainResult::SUCCESS;
  }

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
