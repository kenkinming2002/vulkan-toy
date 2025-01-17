#pragma once

#include "core/command_buffer.hpp"
#include "core/context.hpp"
#include "frame.hpp"
#include "ref.hpp"
#include "resources/allocator.hpp"
#include "swapchain.hpp"

namespace vulkan
{
  REF_DECLARE(RenderTarget, render_target_t);
  render_target_t render_target_create(context_t context, allocator_t allocator, swapchain_t swapchain);

  void render_target_on_invalidate(render_target_t render_target, Delegate& delegate);

  const Frame *render_target_begin_frame(render_target_t render_target);
  void render_target_end_frame(render_target_t render_target, const Frame *frame);

  VkRenderPass render_target_get_render_pass(render_target_t render_target);
  void render_target_get_extent(render_target_t render_target, unsigned& width, unsigned& height);
}

