#include "render_context.hpp"

#include "src/image.hpp"
#include "src/image_view.hpp"
#include "vk_check.hpp"

#include <vulkan/vulkan_core.h>

namespace vulkan
{
  void init_render_context(const Context& context, Allocator& allocator, RenderContextCreateInfo create_info, RenderContext& render_context)
  {
    vulkan::Shader vertex_shader = {};
    vulkan::Shader fragment_shader = {};

    vulkan::load_shader(context, create_info.vertex_shader_file_name, vertex_shader);
    vulkan::load_shader(context, create_info.fragment_shader_file_name, fragment_shader);

    init_swapchain(context, render_context.swapchain);
    init_render_pass_simple(context, RenderPassCreateInfoSimple{
      .color_format = render_context.swapchain.surface_format.format,
      .depth_format = VK_FORMAT_D32_SFLOAT,
    }, render_context.render_pass);
    init_pipeline2(context, PipelineCreateInfo2{
      .render_pass         = render_context.render_pass,
      .vertex_shader       = vertex_shader,
      .fragment_shader     = fragment_shader,
      .vertex_input        = create_info.vertex_input,
      .descriptor_input    = create_info.descriptor_input,
      .push_constant_input = create_info.push_constant_input,
    }, render_context.pipeline);

    vulkan::deinit_shader(context, vertex_shader);
    vulkan::deinit_shader(context, fragment_shader);

    // 5: Create framebuffers
    init_image(context, allocator, ImageCreateInfo{
      .type   = ImageType::DEPTH_ATTACHMENT,
      .format = VK_FORMAT_D32_SFLOAT,
      .extent = render_context.swapchain.extent,
    }, render_context.depth_image);
    init_image_view(context, ImageViewCreateInfo{
      .type   = ImageViewType::DEPTH,
      .format = VK_FORMAT_D32_SFLOAT,
      .image  = render_context.depth_image,
    }, render_context.depth_image_view);

    render_context.framebuffers = new Framebuffer[render_context.swapchain.image_count];
    for(uint32_t i=0; i<render_context.swapchain.image_count; ++i)
    {
      const ImageView image_views[] = {
        render_context.swapchain.image_views[i],
        render_context.depth_image_view,
      };
      init_framebuffer(context, FramebufferCreateInfo{
        .render_pass      = render_context.render_pass.handle,
        .extent           = render_context.swapchain.extent,
        .image_views      = image_views,
        .image_view_count = std::size(image_views),
      }, render_context.framebuffers[i]);
    }

    // 7: Create frame resources
    for(size_t i=0; i<MAX_FRAME_IN_FLIGHT; ++i)
    {
      init_command_buffer(context, render_context.frames[i].command_buffer);
      init_fence(context, render_context.frames[i].fence, true);
      render_context.frames[i].semaphore_image_available = create_semaphore(context.device);
      render_context.frames[i].semaphore_render_finished = create_semaphore(context.device);
    }
    render_context.frame_index = 0;
  }

  void deinit_render_context(const Context& context, Allocator& allocator, RenderContext& render_context)
  {
    (void)allocator;

    for(uint32_t i=0; i<render_context.swapchain.image_count; ++i)
      deinit_framebuffer(context, render_context.framebuffers[i]);

    delete[] render_context.framebuffers;

    deinit_image(context, allocator, render_context.depth_image);
    deinit_image_view(context, render_context.depth_image_view);

    for(size_t i=0; i<MAX_FRAME_IN_FLIGHT; ++i)
    {
      deinit_command_buffer(context, render_context.frames[i].command_buffer);
      deinit_fence(context, render_context.frames[i].fence);
      destroy_semaphore(context.device, render_context.frames[i].semaphore_image_available);
      destroy_semaphore(context.device, render_context.frames[i].semaphore_render_finished);
    }

    deinit_pipeline2(context, render_context.pipeline);
    deinit_render_pass(context, render_context.render_pass);
    deinit_swapchain(context, render_context.swapchain);
  }

  bool begin_render(const Context& context, RenderContext& render_context, Frame& frame)
  {
    // Acquire frame resource
    frame = render_context.frames[render_context.frame_index];
    render_context.frame_index = (render_context.frame_index + 1) % MAX_FRAME_IN_FLIGHT;

    auto result = swapchain_next_image_index(context, render_context.swapchain, frame.semaphore_image_available, render_context.image_index);
    if(result != SwapchainResult::SUCCESS)
      return false;

    // Wait and begin commannd buffer recording
    fence_wait_and_reset(context, frame.fence);
    command_buffer_begin(frame.command_buffer);

    VkRenderPassBeginInfo render_pass_begin_info = {};
    render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_begin_info.renderPass        = render_context.render_pass.handle;
    render_pass_begin_info.framebuffer       = render_context.framebuffers[render_context.image_index].handle;
    render_pass_begin_info.renderArea.offset = {0, 0};
    render_pass_begin_info.renderArea.extent = render_context.swapchain.extent;

    // TODO: Take this as argument
    VkClearValue clear_values[2] = {};
    clear_values[0].color        = {{ 0.0f, 0.0f, 0.0f, 1.0f }};
    clear_values[1].depthStencil = { 1.0f, 0 };

    render_pass_begin_info.clearValueCount = 2;
    render_pass_begin_info.pClearValues = clear_values;

    vkCmdBeginRenderPass(frame.command_buffer.handle, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(frame.command_buffer.handle, VK_PIPELINE_BIND_POINT_GRAPHICS, render_context.pipeline.handle);
    return true;
  }

  bool end_render(const Context& context, RenderContext& render_context, const Frame& frame)
  {
    vkCmdEndRenderPass(frame.command_buffer.handle);

    // End command buffer recording and submit
    command_buffer_end(frame.command_buffer);
    command_buffer_submit(context, frame.command_buffer, frame.fence,
        frame.semaphore_image_available, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        frame.semaphore_render_finished);

    return swapchain_present_image_index(context, render_context.swapchain, frame.semaphore_render_finished, render_context.image_index) == SwapchainResult::SUCCESS;
  }
}
