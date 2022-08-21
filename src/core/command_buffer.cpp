#include "command_buffer.hpp"

#include "vk_check.hpp"

#include <assert.h>

namespace vulkan
{
  static constexpr size_t MAX_RESOURCES_COUNT = 16;

  enum class CommandBufferState
  {
    INITIAL,
    RECORDING,
    EXECUTABLE,
    PENDING,
    INVALID,
  };


  struct CommandBuffer
  {
    Ref ref;

    context_t context;

    CommandBufferState state;

    size_t resource_count;
    ref_t resources[MAX_RESOURCES_COUNT];

    VkCommandBuffer handle;
    VkFence         fence;
  };

  static void command_buffer_free(ref_t ref)
  {
    command_buffer_t command_buffer = container_of(ref, CommandBuffer, ref);

    VkDevice      device       = context_get_device_handle(command_buffer->context);
    VkCommandPool command_pool = context_get_default_command_pool(command_buffer->context);

    for(size_t i=0; i<command_buffer->resource_count; ++i)
      ref_put(command_buffer->resources[i]);
    command_buffer->resource_count = 0;

    vkFreeCommandBuffers(device, command_pool, 1, &command_buffer->handle);
    vkDestroyFence(device, command_buffer->fence, nullptr);

    context_put(command_buffer->context);

    delete command_buffer;
  }

  command_buffer_t command_buffer_create(context_t context, bool initial_state_pending)
  {
    command_buffer_t command_buffer = new CommandBuffer{};
    command_buffer->ref.count = 1;
    command_buffer->ref.free  = &command_buffer_free;

    context_get(context);
    command_buffer->context = context;

    VkDevice      device       = context_get_device_handle(command_buffer->context);
    VkCommandPool command_pool = context_get_default_command_pool(command_buffer->context);

    command_buffer->state          = initial_state_pending ? CommandBufferState::PENDING : CommandBufferState::INITIAL;
    command_buffer->resource_count = 0;

    VkCommandBufferAllocateInfo command_buffer_allocate_info = {};
    command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_allocate_info.commandPool        = command_pool;
    command_buffer_allocate_info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    command_buffer_allocate_info.commandBufferCount = 1;
    VK_CHECK(vkAllocateCommandBuffers(device, &command_buffer_allocate_info, &command_buffer->handle));

    VkFenceCreateInfo fence_create_info = {};
    fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_create_info.flags = initial_state_pending ? VK_FENCE_CREATE_SIGNALED_BIT : 0;
    VK_CHECK(vkCreateFence(device, &fence_create_info, nullptr, &command_buffer->fence));

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
    assert(command_buffer->state == CommandBufferState::RECORDING);
    assert(command_buffer->resource_count != MAX_RESOURCES_COUNT);
    ref_get(resource);
    command_buffer->resources[command_buffer->resource_count++] = resource;
  }

  void command_buffer_begin(command_buffer_t command_buffer)
  {
    assert(command_buffer->state == CommandBufferState::INITIAL);
    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    VK_CHECK(vkBeginCommandBuffer(command_buffer->handle, &begin_info));
    command_buffer->state = CommandBufferState::RECORDING;
  }

  void command_buffer_end(command_buffer_t command_buffer)
  {
    assert(command_buffer->state == CommandBufferState::RECORDING);
    VK_CHECK(vkEndCommandBuffer(command_buffer->handle));
    command_buffer->state = CommandBufferState::EXECUTABLE;
  }

  void command_buffer_submit(command_buffer_t command_buffer)
  {
    VkQueue queue = context_get_queue_handle(command_buffer->context);

    assert(command_buffer->state == CommandBufferState::EXECUTABLE);
    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers    = &command_buffer->handle;
    VK_CHECK(vkQueueSubmit(queue, 1, &submit_info, command_buffer->fence));
    command_buffer->state = CommandBufferState::PENDING;
  }

  void command_buffer_submit(command_buffer_t command_buffer, VkSemaphore wait_semaphore, VkPipelineStageFlags wait_stage, VkSemaphore signal_semaphore)
  {
    VkQueue queue = context_get_queue_handle(command_buffer->context);

    assert(command_buffer->state == CommandBufferState::EXECUTABLE);
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
    command_buffer->state = CommandBufferState::PENDING;
  }

  void command_buffer_wait(command_buffer_t command_buffer)
  {
    VkDevice device = context_get_device_handle(command_buffer->context);

    assert(command_buffer->state == CommandBufferState::PENDING);
    VK_CHECK(vkWaitForFences(device, 1, &command_buffer->fence, VK_TRUE, UINT64_MAX));
    VK_CHECK(vkResetFences(device, 1, &command_buffer->fence));
    command_buffer->state = CommandBufferState::EXECUTABLE;
  }

  void command_buffer_reset(command_buffer_t command_buffer)
  {
    assert(command_buffer->state != CommandBufferState::PENDING);
    vkResetCommandBuffer(command_buffer->handle, 0);
    for(size_t i=0; i<command_buffer->resource_count; ++i)
      ref_put(command_buffer->resources[i]);
    command_buffer->resource_count = 0;
    command_buffer->state = CommandBufferState::INITIAL;
  }
}
