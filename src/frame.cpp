#include "frame.hpp"

#include "vk_check.hpp"

namespace vulkan
{
  void frame_init(const Context& context, Frame& frame)
  {
    frame.command_buffer = command_buffer_create(&context);

    VkFenceCreateInfo fence_create_info = {};
    fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    VK_CHECK(vkCreateFence(context.device, &fence_create_info, nullptr, &frame.fence));

    VkSemaphoreCreateInfo semaphore_create_info = {};
    semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VK_CHECK(vkCreateSemaphore(context.device, &semaphore_create_info, nullptr, &frame.image_available_semaphore));
    VK_CHECK(vkCreateSemaphore(context.device, &semaphore_create_info, nullptr, &frame.render_finished_semaphore));
  }

  void frame_deinit(const Context& context, Frame& frame)
  {
    command_buffer_put(frame.command_buffer);

    vkDestroyFence(context.device, frame.fence, nullptr);
    vkDestroySemaphore(context.device, frame.image_available_semaphore, nullptr);
    vkDestroySemaphore(context.device, frame.render_finished_semaphore, nullptr);
  }
}
