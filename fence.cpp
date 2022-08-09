#include "fence.hpp"

#include "vk_check.hpp"
#include <vulkan/vulkan_core.h>

namespace vulkan
{
  void init_fence(const Context& context, Fence& fence, bool signalled)
  {
    VkFenceCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    if(signalled)
      create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VK_CHECK(vkCreateFence(context.device, &create_info, nullptr, &fence.handle));
  }

  void deinit_fence(const Context& context, Fence& fence)
  {
    vkDestroyFence(context.device, fence.handle, nullptr);
    fence.handle = VK_NULL_HANDLE;
  }

  void fence_wait_and_reset(const Context& context, const Fence& fence)
  {
    VK_CHECK(vkWaitForFences(context.device, 1, &fence.handle, VK_TRUE, UINT64_MAX));
    VK_CHECK(vkResetFences(context.device, 1, &fence.handle));
  }
}
