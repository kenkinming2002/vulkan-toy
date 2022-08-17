#pragma once

#include "context.hpp"
#include "image.hpp"
#include "image_view.hpp"
#include "semaphore.hpp"
#include "utils.hpp"
#include "vulkan.hpp"

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
    Image     *images;
    ImageView *image_views;
  };

  void init_swapchain(const Context& context, Swapchain& swapchain);
  void deinit_swapchain(const Context& context, Swapchain& swapchain);

  enum class SwapchainResult
  {
    SUCCESS,
    SUBOPTIMAL,
    OUT_OF_DATE,
  };
  SwapchainResult swapchain_next_image_index(const Context& context, const Swapchain& swapchain, Semaphore signal_semaphore, uint32_t& image_index);
  SwapchainResult swapchain_present_image_index(const Context& context, const Swapchain& swapchain, Semaphore wait_semaphore, uint32_t image_index);
}
