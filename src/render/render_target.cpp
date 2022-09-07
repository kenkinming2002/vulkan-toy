#include "render_target.hpp"

#include "resources/texture.hpp"
#include "framebuffer.hpp"
#include "render_pass.hpp"

#include "vk_check.hpp"

#include <array>

#include <stdio.h>

namespace vulkan
{
  // Double buffering
  static constexpr size_t MAX_FRAME_IN_FLIGHT = 2;
  struct RenderTarget
  {
    Ref ref;

    context_t   context;
    allocator_t allocator;

    swapchain_t swapchain;
    Delegate    on_swapchain_invalidate;

    Frame frames[MAX_FRAME_IN_FLIGHT];
    size_t frame_index;

    DelegateChain on_invalidate;

    VkRenderPass   render_pass;

    texture_t depth_texture;

    uint32_t       framebuffer_count;
    uint32_t       framebuffer_index;
    framebuffer_t *framebuffers;
  };
  REF_DEFINE(RenderTarget, render_target_t, ref);

  static void render_target_init(render_target_t render_target)
  {
    VkDevice device = context_get_device_handle(render_target->context);

    // Render pass
    VkAttachmentDescription color_attachment_description = {};
    color_attachment_description.format         = swapchain_get_format(render_target->swapchain);
    color_attachment_description.samples        = VK_SAMPLE_COUNT_1_BIT;
    color_attachment_description.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment_description.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment_description.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment_description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment_description.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment_description.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentDescription depth_attachment_description = {};
    depth_attachment_description.format         = VK_FORMAT_D32_SFLOAT;
    depth_attachment_description.samples        = VK_SAMPLE_COUNT_1_BIT;
    depth_attachment_description.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment_description.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    depth_attachment_description.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depth_attachment_description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment_description.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    depth_attachment_description.finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription attachment_descriptions[] = {
      color_attachment_description,
      depth_attachment_description,
    };

    VkAttachmentReference color_attachment_reference = {};
    color_attachment_reference.attachment = 0;
    color_attachment_reference.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depth_attachment_reference = {};
    depth_attachment_reference.attachment = 1;
    depth_attachment_reference.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass_description = {};
    subpass_description.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass_description.colorAttachmentCount    = 1;
    subpass_description.pColorAttachments       = &color_attachment_reference;
    subpass_description.pDepthStencilAttachment = &depth_attachment_reference;

    VkSubpassDependency subpass_dependency = {};
    subpass_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    subpass_dependency.dstSubpass = 0;
    subpass_dependency.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    subpass_dependency.srcAccessMask = 0;
    subpass_dependency.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    subpass_dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;


    VkRenderPassCreateInfo render_pass_create_info = {};
    render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_create_info.attachmentCount = std::size(attachment_descriptions);
    render_pass_create_info.pAttachments    = attachment_descriptions;
    render_pass_create_info.subpassCount    = 1;
    render_pass_create_info.pSubpasses      = &subpass_description;
    render_pass_create_info.dependencyCount = 1;
    render_pass_create_info.pDependencies   = &subpass_dependency;

    VK_CHECK(vkCreateRenderPass(device, &render_pass_create_info, nullptr, &render_target->render_pass));

    // Depth attachment
    VkExtent2D extent = swapchain_get_extent(render_target->swapchain);
    render_target->depth_texture = texture_create(render_target->context, render_target->allocator,
      ImageType::DEPTH_ATTACHMENT,
      VK_FORMAT_D32_SFLOAT,
      extent.width,
      extent.height,
      1,
      ImageViewType::DEPTH
    );

    // Framebuffers
    uint32_t swapchain_texture_count    = swapchain_get_texture_count(render_target->swapchain);
    const texture_t *swapchain_textures = swapchain_get_textures(render_target->swapchain);

    render_target->framebuffer_count = swapchain_texture_count;
    render_target->framebuffer_index = 0;
    render_target->framebuffers = new framebuffer_t[render_target->framebuffer_count];
    for(uint32_t i=0; i<render_target->framebuffer_count; ++i)
    {
      const texture_t attachments[] = { swapchain_textures[i], render_target->depth_texture, };
      render_target->framebuffers[i] = framebuffer_create(
          render_target->context,
          render_target->render_pass,
          swapchain_get_extent(render_target->swapchain),
          attachments, std::size(attachments));
    }
  }

  static void render_target_deinit(render_target_t render_target)
  {
    for(uint32_t i=0; i<render_target->framebuffer_count; ++i)
      put(render_target->framebuffers[i]);

    delete[] render_target->framebuffers;

    put(render_target->depth_texture);

    VkDevice device = context_get_device_handle(render_target->context);
    vkDestroyRenderPass(device, render_target->render_pass, nullptr);
  }

