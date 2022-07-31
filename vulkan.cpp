#include <vulkan/vulkan_core.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vulkan/vulkan.h>

#include <utility>
#include <limits>
#include <algorithm>
#include <optional>
#include <vector>
#include <iostream>

#include <assert.h>
#include <stdlib.h>

#define VK_CHECK(expr) do { if(expr != VK_SUCCESS) { fprintf(stderr, "Vulkan pooped itself:%s\n", #expr); } } while(0)

namespace glfw
{
  std::vector<const char*> get_required_instance_extensions()
  {
    uint32_t instance_extension_count;
    const char** instance_extensions;
    instance_extensions = glfwGetRequiredInstanceExtensions(&instance_extension_count);
    return std::vector<const char*>(&instance_extensions[0], &instance_extensions[instance_extension_count]);
  }
}

namespace vulkan
{
  class Instance
  {
  public:
    Instance(const char* application_name, uint32_t application_version, const char* engine_name, uint32_t engine_version,
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

      VK_CHECK(vkCreateInstance(&instance_create_info, nullptr, &handle));
    }

    ~Instance()
    {
      vkDestroyInstance(handle, nullptr);
    }

  public:
    VkInstance handle;
  };

  std::vector<VkPhysicalDevice> enumerate_physical_devices(const Instance& instance)
  {
    std::vector<VkPhysicalDevice> result;
    {
      uint32_t count;
      vkEnumeratePhysicalDevices(instance.handle, &count, nullptr);
      result.resize(count);
      vkEnumeratePhysicalDevices(instance.handle, &count, result.data());
    }
    return result;
  }

  struct Device
  {
  public:
    Device(VkPhysicalDevice physical_device, const std::vector<uint32_t>& queue_family_indices, VkPhysicalDeviceFeatures physical_device_features,
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

      VK_CHECK(vkCreateDevice(physical_device, &device_create_info, nullptr, &handle));
    }

    VkQueue get_queue(uint32_t queue_family_index, uint32_t queue_index)
    {
      VkQueue queue = VK_NULL_HANDLE;
      vkGetDeviceQueue(handle, queue_family_index, queue_index, &queue);
      return queue;
    }

    ~Device()
    {
      vkDestroyDevice(handle, nullptr);
    }

  public:
    VkDevice handle;
  };

  struct Surface
  {
  public:
    Surface(const Instance& instance, GLFWwindow *window)
      : instance(instance)
    {
      VK_CHECK(glfwCreateWindowSurface(instance.handle, window, nullptr, &handle));
    }

    ~Surface()
    {
      vkDestroySurfaceKHR(instance.handle, handle, nullptr);
    }

  public:
    const Instance& instance;
    VkSurfaceKHR handle;
  };

  VkSurfaceCapabilitiesKHR get_physical_device_surface_capabilities_khr(VkPhysicalDevice physical_device, const Surface& surface)
  {
    VkSurfaceCapabilitiesKHR result = {};
    VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface.handle, &result));
    return result;
  }

  std::vector<VkSurfaceFormatKHR> get_physical_device_surface_formats_khr(VkPhysicalDevice physical_device, const Surface& surface)
  {
    std::vector<VkSurfaceFormatKHR> result;
    {
      uint32_t count;
      vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface.handle, &count, nullptr);
      result.resize(count);
      vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface.handle, &count, result.data());
    }
    return result;
  }

  std::vector<VkPresentModeKHR> get_physical_device_surface_present_modes_khr(VkPhysicalDevice physical_device, const Surface& surface)
  {
    std::vector<VkPresentModeKHR> result;
    {
      uint32_t count;
      vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface.handle, &count, nullptr);
      result.resize(count);
      vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface.handle, &count, result.data());
    }
    return result;
  }

  struct Swapchain
  {
  public:
    Swapchain(const Device& device, const Surface& surface, VkExtent2D extent, uint32_t image_count, VkSurfaceFormatKHR surface_format, VkPresentModeKHR present_mode, VkSurfaceTransformFlagBitsKHR transform)
      : device(device)
    {
      VkSwapchainCreateInfoKHR create_info = {};
      create_info.sType                 = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
      create_info.surface               = surface.handle;
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
      VK_CHECK(vkCreateSwapchainKHR(device.handle, &create_info, nullptr, &handle));
    }

    ~Swapchain()
    {
      vkDestroySwapchainKHR(device.handle, handle, nullptr);
    }

    std::vector<VkImage> images()
    {
      std::vector<VkImage> result;
      {
        uint32_t count;
        vkGetSwapchainImagesKHR(device.handle, handle, &count, nullptr);
        result.resize(count);
        vkGetSwapchainImagesKHR(device.handle, handle, &count, result.data());
      }
      return result;
    }

  public:
    const Device& device;
    VkSwapchainKHR handle;
  };

  struct ImageView
  {
  public:
    ImageView() : device(nullptr), handle(VK_NULL_HANDLE) {}

  public:
    ImageView(ImageView&& other)
    {
      device = std::exchange(other.device, nullptr);
      handle = std::exchange(other.handle, VK_NULL_HANDLE);
    }

    ImageView& operator=(ImageView&& other)
    {
      std::swap(device, other.device);
      std::swap(handle, other.handle);
      return *this;
    }

  public:
    ImageView(const Device& device, VkImage image, VkFormat format)
      : device(&device)
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

      VK_CHECK(vkCreateImageView(device.handle, &create_info, nullptr, &handle));
    }

    ~ImageView()
    {
      if(device && handle != VK_NULL_HANDLE)
        vkDestroyImageView(device->handle, handle, nullptr);
    }

  public:
    const Device* device;
    VkImageView handle;
  };
}

