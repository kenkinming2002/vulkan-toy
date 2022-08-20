#include "command_buffer.hpp"

#include "vk_check.hpp"

#include <assert.h>

namespace vulkan
{
  static constexpr size_t MAX_RESOURCES_COUNT = 16;
  struct CommandBuffer
  {
    Ref ref;

    const Context *context;

    VkCommandBuffer handle;
    size_t resource_count;
    ref_t resources[MAX_RESOURCES_COUNT];
  };

  static void command_buffer_free(ref_t ref)
  {
    command_buffer_t command_buffer = container_of(ref, CommandBuffer, ref);

    vkFreeCommandBuffers(command_buffer->context->device, command_buffer->context->command_pool, 1, &command_buffer->handle);
    for(size_t i=0; i<command_buffer->resource_count; ++i)
      ref_put(command_buffer->resources[i]);
    command_buffer->resource_count = 0;

    delete command_buffer;
  }

  command_buffer_t command_buffer_create(const Context *context)
  {
    command_buffer_t command_buffer = new CommandBuffer{};
    command_buffer->ref.count = 1;
    command_buffer->ref.free  = &command_buffer_free;

    command_buffer->context = context;

    VkCommandBufferAllocateInfo allocate_info = {};
    allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocate_info.commandPool        = command_buffer->context->command_pool;
    allocate_info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocate_info.commandBufferCount = 1;
    VK_CHECK(vkAllocateCommandBuffers(command_buffer->context->device, &allocate_info, &command_buffer->handle));

    return command_buffer;
  }

  void command_buffer_get(command_buffer_t command_buffer)
  {
    ref_get(&command_buffer->ref);
  }

  void command_buffer_put(command_buffer_t command_buffer)
  {
    ref_put(&command_buffer->ref);
  }

  VkCommandBuffer command_buffer_get_handle(command_buffer_t command_buffer)
  {
    return command_buffer->handle;
  }

  void command_buffer_use(command_buffer_t command_buffer, ref_t resource)
  {
    assert(command_buffer->resource_count != MAX_RESOURCES_COUNT);

    ref_get(resource);
    command_buffer->resources[command_buffer->resource_count++] = resource;
  }

  void command_buffer_reset(command_buffer_t command_buffer)
  {
    vkResetCommandBuffer(command_buffer->handle, 0);
    for(size_t i=0; i<command_buffer->resource_count; ++i)
      ref_put(command_buffer->resources[i]);
    command_buffer->resource_count = 0;
  }

  void command_buffer_begin(command_buffer_t command_buffer)
  {
    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    VK_CHECK(vkBeginCommandBuffer(command_buffer->handle, &begin_info));
  }

  void command_buffer_end(command_buffer_t command_buffer)
  {
    VK_CHECK(vkEndCommandBuffer(command_buffer->handle));
  }

  void command_buffer_submit(command_buffer_t command_buffer, const Fence& fence)
  {
    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers    = &command_buffer->handle;
    VK_CHECK(vkQueueSubmit(command_buffer->context->queue, 1, &submit_info, fence.handle));
  }

  void command_buffer_submit(command_buffer_t command_buffer, const Fence& fence,
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
    submit_info.pCommandBuffers    = &command_buffer->handle;

    VK_CHECK(vkQueueSubmit(command_buffer->context->queue, 1, &submit_info, fence.handle));
  }
}
