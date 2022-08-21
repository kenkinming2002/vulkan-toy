#include "frame.hpp"

#include "vk_check.hpp"

namespace vulkan
{
  void frame_init(context_t context, Frame& frame)
  {
    VkDevice device = context_get_device_handle(context);

    frame.command_buffer = command_buffer_create(context, true);

    VkSemaphoreCreateInfo semaphore_create_info = {};
    semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VK_CHECK(vkCreateSemaphore(device, &semaphore_create_info, nullptr, &frame.image_available_semaphore));
    VK_CHECK(vkCreateSemaphore(device, &semaphore_create_info, nullptr, &frame.render_finished_semaphore));
  }

  void frame_deinit(context_t context, Frame& frame)
  {
    VkDevice device = context_get_device_handle(context);

    command_buffer_put(frame.command_buffer);

    vkDestroySemaphore(device, frame.image_available_semaphore, nullptr);
    vkDestroySemaphore(device, frame.render_finished_semaphore, nullptr);
  }
}
