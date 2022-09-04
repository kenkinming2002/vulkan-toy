#include "framebuffer.hpp"

#include "vk_check.hpp"

namespace vulkan
{
  struct Framebuffer
  {
    Ref ref;
    context_t context;
    VkFramebuffer handle;
  };
  REF_DEFINE(Framebuffer, framebuffer_t, ref);

  static void framebuffer_free(ref_t ref)
  {
    framebuffer_t framebuffer = container_of(ref, Framebuffer, ref);

    VkDevice device = context_get_device_handle(framebuffer->context);
    vkDestroyFramebuffer(device, framebuffer->handle, nullptr);

    delete framebuffer;
  }

  framebuffer_t framebuffer_create(context_t context,
      VkRenderPass     render_pass,
      VkExtent2D       extent,
      const texture_t *attachments,
      size_t           attachment_count)
  {
    framebuffer_t framebuffer = new Framebuffer;
    framebuffer->ref.count = 1;
    framebuffer->ref.free  = framebuffer_free;

    get(context);
    framebuffer->context = context;

    VkDevice device = context_get_device_handle(framebuffer->context);

    size_t       image_view_count = attachment_count;
    VkImageView *image_views      = new VkImageView[image_view_count];
    for(size_t i=0; i<image_view_count; ++i)
      image_views[i] = image_view_get_handle(texture_get_image_view(attachments[i]));

    VkFramebufferCreateInfo framebuffer_create_info = {};
    framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebuffer_create_info.renderPass      = render_pass;
    framebuffer_create_info.attachmentCount = image_view_count;
    framebuffer_create_info.pAttachments    = image_views;
    framebuffer_create_info.width           = extent.width;
    framebuffer_create_info.height          = extent.height;
    framebuffer_create_info.layers          = 1;
    VK_CHECK(vkCreateFramebuffer(device, &framebuffer_create_info, nullptr, &framebuffer->handle));

    delete[] image_views;

    return framebuffer;
  }

  VkFramebuffer framebuffer_get_handle(framebuffer_t framebuffer) { return framebuffer->handle; }
}
