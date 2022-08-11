#pragma once

#include "vulkan.hpp"
#include "context.hpp"
#include "fence.hpp"

namespace vulkan
{
  struct CommandBuffer
  {
    VkCommandBuffer handle;
  };

  void init_command_buffer(const Context& context, CommandBuffer& command_buffer);
  void deinit_command_buffer(const Context& context, CommandBuffer& command_buffer);

  void command_buffer_begin(const CommandBuffer& command_buffer);
  void command_buffer_end(const CommandBuffer& command_buffer);

  void command_buffer_submit(const Context& context, const CommandBuffer& command_buffer, const Fence& fence);
  void command_buffer_submit(const Context& context, const CommandBuffer& command_buffer, const Fence& fence,
      VkSemaphore wait_semaphore, VkPipelineStageFlags wait_stage,
      VkSemaphore signal_semaphore);
}
