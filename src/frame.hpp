#pragma once

#include "core/context.hpp"
#include "core/command_buffer.hpp"

namespace vulkan
{
  struct Frame
  {
    command_buffer_t command_buffer;
    VkFence         fence;
    VkSemaphore     image_available_semaphore;
    VkSemaphore     render_finished_semaphore;
  };

  void frame_init(const Context& context, Frame& frame);
  void frame_deinit(const Context& context, Frame& frame);
}
