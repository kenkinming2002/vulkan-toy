#pragma once

#include "vulkan.hpp"
#include "context.hpp"

namespace vulkan
{
  struct CommandBuffer
  {
    VkCommandBuffer handle;
    VkFence fence;
  };

  CommandBuffer create_command_buffer(const Context& context);
  void destroy_command_buffer(const Context& context, CommandBuffer command_buffer);

  void command_buffer_begin(const Context& context, CommandBuffer command_buffer);
  void command_buffer_end(const Context& context, CommandBuffer command_buffer,
      VkSemaphore wait_semaphore, VkPipelineStageFlags wait_stage,
      VkSemaphore signal_semaphore);
}
