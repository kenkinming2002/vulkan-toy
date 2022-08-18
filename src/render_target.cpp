#include "render_target.hpp"

#include "vk_check.hpp"

#include <array>

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

    // Command buffers
    VkCommandBuffer command_buffers[MAX_FRAME_IN_FLIGHT];
    VkCommandBufferAllocateInfo command_buffer_allocate_info = {};
    command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_allocate_info.commandPool        = context.command_pool;
    command_buffer_allocate_info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    command_buffer_allocate_info.commandBufferCount = MAX_FRAME_IN_FLIGHT;
    VK_CHECK(vkAllocateCommandBuffers(context.device, &command_buffer_allocate_info, command_buffers));

    for(size_t i=0; i<MAX_FRAME_IN_FLIGHT; ++i)
      render_target.frames[i].command_buffer = command_buffers[i];

    // Sync objects
    VkFenceCreateInfo fence_create_info = {};
    fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VkSemaphoreCreateInfo semaphore_create_info = {};
    semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    for(size_t i=0; i<MAX_FRAME_IN_FLIGHT; ++i)
    {
      VK_CHECK(vkCreateFence(context.device, &fence_create_info, nullptr, &render_target.frames[i].fence));
      VK_CHECK(vkCreateSemaphore(context.device, &semaphore_create_info, nullptr, &render_target.frames[i].image_available_semaphore));
      VK_CHECK(vkCreateSemaphore(context.device, &semaphore_create_info, nullptr, &render_target.frames[i].render_finished_semaphore));
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

    // Command buffer
    VkCommandBuffer command_buffers[MAX_FRAME_IN_FLIGHT];
    for(size_t i=0; i<MAX_FRAME_IN_FLIGHT; ++i)
      command_buffers[i] = render_target.frames[i].command_buffer;

    vkFreeCommandBuffers(context.device, context.command_pool, MAX_FRAME_IN_FLIGHT, command_buffers);

    // Sync objects
    for(size_t i=0; i<MAX_FRAME_IN_FLIGHT; ++i)
    {
      vkDestroyFence(context.device, render_target.frames[i].fence, nullptr);
      vkDestroySemaphore(context.device, render_target.frames[i].image_available_semaphore, nullptr);
      vkDestroySemaphore(context.device, render_target.frames[i].render_finished_semaphore, nullptr);
    }

    deinit_render_pass(context, render_target.render_pass);
    deinit_swapchain(context, render_target.swapchain);
  }

  bool render_target_begin_frame(const Context& context, RenderTarget& render_target, Frame& frame)
  {
    // Acquire frame resource
    frame = render_target.frames[render_target.frame_index];
    render_target.frame_index = (render_target.frame_index + 1) % MAX_FRAME_IN_FLIGHT;

    auto result = swapchain_next_image_index(context, render_target.swapchain, frame.image_available_semaphore, render_target.image_index);
    if(result != SwapchainResult::SUCCESS)
      return false;

    // Wait and begin commannd buffer recording
    VK_CHECK(vkWaitForFences(context.device, 1, &frame.fence, VK_TRUE, UINT64_MAX));
    VK_CHECK(vkResetFences(context.device, 1, &frame.fence));

    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    VK_CHECK(vkResetCommandBuffer(frame.command_buffer, 0));
    VK_CHECK(vkBeginCommandBuffer(frame.command_buffer, &begin_info));

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

    vkCmdBeginRenderPass(frame.command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
    return true;
  }

  bool render_target_end_frame(const Context& context, RenderTarget& render_target, const Frame& frame)
  {
    vkCmdEndRenderPass(frame.command_buffer);

    // End command buffer recording and submit
    VK_CHECK(vkEndCommandBuffer(frame.command_buffer));

    VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores    = &frame.image_available_semaphore;
    submit_info.pWaitDstStageMask  = &wait_stage;

    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores    = &frame.render_finished_semaphore;

    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers    = &frame.command_buffer;
    VK_CHECK(vkQueueSubmit(context.queue, 1, &submit_info, frame.fence));

    return swapchain_present_image_index(context, render_target.swapchain, frame.render_finished_semaphore, render_target.image_index) == SwapchainResult::SUCCESS;
  }
}
