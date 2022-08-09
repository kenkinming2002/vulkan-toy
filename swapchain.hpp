#pragma once

#include "vulkan.hpp"
#include "context.hpp"

namespace vulkan
{
  struct Swapchain
  {
    uint32_t   image_count;
    VkExtent2D extent;
    VkSurfaceTransformFlagBitsKHR surface_transform;
    VkSurfaceFormatKHR surface_format;
    VkPresentModeKHR   present_mode;

    VkSwapchainKHR handle;
  };

  void init_swapchain(const Context& context, Swapchain& swapchain);
  void deinit_swapchain(const Context& context, Swapchain& swapchain);
}