template<typename T>
T select(const std::vector<T>& choices, const auto& get_score)
{
  using score_t = std::invoke_result_t<decltype(get_score), T>;

  struct Selection
  {
    score_t score;
    T choice;
  };
  std::optional<Selection> best_selection;
  for(const auto& choice : choices)
  {
    Selection selection = {
      .score  = get_score(choice),
      .choice = choice
    };
    if(!best_selection || selection.score > best_selection->score)
      best_selection = selection;
  }
  assert(best_selection && "No choice given");
  return best_selection->choice;
}

VkExtent2D select_swap_extent(const VkSurfaceCapabilitiesKHR& surface_capabilities, GLFWwindow *window)
{
  if(surface_capabilities.currentExtent.height != std::numeric_limits<uint32_t>::max() &&
     surface_capabilities.currentExtent.width  != std::numeric_limits<uint32_t>::max())
    return surface_capabilities.currentExtent;

  int width, height;
  glfwGetFramebufferSize(window, &width, &height);

  VkExtent2D extent = {};
  extent.width  = width;
  extent.height = height;
  extent.width  = std::clamp(extent.width , surface_capabilities.minImageExtent.width , surface_capabilities.maxImageExtent.width );
  extent.height = std::clamp(extent.height, surface_capabilities.minImageExtent.height, surface_capabilities.maxImageExtent.height);
  return extent;
}

uint32_t select_image_count(const VkSurfaceCapabilitiesKHR& surface_capabilities)
{
  uint32_t image_count = surface_capabilities.minImageCount + 1;
  if(surface_capabilities.maxImageCount != 0)
    image_count = std::min(image_count, surface_capabilities.maxImageCount);
  return image_count;
}

VkSurfaceFormatKHR select_surface_format_khr(const std::vector<VkSurfaceFormatKHR>& surface_formats)
{
  return select(surface_formats, [](const VkSurfaceFormatKHR& surface_format)
  {
    if(surface_format.format == VK_FORMAT_B8G8R8A8_SRGB && surface_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
      return 1;

    return 0;
  });
}

VkPresentModeKHR select_present_mode_khr(const std::vector<VkPresentModeKHR>& present_modes)
{
  return select(present_modes, [](const VkPresentModeKHR& present_mode)
  {
    switch(present_mode)
    {
    case VK_PRESENT_MODE_IMMEDIATE_KHR:    return 0;
    case VK_PRESENT_MODE_FIFO_RELAXED_KHR: return 1;
    case VK_PRESENT_MODE_FIFO_KHR:         return 2;
    case VK_PRESENT_MODE_MAILBOX_KHR:      return 3;
    default: return -1;
    }
  });
}

int main()
{
  glfwInit();

  // Window and KHR API
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
  GLFWwindow *window = glfwCreateWindow(800, 600, "Vulkan", nullptr, nullptr);

  // Vulkan
  auto required_layers              = std::vector<const char*>{"VK_LAYER_KHRONOS_validation"};
  auto required_instance_extensions = glfw::get_required_instance_extensions();
  auto required_device_extensions   = std::vector<const char*>{VK_KHR_SWAPCHAIN_EXTENSION_NAME};

  auto instance = vulkan::Instance(
      "Vulkan", VK_MAKE_VERSION(1, 0, 0),
      "Engine", VK_MAKE_VERSION(1, 0, 0),
      required_instance_extensions, required_layers
  );

  auto surface = vulkan::Surface(instance, window);

  // We want some better way to select them, this is the most annoying part of vulkan
  auto physical_device = vulkan::enumerate_physical_devices(instance).at(0);
  auto queue_family_indices = std::vector<uint32_t>{0};

  auto device = vulkan::Device(
      physical_device, queue_family_indices, {},
      required_device_extensions, required_layers
  );
  auto queue = device.get_queue(0, 0);

  auto capabilities  = vulkan::get_physical_device_surface_capabilities_khr(physical_device, surface);
  auto formats       = vulkan::get_physical_device_surface_formats_khr(physical_device, surface);
  auto present_modes = vulkan::get_physical_device_surface_present_modes_khr(physical_device, surface);

  auto extent       = select_swap_extent(capabilities, window);
  auto image_count  = select_image_count(capabilities);
  auto format       = select_surface_format_khr(formats);
  auto present_mode = select_present_mode_khr(present_modes);

  auto swapchain = vulkan::Swapchain(device, surface, extent, image_count, format, present_mode, capabilities.currentTransform);
  auto images      = swapchain.images();
  auto image_views = std::vector<vulkan::ImageView>();
  for(const auto& image : images)
    image_views.emplace_back(device,image, format.format);

  while(!glfwWindowShouldClose(window))
    glfwPollEvents();

  glfwDestroyWindow(window);
  glfwTerminate();
}
