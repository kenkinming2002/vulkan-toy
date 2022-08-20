#pragma once

#include "context.hpp"
#include "fence.hpp"
#include "resources/ref.hpp"
#include "semaphore.hpp"


namespace vulkan
{
  typedef struct CommandBuffer *command_buffer_t;

  command_buffer_t command_buffer_create(const Context *context);
  void command_buffer_get(command_buffer_t command_buffer);
  void command_buffer_put(command_buffer_t command_buffer);

  VkCommandBuffer command_buffer_get_handle(command_buffer_t command_buffer);

  void command_buffer_begin(command_buffer_t command_buffer);
  void command_buffer_end(command_buffer_t command_buffer);

  void command_buffer_use(command_buffer_t command_buffer, ref_t resource);
  void command_buffer_reset(command_buffer_t command_buffer);

  // TODO: Improve this API
  void command_buffer_submit(const Context& context, command_buffer_t command_buffer, const Fence& fence);
  void command_buffer_submit(const Context& context, command_buffer_t command_buffer, const Fence& fence,
      Semaphore wait_semaphore, VkPipelineStageFlags wait_stage,
      Semaphore signal_semaphore);
}
