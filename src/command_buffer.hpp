#pragma once

#include "command_buffer.hpp"
#include "context.hpp"
#include "descriptor_set.hpp"
#include "fence.hpp"
#include "pipeline.hpp"
#include "semaphore.hpp"

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
      Semaphore wait_semaphore, VkPipelineStageFlags wait_stage,
      Semaphore signal_semaphore);

  void command_push_constant(CommandBuffer command_buffer, Pipeline2 pipeline, ShaderStage stage, void *data, size_t offset, size_t size);
  void command_bind_descriptor_set(CommandBuffer command_buffer, Pipeline2 pipeline, DescriptorSet descriptor_set);
}
