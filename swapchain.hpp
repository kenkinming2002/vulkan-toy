#pragma once

#include "vulkan.hpp"
#include "context.hpp"
#include "utils.hpp"

namespace vulkan
{
  struct Swapchain
  {
    VkExtent2D extent;
    VkSurfaceTransformFlagBitsKHR surface_transform;
    VkSurfaceFormatKHR surface_format;
    VkPresentModeKHR   present_mode;

    VkSwapchainKHR handle;

    uint32_t  image_count;
    VkImage  *images;
  };

  void init_swapchain(const Context& context, Swapchain& swapchain);
  void deinit_swapchain(const Context& context, Swapchain& swapchain);
}
