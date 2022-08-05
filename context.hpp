#pragma once

#include "vulkan.hpp"

namespace vulkan
{
  //  A vulkan context include both a window and a all the related context
  typedef struct Context *context_t;

  struct ContextCreateInfo
  {
    const char *application_name;
    const char *engine_name;
    const char *window_name;
    unsigned width, height;
  };

  context_t create_context(ContextCreateInfo create_info);
  void destroy_context(context_t context);

  VkSurfaceKHR context_get_surface(context_t context);

  VkPhysicalDevice context_get_physical_device(context_t context);
  uint32_t context_get_queue_family_index(context_t context);

  VkDevice context_get_device(context_t context);

  VkQueue context_get_queue(context_t context);
  VkCommandPool context_get_command_pool(context_t context);

  bool context_should_destroy(context_t context);
  void context_handle_events(context_t context);
}
