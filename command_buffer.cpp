#include "command_buffer.hpp"
#include <vulkan/vulkan_core.h>

namespace vulkan
{
  struct CommandBuffer
  {
    VkCommandBuffer handle;
    VkFence fence;
  };

  command_buffer_t create_command_buffer(context_t context, bool async)
  {
    VkDevice      device       = context_get_device(context);
    VkCommandPool command_pool = context_get_command_pool(context);

    command_buffer_t command_buffer = new CommandBuffer{};

    VkCommandBufferAllocateInfo allocate_info = {};
    allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocate_info.commandPool        = command_pool;
    allocate_info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocate_info.commandBufferCount = 1;
    VK_CHECK(vkAllocateCommandBuffers(device, &allocate_info, &command_buffer->handle));

    VkFenceCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    if(async)
      create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VK_CHECK(vkCreateFence(device, &create_info, nullptr, &command_buffer->fence));

    return command_buffer;
  }

  void destroy_command_buffer(context_t context, command_buffer_t command_buffer)
  {
    VkDevice device = context_get_device(context);
    VkCommandPool command_pool = context_get_command_pool(context);

    vkDestroyFence(device, command_buffer->fence, nullptr);
    command_buffer->fence = VK_NULL_HANDLE;

    vkFreeCommandBuffers(device, command_pool, 1, &command_buffer->handle);
    command_buffer->handle = VK_NULL_HANDLE;

    delete command_buffer;
  }

  VkCommandBuffer command_buffer_get_handle(command_buffer_t command_buffer) { return command_buffer->handle; }

  void command_buffer_begin(command_buffer_t command_buffer)
  {
    VK_CHECK(vkResetCommandBuffer(command_buffer->handle, 0));

    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    VK_CHECK(vkBeginCommandBuffer(command_buffer->handle, &begin_info));
  }

  void command_buffer_end(command_buffer_t command_buffer)
  {
    VK_CHECK(vkEndCommandBuffer(command_buffer->handle));
  }

  void command_buffer_wait(context_t context, command_buffer_t command_buffer)
  {
    VkDevice device = context_get_device(context);

    vkWaitForFences(device, 1, &command_buffer->fence, VK_TRUE, UINT64_MAX);
    vkResetFences(device, 1, &command_buffer->fence);
  }

  void command_buffer_submit(context_t context, command_buffer_t command_buffer)
  {
    VkQueue queue = context_get_queue(context);

    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers    = &command_buffer->handle;
    VK_CHECK(vkQueueSubmit(queue, 1, &submit_info, command_buffer->fence));
  }

  void command_buffer_submit(context_t context, command_buffer_t command_buffer,
      VkSemaphore wait_semaphore, VkPipelineStageFlags wait_stage,
      VkSemaphore signal_semaphore)
  {
    VkQueue queue = context_get_queue(context);

    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores    = &wait_semaphore;
    submit_info.pWaitDstStageMask  = &wait_stage;

    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores    = &signal_semaphore;

    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers    = &command_buffer->handle;

    VK_CHECK(vkQueueSubmit(queue, 1, &submit_info, command_buffer->fence));
  }
}
