#pragma once

#include "context.hpp"
#include "attachment.hpp"

namespace vulkan
{
  struct FramebufferCreateInfo
  {
    VkRenderPass      render_pass;

    VkExtent2D        extent;
    const Attachment *attachments;
    uint32_t          attachment_count;
  };

  struct Framebuffer
  {
    VkFramebuffer handle;
  };

  void init_framebuffer(const Context& context, FramebufferCreateInfo create_info, Framebuffer& framebuffer);
  void deinit_framebuffer(const Context& context, Framebuffer& framebuffer);
}
