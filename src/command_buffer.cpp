#include "command_buffer.hpp"

#include "vk_check.hpp"

namespace vulkan
{
  void init_command_buffer(const Context& context, CommandBuffer& command_buffer)
  {
    VkCommandBufferAllocateInfo allocate_info = {};
    allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocate_info.commandPool        = context.command_pool;
    allocate_info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocate_info.commandBufferCount = 1;
    VK_CHECK(vkAllocateCommandBuffers(context.device, &allocate_info, &command_buffer.handle));
  }

  void deinit_command_buffer(const Context& context, CommandBuffer& command_buffer)
  {
    vkFreeCommandBuffers(context.device, context.command_pool, 1, &command_buffer.handle);
    command_buffer.handle = VK_NULL_HANDLE;
  }

  void command_buffer_begin(const CommandBuffer& command_buffer)
  {
    VK_CHECK(vkResetCommandBuffer(command_buffer.handle, 0));

    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    VK_CHECK(vkBeginCommandBuffer(command_buffer.handle, &begin_info));
  }

  void command_buffer_end(const CommandBuffer& command_buffer)
  {
    VK_CHECK(vkEndCommandBuffer(command_buffer.handle));
  }

  void command_buffer_submit(const Context& context, const CommandBuffer& command_buffer, const Fence& fence)
  {
    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers    = &command_buffer.handle;
    VK_CHECK(vkQueueSubmit(context.queue, 1, &submit_info, fence.handle));
  }

  void command_buffer_submit(const Context& context, const CommandBuffer& command_buffer, const Fence& fence,
      Semaphore wait_semaphore, VkPipelineStageFlags wait_stage,
      Semaphore signal_semaphore)
  {
    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores    = &wait_semaphore.handle;
    submit_info.pWaitDstStageMask  = &wait_stage;

    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores    = &signal_semaphore.handle;

    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers    = &command_buffer.handle;

    VK_CHECK(vkQueueSubmit(context.queue, 1, &submit_info, fence.handle));
  }
}
