#pragma once

#include "core/command_buffer.hpp"
#include "core/context.hpp"
#include "frame.hpp"
#include "framebuffer.hpp"
#include "render_pass.hpp"
#include "resources/image.hpp"
#include "resources/image_view.hpp"
#include "swapchain.hpp"

namespace vulkan
{
  typedef struct RenderTarget *render_target_t;

  render_target_t render_target_create(context_t context, allocator_t allocator, swapchain_t swapchain);
  void render_target_destroy(render_target_t render_target);

  const Frame *render_target_begin_frame(render_target_t render_target);
  bool render_target_end_frame(render_target_t render_target, const Frame *frame);

  VkRenderPass render_target_get_render_pass(render_target_t render_target);
  void render_target_get_extent(render_target_t render_target, unsigned& width, unsigned& height);
}

