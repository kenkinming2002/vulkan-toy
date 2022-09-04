#pragma once

#include "core/context.hpp"
#include "resources/texture.hpp"

namespace vulkan
{
  REF_DECLARE(Framebuffer, framebuffer_t);
  framebuffer_t framebuffer_create(context_t context,
      VkRenderPass render_pass,
      VkExtent2D   extent,
      const texture_t *attachments,
      size_t           attachment_count);

  VkFramebuffer framebuffer_get_handle(framebuffer_t framebuffer);
}
