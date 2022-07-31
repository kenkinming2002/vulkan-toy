#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include <vector>

#define VK_CHECK(expr) do { if(expr != VK_SUCCESS) { fprintf(stderr, "Vulkan pooped itself:%s\n", #expr); } } while(0)

namespace vulkan
{
  VkInstance create_instance(
      const char* application_name, uint32_t application_version,
      const char* engine_name, uint32_t engine_version,
      const std::vector<const char*>& extensions,
      const std::vector<const char*>& layers);

  void destroy_instance(VkInstance instance);

  std::vector<VkPhysicalDevice> enumerate_physical_devices(VkInstance instance);

  VkDevice create_device(
      VkPhysicalDevice physical_device,
      const std::vector<uint32_t>& queue_family_indices,
      VkPhysicalDeviceFeatures physical_device_features,
      const std::vector<const char*>& extensions,
      const std::vector<const char*>& layers);

  void destroy_device(VkDevice device);

  VkQueue device_get_queue(VkDevice device, uint32_t queue_family_index, uint32_t queue_index);

  VkSurfaceKHR create_surface(VkInstance instance, GLFWwindow *window);
  void destroy_surface(VkInstance instance, VkSurfaceKHR surface);

  VkSurfaceCapabilitiesKHR get_physical_device_surface_capabilities_khr(VkPhysicalDevice physical_device, VkSurfaceKHR surface);
  std::vector<VkSurfaceFormatKHR> get_physical_device_surface_formats_khr(VkPhysicalDevice physical_device, VkSurfaceKHR surface);
  std::vector<VkPresentModeKHR> get_physical_device_surface_present_modes_khr(VkPhysicalDevice physical_device, VkSurfaceKHR surface);

  VkSwapchainKHR create_swapchain_khr(
      VkDevice device, VkSurfaceKHR surface,
      VkExtent2D extent, uint32_t image_count,
      VkSurfaceFormatKHR surface_format, VkPresentModeKHR present_mode,
      VkSurfaceTransformFlagBitsKHR transform);

  void destroy_swapchain_khr(VkDevice device, VkSwapchainKHR swapchain);

  std::vector<VkImage> swapchain_get_images(VkDevice device, VkSwapchainKHR swapchain);
  VkImageView create_image_view(VkDevice device, VkImage image, VkFormat format);
  void destroy_image_view(VkDevice device, VkImageView image_view);

  VkShaderModule create_shader_module(VkDevice device, const std::vector<char>& code);
  void destroy_shader_module(VkDevice device, VkShaderModule shader_module);

  VkPipelineLayout create_empty_pipeline_layout(VkDevice device);
  void destroy_pipeline_layout(VkDevice device, VkPipelineLayout pipeline_layout);

  VkFramebuffer create_framebuffer(VkDevice device, VkRenderPass render_pass, VkImageView attachment, VkExtent2D extent);
  void destroy_framebuffer(VkDevice device, VkFramebuffer framebuffer);

  VkCommandPool create_command_pool(VkDevice device, uint32_t queue_family_index);
  void destroy_command_pool(VkDevice device, VkCommandPool command_pool);

  VkCommandBuffer create_command_buffer(VkDevice device, VkCommandPool command_pool);
  void destroy_command_buffer(VkDevice device, VkCommandPool command_pool, VkCommandBuffer command_buffer);

  VkSemaphore create_semaphore(VkDevice device);
  void destroy_semaphore(VkDevice device, VkSemaphore semaphore);

  VkFence create_fence(VkDevice device, bool signaled);
  void destroy_fence(VkDevice device, VkFence fence);
}
