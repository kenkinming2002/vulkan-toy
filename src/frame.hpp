#pragma once

#include "context.hpp"

namespace vulkan
{
  struct Frame
  {
    VkCommandBuffer command_buffer;
    VkFence         fence;
    VkSemaphore     image_available_semaphore;
    VkSemaphore     render_finished_semaphore;
  };

  void frame_init(const Context& context, Frame& frame);
  void frame_deinit(const Context& context, Frame& frame);
}
