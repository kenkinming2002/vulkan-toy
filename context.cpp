#include "context.hpp"

#include <GLFW/glfw3.h>

#include <array>
#include <span>

#include <assert.h>
#include <vulkan/vulkan_core.h>

namespace vulkan
{
  static std::span<const char*> glfw_get_required_instance_extensions()
  {
    uint32_t instance_extension_count;
    const char** instance_extensions;
    instance_extensions = glfwGetRequiredInstanceExtensions(&instance_extension_count);
    return std::span(instance_extensions, instance_extension_count);
  }

  void init_context(ContextCreateInfo create_info, Context& context)
  {
    const auto enabled_layers              = std::array{ "VK_LAYER_KHRONOS_validation" };
    const auto enabled_instance_extensions = glfw_get_required_instance_extensions();
    const auto enabled_device_extensions   = std::array{ VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    // 1: Create instance
    {
      VkApplicationInfo application_info = {};
      application_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
      application_info.pApplicationName   = create_info.application_name;
      application_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
      application_info.pEngineName        = create_info.engine_name;
      application_info.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
      application_info.apiVersion         = VK_API_VERSION_1_0;

      VkInstanceCreateInfo instance_create_info = {};
      instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
      instance_create_info.pApplicationInfo        = &application_info;
      instance_create_info.enabledLayerCount       = enabled_layers.size();
      instance_create_info.ppEnabledLayerNames     = enabled_layers.data();
      instance_create_info.enabledExtensionCount   = enabled_instance_extensions.size();
      instance_create_info.ppEnabledExtensionNames = enabled_instance_extensions.data();

      VK_CHECK(vkCreateInstance(&instance_create_info, nullptr, &context.instance));
    }

    // 2: Create window and surface
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    context.window = glfwCreateWindow(create_info.width, create_info.height, create_info.window_name, nullptr, nullptr);
    VK_CHECK(glfwCreateWindowSurface(context.instance, context.window, nullptr, &context.surface));

    // 3: Select physical device and queue family
    {
      uint32_t count;
      vkEnumeratePhysicalDevices(context.instance, &count, nullptr);
      VkPhysicalDevice *physical_devices = new VkPhysicalDevice[count];
      vkEnumeratePhysicalDevices(context.instance, &count, physical_devices);

      // Select the first for now
      assert(count>=1);
      context.physical_device    = physical_devices[0];
      context.queue_family_index = 0;

      delete[] physical_devices;
    }

    // 4: Create logical device
    {
      const float queue_priority = 1.0f;

      VkDeviceQueueCreateInfo device_queue_create_info = {};
      device_queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
      device_queue_create_info.queueFamilyIndex = context.queue_family_index;
      device_queue_create_info.queueCount       = 1;
      device_queue_create_info.pQueuePriorities = &queue_priority;

      VkPhysicalDeviceFeatures physical_device_features = {};
      physical_device_features.samplerAnisotropy = VK_TRUE;

      VkDeviceCreateInfo device_create_info = {};
      device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
      device_create_info.queueCreateInfoCount    = 1;
      device_create_info.pQueueCreateInfos       = &device_queue_create_info;
      device_create_info.pEnabledFeatures        = &physical_device_features;
      device_create_info.enabledLayerCount       = enabled_layers.size();
      device_create_info.ppEnabledLayerNames     = enabled_layers.data();
      device_create_info.enabledExtensionCount   = enabled_device_extensions.size();
      device_create_info.ppEnabledExtensionNames = enabled_device_extensions.data();
      VK_CHECK(vkCreateDevice(context.physical_device, &device_create_info, nullptr, &context.device));
    }

    // 4: Queue
    vkGetDeviceQueue(context.device, context.queue_family_index, 0, &context.queue);

    // 5: Command pool
    {
      VkCommandPoolCreateInfo command_pool_create_info = {};
      command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
      command_pool_create_info.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
      command_pool_create_info.queueFamilyIndex = context.queue_family_index;
      VK_CHECK(vkCreateCommandPool(context.device, &command_pool_create_info, nullptr, &context.command_pool));
    }
  }

  void deinit_context(Context& context)
  {
    vkDestroyCommandPool(context.device, context.command_pool, nullptr);
    context.command_pool = VK_NULL_HANDLE;

    context.queue = VK_NULL_HANDLE;

    vkDestroyDevice(context.device, nullptr);
    context.device = VK_NULL_HANDLE;

    vkDestroySurfaceKHR(context.instance, context.surface, nullptr);
    context.instance = VK_NULL_HANDLE;

    glfwDestroyWindow(context.window);
    context.window = nullptr;

    vkDestroyInstance(context.instance, nullptr);
    context.instance = VK_NULL_HANDLE;
  }

  bool context_should_destroy(const Context& context)
  {
    return glfwWindowShouldClose(context.window);
  }

  void context_handle_events(const Context& context)
  {
    (void)context;
    glfwPollEvents();
  }
}
