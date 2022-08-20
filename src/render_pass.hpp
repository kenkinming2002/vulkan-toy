#pragma once

#include "core/context.hpp"

#include <vulkan/vulkan.h>

namespace vulkan
{
  struct RenderPass
  {
    VkRenderPass handle;
  };

  struct RenderPassCreateInfoSimple
  {
    VkFormat color_format;
    VkFormat depth_format;
  };

  // Init simple render pass with single subpass, single color and depth attachment and no multi-sampling
  void init_render_pass_simple(const Context& context, RenderPassCreateInfoSimple create_info, RenderPass& render_pass);
  void deinit_render_pass(const Context& context, RenderPass& render_pass);
}
