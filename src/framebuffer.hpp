#pragma once

#include "core/context.hpp"
#include "resources/image_view.hpp"

namespace vulkan
{
  struct FramebufferCreateInfo
  {
    VkRenderPass      render_pass;

    VkExtent2D          extent;
    const image_view_t *image_views;
    uint32_t            image_view_count;
  };

  struct Framebuffer
  {
    VkFramebuffer handle;
  };

  void init_framebuffer(context_t context, FramebufferCreateInfo create_info, Framebuffer& framebuffer);
  void deinit_framebuffer(context_t context, Framebuffer& framebuffer);
}
