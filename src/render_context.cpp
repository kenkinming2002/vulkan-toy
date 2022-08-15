#include "render_context.hpp"

#include "src/image.hpp"
#include "src/image_view.hpp"
#include "vk_check.hpp"

#include <vulkan/vulkan_core.h>

namespace vulkan
{
  void init_render_context(const Context& context, Allocator& allocator, RenderContextCreateInfo create_info, RenderContext& render_context)
  {
    init_swapchain(context, render_context.swapchain);
    init_render_pass_simple(context, RenderPassCreateInfoSimple{
      .color_format = render_context.swapchain.surface_format.format,
      .depth_format = VK_FORMAT_D32_SFLOAT,
    }, render_context.render_pass);
    init_pipeline2(context, PipelineCreateInfo2{
      .render_pass         = render_context.render_pass,
      .vertex_shader       = create_info.vertex_shader,
      .fragment_shader     = create_info.fragment_shader,
      .vertex_input        = create_info.vertex_input,
      .descriptor_input    = create_info.descriptor_input,
      .push_constant_input = create_info.push_constant_input,
    }, render_context.pipeline);

    // 5: Create image resources
    {
      render_context.image_resources = new ImageResource[render_context.swapchain.image_count];
      for(uint32_t i=0; i<render_context.swapchain.image_count; ++i)
      {
        ImageResource image_resource = {};

        init_image(context, allocator, ImageCreateInfo{
          .type   = ImageType::DEPTH_ATTACHMENT,
          .format = VK_FORMAT_D32_SFLOAT,
          .extent = render_context.swapchain.extent,
        }, image_resource.depth_image);
        init_image_view(context, ImageViewCreateInfoSwapchain{
          .swapchain = render_context.swapchain,
          .index     = i,
        }, image_resource.color_view);
        init_image_view(context, ImageViewCreateInfo{
          .type   = ImageViewType::DEPTH,
          .format = VK_FORMAT_D32_SFLOAT,
          .image  = image_resource.depth_image,
        }, image_resource.depth_view);

        const ImageView image_views[] = {
          image_resource.color_view,
          image_resource.depth_view,
        };
        init_framebuffer(context, FramebufferCreateInfo{
          .render_pass      = render_context.render_pass.handle,
          .extent           = render_context.swapchain.extent,
          .image_views      = image_views,
          .image_view_count = std::size(image_views),
        }, image_resource.framebuffer);

        render_context.image_resources[i] = image_resource;
      }
    }

    // 7: Create frame resources
    {
      render_context.frame_resources = new FrameResource[create_info.max_frame_in_flight];
      for(uint32_t i=0; i<create_info.max_frame_in_flight; ++i)
      {
        FrameResource frame_resource = {};
        init_command_buffer(context, frame_resource.command_buffer);
        init_fence(context, frame_resource.fence, true);
        frame_resource.semaphore_image_available = vulkan::create_semaphore(context.device);
        frame_resource.semaphore_render_finished = vulkan::create_semaphore(context.device);
        render_context.frame_resources[i] = frame_resource;
      }
      render_context.frame_count = create_info.max_frame_in_flight;
      render_context.frame_index = 0;
    }
  }

  void deinit_render_context(const Context& context, Allocator& allocator, RenderContext& render_context)
  {
    (void)allocator;

    for(uint32_t i=0; i<render_context.swapchain.image_count; ++i)
    {
      ImageResource& frame = render_context.image_resources[i];
      deinit_framebuffer(context, frame.framebuffer);
      deinit_image(context, allocator, frame.depth_image);
      deinit_image_view(context, frame.color_view);
      deinit_image_view(context, frame.depth_view);
    }
    delete[] render_context.image_resources;

    for(uint32_t i=0; i<render_context.frame_count; ++i)
    {
      FrameResource& frame = render_context.frame_resources[i];
      deinit_command_buffer(context, frame.command_buffer);
      deinit_fence(context, frame.fence);
      vulkan::destroy_semaphore(context.device, frame.semaphore_image_available);
      vulkan::destroy_semaphore(context.device, frame.semaphore_render_finished);
    }
    delete[] render_context.frame_resources;

    deinit_pipeline2(context, render_context.pipeline);
    deinit_render_pass(context, render_context.render_pass);
    deinit_swapchain(context, render_context.swapchain);
  }

  std::optional<RenderInfo> begin_render(const Context& context, RenderContext& render_context)
  {
    // Render info
    RenderInfo info  = {};

    // Acquire frame resource
    info.frame_index = render_context.frame_index;
    render_context.frame_index = (render_context.frame_index + 1) % render_context.frame_count;
    FrameResource frame_resource = render_context.frame_resources[info.frame_index];

    auto result = swapchain_next_image_index(context, render_context.swapchain, frame_resource.semaphore_image_available, info.image_index);
    if(result != SwapchainResult::SUCCESS)
      return std::nullopt;

    ImageResource image_resource = render_context.image_resources[info.image_index];

    // Wait and begin commannd buffer recording
    fence_wait_and_reset(context, frame_resource.fence);
    command_buffer_begin(frame_resource.command_buffer);

    VkRenderPassBeginInfo render_pass_begin_info = {};
    render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_begin_info.renderPass        = render_context.render_pass.handle;
    render_pass_begin_info.framebuffer       = image_resource.framebuffer.handle;
    render_pass_begin_info.renderArea.offset = {0, 0};
    render_pass_begin_info.renderArea.extent = render_context.swapchain.extent;

    // TODO: Take this as argument
    VkClearValue clear_values[2] = {};
    clear_values[0].color        = {{ 0.0f, 0.0f, 0.0f, 1.0f }};
    clear_values[1].depthStencil = { 1.0f, 0 };

    render_pass_begin_info.clearValueCount = 2;
    render_pass_begin_info.pClearValues = clear_values;

    vkCmdBeginRenderPass(frame_resource.command_buffer.handle, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(frame_resource.command_buffer.handle, VK_PIPELINE_BIND_POINT_GRAPHICS, render_context.pipeline.handle);

    info.semaphore_image_available = frame_resource.semaphore_image_available;
    info.semaphore_render_finished = frame_resource.semaphore_render_finished;
    info.command_buffer            = frame_resource.command_buffer;
    return info;
  }

  bool end_render(const Context& context, RenderContext& render_context, RenderInfo info)
  {
    FrameResource frame_resource = render_context.frame_resources[info.frame_index];

    vkCmdEndRenderPass(frame_resource.command_buffer.handle);

    // End command buffer recording and submit
    command_buffer_end(frame_resource.command_buffer);
    command_buffer_submit(context, frame_resource.command_buffer, frame_resource.fence,
        frame_resource.semaphore_image_available, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        frame_resource.semaphore_render_finished);

    return swapchain_present_image_index(context, render_context.swapchain, frame_resource.semaphore_render_finished, info.image_index) == SwapchainResult::SUCCESS;
  }
}
