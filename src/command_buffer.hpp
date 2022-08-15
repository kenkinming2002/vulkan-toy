#pragma once

#include "command_buffer.hpp"
#include "context.hpp"
#include "fence.hpp"
#include "pipeline.hpp"
#include "vulkan.hpp"

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

  void command_push_constant(CommandBuffer command_buffer, Pipeline2 pipeline, ShaderStage stage, void *data, size_t offset, size_t size);
}
