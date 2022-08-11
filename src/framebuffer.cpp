#include "framebuffer.hpp"

#include "vk_check.hpp"
#include "utils.hpp"

#include <type_traits>

namespace vulkan
{
  void init_framebuffer(const Context& context, FramebufferCreateInfo create_info, Framebuffer& framebuffer)
  {
    dynarray<VkImageView> attachments = create_dynarray<VkImageView>(create_info.image_view_count);
    for(uint32_t i=0; i<create_info.image_view_count; ++i)
      attachments[i] = create_info.image_views[i].handle;

    VkFramebufferCreateInfo framebuffer_create_info = {};
    framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebuffer_create_info.renderPass      = create_info.render_pass;
    framebuffer_create_info.attachmentCount = size(attachments);
    framebuffer_create_info.pAttachments    = data(attachments);
    framebuffer_create_info.width           = create_info.extent.width;
    framebuffer_create_info.height          = create_info.extent.height;
    framebuffer_create_info.layers          = 1;
    VK_CHECK(vkCreateFramebuffer(context.device, &framebuffer_create_info, nullptr, &framebuffer.handle));
  }

  void deinit_framebuffer(const Context& context, Framebuffer& framebuffer)
  {
    vkDestroyFramebuffer(context.device, framebuffer.handle, nullptr);
    framebuffer.handle = VK_NULL_HANDLE;
  }
}
