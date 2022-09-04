#pragma once

#include "core/context.hpp"
#include "ref.hpp"
#include "resources/texture.hpp"
#include "utils.hpp"

namespace vulkan
{
  REF_DECLARE(Swapchain, swapchain_t);

  swapchain_t swapchain_create(context_t context);

  VkFormat swapchain_get_format(swapchain_t swapchain);
  VkExtent2D swapchain_get_extent(swapchain_t swapchain);

  uint32_t swapchain_get_texture_count(swapchain_t swapchain);
  const texture_t *swapchain_get_textures(swapchain_t swapchain);

  enum class SwapchainResult
  {
    SUCCESS,
    SUBOPTIMAL,
    OUT_OF_DATE,
  };
  SwapchainResult swapchain_next_image_index(swapchain_t swapchain, VkSemaphore signal_semaphore, uint32_t& image_index);
  SwapchainResult swapchain_present_image_index(swapchain_t swapchain, VkSemaphore wait_semaphore, uint32_t image_index);
}
