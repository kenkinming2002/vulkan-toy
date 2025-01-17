#include "swapchain.hpp"

#include "utils.hpp"
#include "vk_check.hpp"

#include <algorithm>
#include <assert.h>
#include <limits>
#include <stdio.h>

namespace vulkan
{
  struct Swapchain
  {
    Ref ref;

    context_t context;

    DelegateChain on_invalidate;

    VkSwapchainKHR handle;

    size_t     texture_count;
    texture_t *textures;

    VkExtent2D extent;
    VkFormat   format;
  };
  REF_DEFINE(Swapchain, swapchain_t, ref);

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

  static void swapchain_init(swapchain_t swapchain)
  {
    uint32_t                      min_image_count;
    uint32_t                      image_count;
    VkExtent2D                    extent;
    VkSurfaceFormatKHR            surface_format;
    VkSurfaceTransformFlagBitsKHR surface_transform;
    VkPresentModeKHR              present_mode;

    VkSurfaceKHR     surface         = context_get_surface(swapchain->context);
    VkPhysicalDevice physical_device = context_get_physical_device(swapchain->context);
    VkDevice         device          = context_get_device_handle(swapchain->context);

    // Capabilities
    {
      VkSurfaceCapabilitiesKHR capabilities = {};
      VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &capabilities));

      // Image count
      min_image_count = capabilities.minImageCount + 1;
      if(capabilities.maxImageCount != 0)
        min_image_count = std::min(min_image_count, capabilities.maxImageCount);

      // Extent
      extent = capabilities.currentExtent;
      if(extent.height == std::numeric_limits<uint32_t>::max() ||
         extent.width  == std::numeric_limits<uint32_t>::max())
      {
        // This should never happen in practice since this means we did not
        // speicify the window size when we create the window. Just use some
        // hard-coded value in this case
        fprintf(stderr, "Window size not specified. Using a default value of 1080x720\n");
        extent = { 1080, 720 };
      }
      extent.width  = std::clamp(extent.width , capabilities.minImageExtent.width , capabilities.maxImageExtent.width);
      extent.height = std::clamp(extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
      surface_transform = capabilities.currentTransform;
    }

    // Surface format
    {
      uint32_t count;
      VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &count, nullptr));
      VkSurfaceFormatKHR *surface_formats = new VkSurfaceFormatKHR[count] {};
      VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &count, surface_formats));
      surface_format = swapchain_select_surface_format(surface_formats, count);
      delete[] surface_formats;
    }

    // Present mode
    {
      uint32_t count;
      VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &count, nullptr));
      VkPresentModeKHR *present_modes = new VkPresentModeKHR[count];
      VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &count, present_modes));
      present_mode = swapchain_select_present_mode(present_modes, count);
      delete[] present_modes;
    }

    // 2: Create Swapchain creation
    {
      VkSwapchainCreateInfoKHR create_info = {};
      create_info.sType                 = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
      create_info.surface               = surface;
      create_info.imageExtent           = extent;
      create_info.minImageCount         = min_image_count;
      create_info.imageFormat           = surface_format.format;
      create_info.imageColorSpace       = surface_format.colorSpace;
      create_info.imageArrayLayers      = 1;
      create_info.imageUsage            = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
      create_info.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
      create_info.queueFamilyIndexCount = 0;
      create_info.pQueueFamilyIndices   = nullptr;
      create_info.preTransform          = surface_transform;
      create_info.compositeAlpha        = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
      create_info.presentMode           = present_mode;
      create_info.clipped               = VK_TRUE;
      create_info.oldSwapchain          = VK_NULL_HANDLE;
      VK_CHECK(vkCreateSwapchainKHR(device, &create_info, nullptr, &swapchain->handle));
    }

    // 3: Retrive images
    vkGetSwapchainImagesKHR(device, swapchain->handle, &image_count, nullptr);
    VkImage *image_handles = new VkImage[image_count];
    vkGetSwapchainImagesKHR(device, swapchain->handle, &image_count, image_handles);

    swapchain->texture_count = image_count;
    swapchain->textures = new texture_t[swapchain->texture_count];
    for(uint32_t i=0; i<image_count; ++i)
      swapchain->textures[i] = present_texture_create(swapchain->context, image_handles[i], surface_format.format, extent.width, extent.height, 1, ImageViewType::COLOR);

    delete[] image_handles;

    // 4: Store info
    swapchain->format      = surface_format.format;
    swapchain->extent      = extent;
  }

  static void swapchain_deinit(swapchain_t swapchain)
  {
    VkDevice device = context_get_device_handle(swapchain->context);
    for(size_t i=0; i<swapchain->texture_count; ++i)
      put(swapchain->textures[i]);
    delete[] swapchain->textures;

    vkDestroySwapchainKHR(device, swapchain->handle, nullptr);
  }

  static void swapchain_invalidate(swapchain_t swapchain)
  {
    VkDevice device = context_get_device_handle(swapchain->context);
    vkDeviceWaitIdle(device);

    swapchain_deinit(swapchain);
    swapchain_init(swapchain);
    delegate_chain_invoke(swapchain->on_invalidate);
  }

  static void swapchain_free(ref_t ref)
  {
    swapchain_t swapchain = container_of(ref, Swapchain, ref);

    swapchain_deinit(swapchain);
    put(swapchain->context);
    delete swapchain;
  }

  swapchain_t swapchain_create(context_t context)
  {
    swapchain_t swapchain = new Swapchain;
    swapchain->ref.count = 1;
    swapchain->ref.free  = swapchain_free;

    delegate_chain_init(swapchain->on_invalidate);

    get(context);
    swapchain->context = context;
    swapchain_init(swapchain);
    return swapchain;
  }

  void swapchain_on_invalidate(swapchain_t swapchain, Delegate& delegate)
  {
    delegate_chain_register(swapchain->on_invalidate, delegate);
  }

  void swapchain_next_image_index(swapchain_t swapchain, VkSemaphore signal_semaphore, uint32_t& image_index)
  {
    VkDevice device = context_get_device_handle(swapchain->context);
    for(;;)
    {
      VkResult result = vkAcquireNextImageKHR(device, swapchain->handle, UINT64_MAX, signal_semaphore, VK_NULL_HANDLE, &image_index);
      if(result != VK_SUBOPTIMAL_KHR && result != VK_ERROR_OUT_OF_DATE_KHR)
      {
        VK_CHECK(result);
        return;
      }
      swapchain_invalidate(swapchain);
    }
  }

  void swapchain_present_image_index(swapchain_t swapchain, VkSemaphore wait_semaphore, uint32_t image_index)
  {
    VkQueue queue = context_get_queue_handle(swapchain->context);

    VkPresentInfoKHR present_info = {};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = &wait_semaphore;
    present_info.swapchainCount = 1;
    present_info.pSwapchains    = &swapchain->handle;
    present_info.pImageIndices  = &image_index;
    present_info.pResults       = nullptr;

    VkResult result = vkQueuePresentKHR(queue, &present_info);
    if(result != VK_SUBOPTIMAL_KHR && result != VK_ERROR_OUT_OF_DATE_KHR)
    {
      VK_CHECK(result);
      return;
    }
    swapchain_invalidate(swapchain);
  }

  VkFormat swapchain_get_format(swapchain_t swapchain) { return swapchain->format; }
  VkExtent2D swapchain_get_extent(swapchain_t swapchain) { return swapchain->extent; }

  uint32_t swapchain_get_texture_count(swapchain_t swapchain) { return swapchain->texture_count; }
  const texture_t *swapchain_get_textures(swapchain_t swapchain) { return swapchain->textures; }

}
