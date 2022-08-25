#pragma once

#include "context.hpp"
#include "resources/ref.hpp"


namespace vulkan
{
  typedef struct CommandBuffer *command_buffer_t;

  command_buffer_t command_buffer_create(context_t context, bool initial_state_pending = false);
  void command_buffer_get(command_buffer_t command_buffer);
  void command_buffer_put(command_buffer_t command_buffer);

  VkCommandBuffer command_buffer_get_handle(command_buffer_t command_buffer);

  void command_buffer_use(command_buffer_t command_buffer, ref_t resource);

  void command_buffer_begin(command_buffer_t command_buffer);
  void command_buffer_end(command_buffer_t command_buffer);
  void command_buffer_submit(command_buffer_t command_buffer);
  void command_buffer_submit(command_buffer_t command_buffer, VkSemaphore wait_semaphore, VkPipelineStageFlags wait_stage, VkSemaphore signal_semaphore);
  void command_buffer_wait(command_buffer_t command_buffer);
  void command_buffer_reset(command_buffer_t command_buffer);
}
