#pragma once

#include "context.hpp"
#include <vulkan/vulkan.h>

namespace vulkan
{
  struct Semaphore
  {
    VkSemaphore handle;
  };

  void init_semaphore(const Context& context, Semaphore& semaphore);
  void deinit_semaphore(const Context& context, Semaphore& semaphore);
}
