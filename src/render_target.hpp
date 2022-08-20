#pragma once

#include "core/command_buffer.hpp"
#include "core/context.hpp"
#include "core/fence.hpp"
#include "frame.hpp"
#include "framebuffer.hpp"
#include "render_pass.hpp"
#include "resources/image.hpp"
#include "resources/image_view.hpp"
#include "core/semaphore.hpp"
#include "swapchain.hpp"

namespace vulkan
{
  // Double buffering
  static constexpr size_t MAX_FRAME_IN_FLIGHT = 2;
  struct RenderTarget
  {
    Swapchain swapchain;

    RenderPass   render_pass;
    image_t      depth_image;
    image_view_t depth_image_view;
    Framebuffer *framebuffers;

    uint32_t image_index;
    uint32_t image_count;

    Frame frames[MAX_FRAME_IN_FLIGHT];
    size_t frame_index;
  };

  void render_target_init(const Context& context, Allocator& allocator, RenderTarget& render_target);
  void render_target_deinit(const Context& context, Allocator& allocator, RenderTarget& render_target);

  bool render_target_begin_frame(const Context& context, RenderTarget& render_target, Frame& frame);
  bool render_target_end_frame(const Context& context, RenderTarget& render_target, const Frame& frame);
}

