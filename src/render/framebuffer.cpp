#include "framebuffer.hpp"

#include "vk_check.hpp"
#include "utils.hpp"

#include <type_traits>

namespace vulkan
{
  void init_framebuffer(context_t context, FramebufferCreateInfo create_info, Framebuffer& framebuffer)
  {
    VkDevice device = context_get_device_handle(context);

    dynarray<VkImageView> attachments = create_dynarray<VkImageView>(create_info.image_view_count);
    for(uint32_t i=0; i<create_info.image_view_count; ++i)
      attachments[i] = image_view_get_handle(create_info.image_views[i]);

    VkFramebufferCreateInfo framebuffer_create_info = {};
    framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebuffer_create_info.renderPass      = create_info.render_pass;
    framebuffer_create_info.attachmentCount = size(attachments);
    framebuffer_create_info.pAttachments    = data(attachments);
    framebuffer_create_info.width           = create_info.extent.width;
    framebuffer_create_info.height          = create_info.extent.height;
    framebuffer_create_info.layers          = 1;
    VK_CHECK(vkCreateFramebuffer(device, &framebuffer_create_info, nullptr, &framebuffer.handle));

    destroy_dynarray(attachments);
  }

  void deinit_framebuffer(context_t context, Framebuffer& framebuffer)
  {
    VkDevice device = context_get_device_handle(context);

    vkDestroyFramebuffer(device, framebuffer.handle, nullptr);
    framebuffer.handle = VK_NULL_HANDLE;
  }
}
