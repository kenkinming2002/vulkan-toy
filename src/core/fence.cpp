#include "fence.hpp"

#include "vk_check.hpp"
#include <vulkan/vulkan_core.h>

namespace vulkan
{
  void init_fence(context_t context, Fence& fence, bool signalled)
  {
    VkDevice device = context_get_device_handle(context);

    VkFenceCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    if(signalled)
      create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VK_CHECK(vkCreateFence(device, &create_info, nullptr, &fence.handle));
  }

  void deinit_fence(context_t context, Fence& fence)
  {
    VkDevice device = context_get_device_handle(context);

    vkDestroyFence(device, fence.handle, nullptr);
    fence.handle = VK_NULL_HANDLE;
  }

  void fence_wait_and_reset(context_t context, const Fence& fence)
  {
    VkDevice device = context_get_device_handle(context);

    VK_CHECK(vkWaitForFences(device, 1, &fence.handle, VK_TRUE, UINT64_MAX));
    VK_CHECK(vkResetFences(device, 1, &fence.handle));
  }
}
