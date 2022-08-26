#include "render_target.hpp"

#include "vk_check.hpp"

#include <array>

namespace vulkan
{
  void render_target_init(context_t context, allocator_t allocator, RenderTarget& render_target)
  {
    init_swapchain(context, render_target.swapchain);

    // Render pass
    init_render_pass_simple(context, RenderPassCreateInfoSimple{
        .color_format = render_target.swapchain.surface_format.format,
        .depth_format = VK_FORMAT_D32_SFLOAT,
    }, render_target.render_pass);

    // Depth attachment
    render_target.depth_image = image_create(context, allocator,
      ImageType::DEPTH_ATTACHMENT,
      VK_FORMAT_D32_SFLOAT,
      render_target.swapchain.extent.width,
      render_target.swapchain.extent.height,
      1
    );
    render_target.depth_image_view = image_view_create(context,
      ImageViewType::DEPTH,
      VK_FORMAT_D32_SFLOAT,
      1,
      render_target.depth_image
    );

    // Framebuffers
    render_target.framebuffers = new Framebuffer[render_target.swapchain.image_count];
    for(uint32_t i=0; i<render_target.swapchain.image_count; ++i)
    {
      const image_view_t image_views[] = {
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
      frame_init(context, render_target.frames[i]);

    render_target.frame_index = 0;
  }

  void render_target_deinit(context_t context, allocator_t allocator, RenderTarget& render_target)
  {
    (void)allocator;

    for(uint32_t i=0; i<render_target.image_count; ++i)
      deinit_framebuffer(context, render_target.framebuffers[i]);

    delete[] render_target.framebuffers;

    put(render_target.depth_image);
    put(render_target.depth_image_view);

    for(size_t i=0; i<MAX_FRAME_IN_FLIGHT; ++i)
      frame_deinit(context, render_target.frames[i]);

    deinit_render_pass(context, render_target.render_pass);
    deinit_swapchain(context, render_target.swapchain);
  }

  bool render_target_begin_frame(context_t context, RenderTarget& render_target, Frame& frame)
  {
    // Acquire frame resource
    frame = render_target.frames[render_target.frame_index];
    render_target.frame_index = (render_target.frame_index + 1) % MAX_FRAME_IN_FLIGHT;

    auto result = swapchain_next_image_index(context, render_target.swapchain, frame.image_available_semaphore, render_target.image_index);
    if(result != SwapchainResult::SUCCESS)
      return false;

    // Wait and begin commannd buffer recording
    command_buffer_wait(frame.command_buffer);
    command_buffer_reset(frame.command_buffer);
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

    VkCommandBuffer handle = command_buffer_get_handle(frame.command_buffer);
    vkCmdBeginRenderPass(handle, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
    return true;
  }

  bool render_target_end_frame(context_t context, RenderTarget& render_target, const Frame& frame)
  {
    VkCommandBuffer handle = command_buffer_get_handle(frame.command_buffer);
    vkCmdEndRenderPass(handle);

    // End command buffer recording and submit
    command_buffer_end(frame.command_buffer);
    command_buffer_submit(frame.command_buffer, frame.image_available_semaphore, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, frame.render_finished_semaphore);

    return swapchain_present_image_index(context, render_target.swapchain, frame.render_finished_semaphore, render_target.image_index) == SwapchainResult::SUCCESS;
  }
}
