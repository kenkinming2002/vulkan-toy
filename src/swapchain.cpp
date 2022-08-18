#include "swapchain.hpp"
#include "image_view.hpp"
#include "utils.hpp"
#include "vk_check.hpp"

#include <algorithm>
#include <assert.h>
#include <limits>
#include <stdio.h>

namespace vulkan
{
  static VkSurfaceFormatKHR swapchain_select_surface_format(VkSurfaceFormatKHR *surface_formats, uint32_t count)
  {
    for(uint32_t i=0; i<count; ++i)
        if(surface_formats[i].format == VK_FORMAT_B8G8R8A8_SRGB && surface_formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
          return surface_formats[i];

    assert(count>=1);
    return surface_formats[0];
  }

  static VkPresentModeKHR swapchain_select_present_mode(VkPresentModeKHR *present_modes, uint32_t count)
  {
    for(uint32_t i=0; i<count; ++i)
      if(present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
        return present_modes[i];

    for(uint32_t i=0; i<count; ++i)
      if(present_modes[i] == VK_PRESENT_MODE_FIFO_KHR)
        return present_modes[i];

    for(uint32_t i=0; i<count; ++i)
      if(present_modes[i] == VK_PRESENT_MODE_FIFO_RELAXED_KHR)
        return present_modes[i];

    for(uint32_t i=0; i<count; ++i)
      if(present_modes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR)
        return present_modes[i];

    assert(count>=1);
    return present_modes[0];
  }

  void init_swapchain(const Context& context, Swapchain& swapchain)
  {
    // Capabilities
    uint32_t min_image_count;
    {
      VkSurfaceCapabilitiesKHR capabilities = {};
      VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(context.physical_device, context.surface, &capabilities));

      // Image count
      min_image_count = capabilities.minImageCount + 1;
      if(capabilities.maxImageCount != 0)
        min_image_count = std::min(min_image_count, capabilities.maxImageCount);

      // Extent
      swapchain.extent = capabilities.currentExtent;
      if(swapchain.extent.height == std::numeric_limits<uint32_t>::max() ||
          swapchain.extent.width  == std::numeric_limits<uint32_t>::max())
      {
        // This should never happen in practice since this means we did not
        // speicify the window size when we create the window. Just use some
        // hard-coded value in this case
        fprintf(stderr, "Window size not specified. Using a default value of 1080x720\n");
        swapchain.extent = { 1080, 720 };
      }
      swapchain.extent.width  = std::clamp(swapchain.extent.width , capabilities.minImageExtent.width , capabilities.maxImageExtent.width);
      swapchain.extent.height = std::clamp(swapchain.extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
      swapchain.surface_transform = capabilities.currentTransform;
    }

    // Surface format
    {
      uint32_t count;
      VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(context.physical_device, context.surface, &count, nullptr));
      VkSurfaceFormatKHR *surface_formats = new VkSurfaceFormatKHR[count] {};
      VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(context.physical_device, context.surface, &count, surface_formats));
      swapchain.surface_format = swapchain_select_surface_format(surface_formats, count);
      delete[] surface_formats;
    }

    // Present mode
    {
      uint32_t count;
      VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(context.physical_device, context.surface, &count, nullptr));
      VkPresentModeKHR *present_modes = new VkPresentModeKHR[count];
      VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(context.physical_device, context.surface, &count, present_modes));
      swapchain.present_mode = swapchain_select_present_mode(present_modes, count);
      delete[] present_modes;
    }

    // 2: Create Swapchain creation
    {
      VkSwapchainCreateInfoKHR create_info = {};
      create_info.sType                 = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
      create_info.surface               = context.surface;
      create_info.imageExtent           = swapchain.extent;
      create_info.minImageCount         = min_image_count;
      create_info.imageFormat           = swapchain.surface_format.format;
      create_info.imageColorSpace       = swapchain.surface_format.colorSpace;
      create_info.imageArrayLayers      = 1;
      create_info.imageUsage            = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
      create_info.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
      create_info.queueFamilyIndexCount = 0;
      create_info.pQueueFamilyIndices   = nullptr;
      create_info.preTransform          = swapchain.surface_transform;
      create_info.compositeAlpha        = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
      create_info.presentMode           = swapchain.present_mode;
      create_info.clipped               = VK_TRUE;
      create_info.oldSwapchain          = VK_NULL_HANDLE;
      VK_CHECK(vkCreateSwapchainKHR(context.device, &create_info, nullptr, &swapchain.handle));
    }

    // 3: Retrive images
    uint32_t image_count;
    vkGetSwapchainImagesKHR(context.device, swapchain.handle, &image_count, nullptr);
    VkImage *images = new VkImage[image_count];
    vkGetSwapchainImagesKHR(context.device, swapchain.handle, &image_count, images);

    swapchain.image_count = image_count;
    swapchain.images      = new Image[image_count];
    swapchain.image_views = new ImageView[image_count];
    for(uint32_t i=0; i<image_count; ++i)
    {
      swapchain.images[i] = Image{.handle = images[i], .allocation = {}};
      init_image_view(context, ImageViewCreateInfo{
        .type = ImageViewType::COLOR,
        .format = swapchain.surface_format.format,
        .image = swapchain.images[i],
      }, swapchain.image_views[i]);
    }

    delete[] images;
  }

  void deinit_swapchain(const Context& context, Swapchain& swapchain)
  {
    delete[] swapchain.images;
    swapchain.images = nullptr;

    for(uint32_t i=0; i<swapchain.image_count; ++i)
      deinit_image_view(context, swapchain.image_views[i]);

    swapchain.image_count = 0;

    delete[] swapchain.image_views;
    swapchain.image_views = nullptr;

    vkDestroySwapchainKHR(context.device, swapchain.handle, nullptr);
    swapchain.handle = nullptr;
    swapchain = {};
  }

  SwapchainResult swapchain_next_image_index(const Context& context, const Swapchain& swapchain, VkSemaphore signal_semaphore, uint32_t& image_index)
  {
    VkResult result = vkAcquireNextImageKHR(context.device, swapchain.handle, UINT64_MAX, signal_semaphore, VK_NULL_HANDLE, &image_index);
    switch(result)
    {
    default:
      VK_CHECK(result);
      return SwapchainResult::SUCCESS;
    case VK_SUBOPTIMAL_KHR:
      return SwapchainResult::SUBOPTIMAL;
    case VK_ERROR_OUT_OF_DATE_KHR:
      return SwapchainResult::OUT_OF_DATE;
    }
  }

  SwapchainResult swapchain_present_image_index(const Context& context, const Swapchain& swapchain, VkSemaphore wait_semaphore, uint32_t image_index)
  {
    VkPresentInfoKHR present_info = {};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = &wait_semaphore;
    present_info.swapchainCount = 1;
    present_info.pSwapchains    = &swapchain.handle;
    present_info.pImageIndices  = &image_index;
    present_info.pResults       = nullptr;

    VkResult result = vkQueuePresentKHR(context.queue, &present_info);
    switch(result)
    {
    default:
      VK_CHECK(result);
      return SwapchainResult::SUCCESS;
    case VK_SUBOPTIMAL_KHR:
      return SwapchainResult::SUBOPTIMAL;
    case VK_ERROR_OUT_OF_DATE_KHR:
      return SwapchainResult::OUT_OF_DATE;
    }
  }
}
