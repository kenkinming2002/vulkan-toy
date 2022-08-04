#pragma once

#include "vulkan.hpp"

namespace glfw
{
  inline std::vector<const char*> get_required_instance_extensions()
  {
    uint32_t instance_extension_count;
    const char** instance_extensions;
    instance_extensions = glfwGetRequiredInstanceExtensions(&instance_extension_count);
    return std::vector<const char*>(&instance_extensions[0], &instance_extensions[instance_extension_count]);
  }
}

namespace vulkan
{
  // General vulkan context of a single window
  struct Context
  {
    VkInstance       instance;

    VkPhysicalDevice physical_device;
    uint32_t         queue_family_index;

    VkDevice device;
    VkQueue  queue;

    VkCommandPool   command_pool;
    VkSurfaceKHR   surface;
  };

  struct ContextCreateInfo
  {
    const char *application_name;
    uint32_t    application_version;

    const char *engine_name;
    uint32_t    engine_version;

    GLFWwindow *window;
  };

  inline Context create_context(ContextCreateInfo create_info)
  {
    auto required_layers              = std::vector<const char*>{"VK_LAYER_KHRONOS_validation"};
    auto required_instance_extensions = glfw::get_required_instance_extensions();
    auto required_device_extensions   = std::vector<const char*>{VK_KHR_SWAPCHAIN_EXTENSION_NAME};

    Context context = {};
    context.instance = vulkan::create_instance(
        "Vulkan", VK_MAKE_VERSION(1, 0, 0),
        "Engine", VK_MAKE_VERSION(1, 0, 0),
        required_instance_extensions, required_layers
    );

    context.surface = vulkan::create_surface(context.instance, create_info.window);
    context.physical_device    = vulkan::enumerate_physical_devices(context.instance).at(0);
    context.queue_family_index = 0;

    VkPhysicalDeviceFeatures physical_device_features = {};
    physical_device_features.samplerAnisotropy = VK_TRUE;
    context.device = vulkan::create_device(
        context.physical_device, { context.queue_family_index }, physical_device_features,
        required_device_extensions, required_layers
    );
    context.queue = vulkan::device_get_queue(context.device, context.queue_family_index, 0);

    context.command_pool   = vulkan::create_command_pool(context.device, 0);
    return context;
  }
}
