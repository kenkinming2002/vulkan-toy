#pragma once

#include "context.hpp"
#include "attachment.hpp"

namespace vulkan
{
  struct FramebufferInfo
  {
    uint32_t   count;
    VkExtent2D extent;
  };

  struct FramebufferCreateInfo
  {
    FramebufferInfo framebuffer_info;

    VkRenderPass      render_pass;
    const Attachment *attachments;
    uint32_t          attachment_count;
  };

  struct Framebuffer
  {
    FramebufferInfo info;
    VkFramebuffer *handles;
  };

  void init_framebuffer(const Context& context, Framebuffer& framebuffer, const Attachment *attachments, uint32_t attachment_count);
  void deinit_framebuffer(const Context& context, Framebuffer& framebuffer);
}
