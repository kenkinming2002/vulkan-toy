#include "framebuffer.hpp"

#include "vk_check.hpp"
#include "utils.hpp"

namespace vulkan
{
  void init_framebuffer(const Context& context, FramebufferCreateInfo create_info, Framebuffer& framebuffer)
  {
    framebuffer.info = create_info.framebuffer_info;

    framebuffer.handles = new VkFramebuffer[framebuffer.info.count];

    dynarray<VkImageView> attachments = create_dynarray<VkImageView>(create_info.attachment_count);
    for(uint32_t i=0; i<framebuffer.info.count; ++i)
    {
      for(uint32_t j=0; j<create_info.attachment_count; ++j)
        attachments[j] = create_info.attachments[j].image_views[i];

      VkFramebufferCreateInfo framebuffer_create_info = {};
      framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
      framebuffer_create_info.renderPass      = create_info.render_pass;
      framebuffer_create_info.attachmentCount = size(attachments);
      framebuffer_create_info.pAttachments    = data(attachments);
      framebuffer_create_info.width           = framebuffer.info.extent.width;
      framebuffer_create_info.height          = framebuffer.info.extent.height;
      framebuffer_create_info.layers          = 1;
      VK_CHECK(vkCreateFramebuffer(context.device, &framebuffer_create_info, nullptr, &framebuffer.handles[i]));
    }
    destroy_dynarray(attachments);
  }

  void deinit_framebuffer(const Context& context, Framebuffer& framebuffer)
  {
    for(uint32_t i=0; i<framebuffer.info.count; ++i)
    {
      vkDestroyFramebuffer(context.device, framebuffer.handles[i], nullptr);
      framebuffer.handles[i] = VK_NULL_HANDLE;
    }
    delete[] framebuffer.handles;
    framebuffer.handles = nullptr;

    framebuffer = {};
  }
}