  static void render_target_invalidate(render_target_t render_target)
  {
    VkDevice device = context_get_device_handle(render_target->context);
    vkDeviceWaitIdle(device);

    render_target_deinit(render_target);
    render_target_init(render_target);
    delegate_chain_invoke(render_target->on_invalidate);
  }

  static void on_swapchain_invalidate(void *data)
  {
    render_target_t render_target = static_cast<render_target_t>(data);
    printf("swapchain invalidate received\n");
    render_target_invalidate(render_target);
  }

  static void render_target_free(ref_t ref)
  {
    render_target_t render_target = container_of(ref, RenderTarget, ref);

    render_target_deinit(render_target);

    for(size_t i=0; i<MAX_FRAME_IN_FLIGHT; ++i)
      frame_deinit(render_target->context, render_target->frames[i]);

    delegate_chain_deregister(render_target->on_swapchain_invalidate);

    put(render_target->swapchain);
    put(render_target->allocator);
    put(render_target->context);

    delete render_target;
  }

  render_target_t render_target_create(context_t context, allocator_t allocator, swapchain_t swapchain)
  {
    render_target_t render_target = new RenderTarget;
    render_target->ref.count = 1;
    render_target->ref.free  = render_target_free;

    get(context);
    get(allocator);
    get(swapchain);

    render_target->context   = context;
    render_target->allocator = allocator;
    render_target->swapchain = swapchain;

    render_target->on_swapchain_invalidate = Delegate{ .node = {}, .ptr  = on_swapchain_invalidate, .data = render_target, };
    swapchain_on_invalidate(render_target->swapchain, render_target->on_swapchain_invalidate);

    for(size_t i=0; i<MAX_FRAME_IN_FLIGHT; ++i)
      frame_init(render_target->context, render_target->frames[i]);
    render_target->frame_index = 0;

    delegate_chain_init(render_target->on_invalidate);
    render_target_init(render_target);

    return render_target;
  }

  void render_target_on_invalidate(render_target_t render_target, Delegate& delegate)
  {
    delegate_chain_register(render_target->on_invalidate, delegate);
  }

  const Frame *render_target_begin_frame(render_target_t render_target)
  {
    // Acquire frame resource
    const Frame *frame = &render_target->frames[render_target->frame_index];
    render_target->frame_index = (render_target->frame_index + 1) % MAX_FRAME_IN_FLIGHT;

    swapchain_next_image_index(render_target->swapchain, frame->image_available_semaphore, render_target->framebuffer_index);

    // Wait and begin commannd buffer recording
    command_buffer_wait(frame->command_buffer);
    command_buffer_reset(frame->command_buffer);
    command_buffer_begin(frame->command_buffer);

    VkRenderPassBeginInfo render_pass_begin_info = {};
    render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_begin_info.renderPass        = render_target->render_pass;
    render_pass_begin_info.framebuffer       = framebuffer_get_handle(render_target->framebuffers[render_target->framebuffer_index]);
    render_pass_begin_info.renderArea.offset = {0, 0};
    render_pass_begin_info.renderArea.extent = swapchain_get_extent(render_target->swapchain);

    // TODO: Take this as argument
    VkClearValue clear_values[2] = {};
    clear_values[0].color        = {{ 0.0f, 0.0f, 0.0f, 1.0f }};
    clear_values[1].depthStencil = { 1.0f, 0 };

    render_pass_begin_info.clearValueCount = 2;
    render_pass_begin_info.pClearValues = clear_values;

    VkCommandBuffer handle = command_buffer_get_handle(frame->command_buffer);
    vkCmdBeginRenderPass(handle, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
    return frame;
  }

  void render_target_end_frame(render_target_t render_target, const Frame *frame)
  {
    VkCommandBuffer handle = command_buffer_get_handle(frame->command_buffer);
    vkCmdEndRenderPass(handle);

    // End command buffer recording and submit
    command_buffer_end(frame->command_buffer);
    command_buffer_submit(frame->command_buffer, frame->image_available_semaphore, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, frame->render_finished_semaphore);

    swapchain_present_image_index(render_target->swapchain, frame->render_finished_semaphore, render_target->framebuffer_index);
  }

  VkRenderPass render_target_get_render_pass(render_target_t render_target)
  {
    return render_target->render_pass;
  }

  void render_target_get_extent(render_target_t render_target, unsigned& width, unsigned& height)
  {
    VkExtent2D extent = swapchain_get_extent(render_target->swapchain);
    width  = extent.width;
    height = extent.height;
  }
}
