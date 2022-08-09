#pragma once

#include "context.hpp"

namespace vulkan
{
  struct Fence
  {
    VkFence handle;
  };

  void init_fence(const Context& context, Fence& fence, bool signalled);
  void deinit_fence(const Context& context, Fence& fence);
}
