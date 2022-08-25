#pragma once

#include "ref.hpp"

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

  typedef struct Context *context_t;

  context_t context_create(const char *application_name, const char *engine_name, const char *window_name, unsigned width, unsigned height);
  ref_t context_as_ref(context_t context);

  inline void context_get(context_t context) { ref_get(context_as_ref(context)); }
  inline void context_put(context_t context) { ref_put(context_as_ref(context));  }

  VkSurfaceKHR context_get_surface(context_t context);
  VkPhysicalDevice context_get_physical_device(context_t context);
  VkDevice context_get_device_handle(context_t context);
  VkQueue context_get_queue_handle(context_t context);
  VkCommandPool context_get_default_command_pool(context_t context);

  bool context_should_destroy(context_t context);
  void context_handle_events(context_t context);
}
