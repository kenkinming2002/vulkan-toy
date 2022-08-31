#pragma once

#include "core/context.hpp"
#include "resources/image.hpp"
#include "resources/image_view.hpp"
#include "utils.hpp"

namespace vulkan
{
  typedef struct Swapchain *swapchain_t;

  swapchain_t swapchain_create(context_t context);
  void swapchain_destroy(swapchain_t swapchain);

  VkFormat swapchain_get_format(swapchain_t swapchain);
  VkExtent2D swapchain_get_extent(swapchain_t swapchain);
  uint32_t swapchain_get_image_count(swapchain_t swapchain);

  image_view_t swapchain_get_image_view(swapchain_t swapchain, uint32_t index);

  enum class SwapchainResult
  {
    SUCCESS,
    SUBOPTIMAL,
    OUT_OF_DATE,
  };
  SwapchainResult swapchain_next_image_index(swapchain_t swapchain, VkSemaphore signal_semaphore, uint32_t& image_index);
  SwapchainResult swapchain_present_image_index(swapchain_t swapchain, VkSemaphore wait_semaphore, uint32_t image_index);
}
