#pragma once

#include <vulkan/vulkan.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace vulkan
{
  struct ContextCreateInfo
  {
    const char *application_name;
    const char *engine_name;
    const char *window_name;
    unsigned width, height;
  };

  struct Context
  {
    VkInstance instance;

    GLFWwindow*  window;
    VkSurfaceKHR surface;

    VkPhysicalDevice physical_device;
    uint32_t         queue_family_index;

    VkDevice device;
    VkQueue  queue;

    VkCommandPool   command_pool;
  };

  // TODO: Error code
  void init_context(ContextCreateInfo create_info, Context& context);
  void deinit_context(Context& context);

  bool context_should_destroy(const Context& context);
  void context_handle_events(const Context& context);
}
