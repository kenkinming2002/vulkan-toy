#include "vulkan.hpp"

#include <stdio.h>
#include <stdlib.h>

#define VK_CHECK(expr) do { if(expr != VK_SUCCESS) { fprintf(stderr, "Vulkan pooped itself:%s\n", #expr); } } while(0)

namespace vulkan
{
  VkInstance create_instance(
      const char* application_name, uint32_t application_version,
      const char* engine_name, uint32_t engine_version,
      const std::vector<const char*>& extensions,
      const std::vector<const char*>& layers)
  {
    VkApplicationInfo application_info = {};
    application_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    application_info.pApplicationName   = application_name;
    application_info.applicationVersion = application_version;
    application_info.pEngineName        = engine_name;
    application_info.engineVersion      = engine_version;
    application_info.apiVersion         = VK_API_VERSION_1_0;

    VkInstanceCreateInfo instance_create_info = {};
    instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_create_info.pApplicationInfo        = &application_info;
    instance_create_info.enabledExtensionCount   = extensions.size();
    instance_create_info.ppEnabledExtensionNames = extensions.data();
    instance_create_info.enabledLayerCount       = layers.size();
    instance_create_info.ppEnabledLayerNames     = layers.data();

    VkInstance instance = VK_NULL_HANDLE;
    VK_CHECK(vkCreateInstance(&instance_create_info, nullptr, &instance));
    return instance;
  }

  void destroy_instance(VkInstance instance)
  {
    vkDestroyInstance(instance, nullptr);
  }

  std::vector<VkPhysicalDevice> enumerate_physical_devices(VkInstance instance)
  {
    std::vector<VkPhysicalDevice> result;
    {
      uint32_t count;
      vkEnumeratePhysicalDevices(instance, &count, nullptr);
      result.resize(count);
      vkEnumeratePhysicalDevices(instance, &count, result.data());
    }
    return result;
  }

  VkDevice create_device(
      VkPhysicalDevice physical_device,
      const std::vector<uint32_t>& queue_family_indices,
      VkPhysicalDeviceFeatures physical_device_features,
      const std::vector<const char*>& extensions,
      const std::vector<const char*>& layers)
  {
    std::vector<VkDeviceQueueCreateInfo> device_queue_create_infos;

    const float queue_priority = 1.0f;
    for(uint32_t queue_family_index : queue_family_indices)
    {
      VkDeviceQueueCreateInfo device_queue_create_info = {};
      device_queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
      device_queue_create_info.queueFamilyIndex = queue_family_index;
      device_queue_create_info.queueCount = 1;
      device_queue_create_info.pQueuePriorities = &queue_priority;
      device_queue_create_infos.push_back(device_queue_create_info);
    }

    VkDeviceCreateInfo device_create_info = {};
    device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.queueCreateInfoCount = device_queue_create_infos.size();
    device_create_info.pQueueCreateInfos    = device_queue_create_infos.data();
    device_create_info.pEnabledFeatures     = &physical_device_features;
    device_create_info.enabledExtensionCount   = extensions.size();
    device_create_info.ppEnabledExtensionNames = extensions.data();
    device_create_info.enabledLayerCount       = layers.size();
    device_create_info.ppEnabledLayerNames     = layers.data();

    VkDevice device = VK_NULL_HANDLE;
    VK_CHECK(vkCreateDevice(physical_device, &device_create_info, nullptr, &device));
    return device;
  }

  void destroy_device(VkDevice device)
  {
    vkDestroyDevice(device, nullptr);
  }

  VkQueue device_get_queue(VkDevice device, uint32_t queue_family_index, uint32_t queue_index)
  {
    VkQueue queue = VK_NULL_HANDLE;
    vkGetDeviceQueue(device, queue_family_index, queue_index, &queue);
    return queue;
  }

  VkSurfaceKHR create_surface(VkInstance instance, GLFWwindow *window)
  {
    VkSurfaceKHR surface;
    VK_CHECK(glfwCreateWindowSurface(instance, window, nullptr, &surface));
    return surface;
  }

  void destroy_surface(VkInstance instance, VkSurfaceKHR surface)
  {
    vkDestroySurfaceKHR(instance, surface, nullptr);
  }

  VkSurfaceCapabilitiesKHR get_physical_device_surface_capabilities_khr(VkPhysicalDevice physical_device, VkSurfaceKHR surface)
  {
    VkSurfaceCapabilitiesKHR result = {};
    VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &result));
    return result;
  }

  std::vector<VkSurfaceFormatKHR> get_physical_device_surface_formats_khr(VkPhysicalDevice physical_device, VkSurfaceKHR surface)
  {
    std::vector<VkSurfaceFormatKHR> result;
    {
      uint32_t count;
      vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &count, nullptr);
      result.resize(count);
      vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &count, result.data());
    }
    return result;
  }

  std::vector<VkPresentModeKHR> get_physical_device_surface_present_modes_khr(VkPhysicalDevice physical_device, VkSurfaceKHR surface)
  {
    std::vector<VkPresentModeKHR> result;
    {
      uint32_t count;
      vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &count, nullptr);
      result.resize(count);
      vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &count, result.data());
    }
    return result;
  }

  VkSwapchainKHR create_swapchain_khr(VkDevice device, VkSurfaceKHR surface, VkExtent2D extent, uint32_t image_count, VkSurfaceFormatKHR surface_format, VkPresentModeKHR present_mode, VkSurfaceTransformFlagBitsKHR transform)
  {
    VkSwapchainCreateInfoKHR create_info = {};
    create_info.sType                 = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.surface               = surface;
    create_info.imageExtent           = extent;
    create_info.minImageCount         = image_count;
    create_info.imageFormat           = surface_format.format;
    create_info.imageColorSpace       = surface_format.colorSpace;
    create_info.imageArrayLayers      = 1;
    create_info.imageUsage            = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    create_info.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
    create_info.queueFamilyIndexCount = 0;
    create_info.pQueueFamilyIndices   = nullptr;
    create_info.preTransform          = transform;
    create_info.compositeAlpha        = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    create_info.presentMode           = present_mode;
    create_info.clipped               = VK_TRUE;
    create_info.oldSwapchain          = VK_NULL_HANDLE;

    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    VK_CHECK(vkCreateSwapchainKHR(device, &create_info, nullptr, &swapchain));
    return swapchain;
  }

  void destroy_swapchain_khr(VkDevice device, VkSwapchainKHR swapchain)
  {
    vkDestroySwapchainKHR(device, swapchain, nullptr);
  }

  std::vector<VkImage> swapchain_get_images(VkDevice device, VkSwapchainKHR swapchain)
  {
    std::vector<VkImage> result;
    {
      uint32_t count;
      vkGetSwapchainImagesKHR(device, swapchain, &count, nullptr);
      result.resize(count);
      vkGetSwapchainImagesKHR(device, swapchain, &count, result.data());
    }
    return result;
  }

  VkImageView create_image_view(VkDevice device, VkImage image, VkFormat format)
  {
    VkImageViewCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    create_info.image = image;
    create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    create_info.format = format;
    create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    create_info.subresourceRange.baseMipLevel = 0;
    create_info.subresourceRange.levelCount = 1;
    create_info.subresourceRange.baseArrayLayer = 0;
    create_info.subresourceRange.layerCount = 1;

    VkImageView image_view = VK_NULL_HANDLE;
    VK_CHECK(vkCreateImageView(device, &create_info, nullptr, &image_view));
    return image_view;
  }

  void destroy_image_view(VkDevice device, VkImageView image_view)
  {
    vkDestroyImageView(device, image_view, nullptr);
  }
}
