#include "semaphore.hpp"

#include "vk_check.hpp"

namespace vulkan
{
  void init_semaphore(context_t context, Semaphore& semaphore)
  {
    VkDevice device = context_get_device_handle(context);

    VkSemaphoreCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VK_CHECK(vkCreateSemaphore(device, &create_info, nullptr, &semaphore.handle));
  }

  void deinit_semaphore(context_t context, Semaphore& semaphore)
  {
    VkDevice device = context_get_device_handle(context);

    vkDestroySemaphore(device, semaphore.handle, nullptr);
  }
}
