#include "vulkan.hpp"

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

template<typename T>
struct Defer
{
  Defer(T func) : func(func) {}
  ~Defer() { func(); }
  T func;
};

#define VK_CHECK(expr) do { if(expr != VK_SUCCESS) { fprintf(stderr, "Vulkan pooped itself:%s\n", #expr); } } while(0)

#define DEFER__(expr, counter) Defer _defer##counter([&](){ expr; })
#define DEFER_(expr, counter) DEFER__(expr, counter)
#define DEFER(expr) DEFER_(expr, __COUNTER__)

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

  auto instance = vulkan::create_instance(
      "Vulkan", VK_MAKE_VERSION(1, 0, 0),
      "Engine", VK_MAKE_VERSION(1, 0, 0),
      required_instance_extensions, required_layers
  );
  DEFER(vulkan::destroy_instance(instance));

  auto surface = vulkan::create_surface(instance, window);
  DEFER(vulkan::destroy_surface(instance, surface));

  // We want some better way to select them, this is the most annoying part of vulkan
  auto physical_device = vulkan::enumerate_physical_devices(instance).at(0);
  auto queue_family_indices = std::vector<uint32_t>{0};

  auto device = vulkan::create_device(
      physical_device, queue_family_indices, {},
      required_device_extensions, required_layers
  );
  DEFER(vulkan::destroy_device(device));

  auto queue = vulkan::device_get_queue(device, 0, 0);

  auto capabilities  = vulkan::get_physical_device_surface_capabilities_khr(physical_device, surface);
  auto formats       = vulkan::get_physical_device_surface_formats_khr(physical_device, surface);
  auto present_modes = vulkan::get_physical_device_surface_present_modes_khr(physical_device, surface);

  auto extent       = select_swap_extent(capabilities, window);
  auto image_count  = select_image_count(capabilities);
  auto format       = select_surface_format_khr(formats);
  auto present_mode = select_present_mode_khr(present_modes);

  auto swapchain = vulkan::create_swapchain_khr(device, surface, extent, image_count, format, present_mode, capabilities.currentTransform);
  DEFER(vulkan::destroy_swapchain_khr(device, swapchain));

  auto images = vulkan::swapchain_get_images(device, swapchain);

  auto image_views = std::vector<VkImageView>();
  for(const auto& image : images) image_views.push_back(vulkan::create_image_view(device, image, format.format));
  DEFER(for(const auto& image_view : image_views) { vulkan::destroy_image_view(device, image_view); });

  while(!glfwWindowShouldClose(window))
    glfwPollEvents();

  glfwDestroyWindow(window);
  glfwTerminate();
}
