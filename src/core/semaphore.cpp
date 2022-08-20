#include "semaphore.hpp"

#include "vk_check.hpp"

namespace vulkan
{
  void init_semaphore(const Context& context, Semaphore& semaphore)
  {
    VkSemaphoreCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VK_CHECK(vkCreateSemaphore(context.device, &create_info, nullptr, &semaphore.handle));
  }

  void deinit_semaphore(const Context& context, Semaphore& semaphore)
  {
    vkDestroySemaphore(context.device, semaphore.handle, nullptr);
  }
}
