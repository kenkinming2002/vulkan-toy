#pragma once

#include "vulkan.hpp"
#include "context.hpp"

namespace vulkan
{
  typedef struct CommandBuffer *command_buffer_t;

  command_buffer_t create_command_buffer(const Context& context, bool async);
  void destroy_command_buffer(const Context& context, command_buffer_t command_buffer);

  VkCommandBuffer command_buffer_get_handle(command_buffer_t command_buffer);

  void command_buffer_begin(command_buffer_t command_buffer);
  void command_buffer_end(command_buffer_t command_buffer);

  void command_buffer_wait(const Context& context, command_buffer_t command_buffer);

  void command_buffer_submit(const Context& context, command_buffer_t command_buffer);
  void command_buffer_submit(const Context& context, command_buffer_t command_buffer,
      VkSemaphore wait_semaphore, VkPipelineStageFlags wait_stage,
      VkSemaphore signal_semaphore);
}
