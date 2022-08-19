#pragma once

#include "command_buffer.hpp"
#include "context.hpp"
#include "fence.hpp"
#include "framebuffer.hpp"
#include "resources/image.hpp"
#include "image_view.hpp"
#include "render_pass.hpp"
#include "semaphore.hpp"
#include "swapchain.hpp"
#include "frame.hpp"

namespace vulkan
{
  // Double buffering
  static constexpr size_t MAX_FRAME_IN_FLIGHT = 2;
  struct RenderTarget
  {
    Swapchain swapchain;

    RenderPass   render_pass;
    Image        depth_image;
    ImageView    depth_image_view;
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

