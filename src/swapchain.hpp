#pragma once

#include "core/context.hpp"
#include "core/semaphore.hpp"
#include "resources/image.hpp"
#include "resources/image_view.hpp"
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
    VkImage      *images;
    image_view_t *image_views;
  };

  void init_swapchain(context_t context, Swapchain& swapchain);
  void deinit_swapchain(context_t context, Swapchain& swapchain);

  enum class SwapchainResult
  {
    SUCCESS,
    SUBOPTIMAL,
    OUT_OF_DATE,
  };
  SwapchainResult swapchain_next_image_index(context_t context, const Swapchain& swapchain, VkSemaphore signal_semaphore, uint32_t& image_index);
  SwapchainResult swapchain_present_image_index(context_t context, const Swapchain& swapchain, VkSemaphore wait_semaphore, uint32_t image_index);
}
