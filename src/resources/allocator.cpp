#include "allocator.hpp"

#include "vk_check.hpp"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <vulkan/vulkan_core.h>

namespace vulkan
{
  struct Allocator
  {
    Ref ref;

    context_t context;
    VkPhysicalDeviceMemoryProperties memory_properties;
  };

  struct DeviceMemory
  {
    Ref ref;

    context_t context;

    VkDeviceMemory handle;

    VkMemoryPropertyFlags properties;
    VkDeviceSize          size;
  };

  ref_t allocator_as_ref(allocator_t allocator)
  {
    return &allocator->ref;
  }

  ref_t device_memory_as_ref(device_memory_t device_memory)
  {
    return &device_memory->ref;
  }

  VkDeviceMemory device_memory_get_handle(device_memory_t device_memory)
  {
    return device_memory->handle;
  }

  static void allocator_free(ref_t ref)
  {
    allocator_t allocator = container_of(ref, Allocator, ref);
    context_put(allocator->context);
    delete allocator;
  }

  allocator_t allocator_create(context_t context)
  {
    allocator_t allocator = new Allocator;
    allocator->ref.count = 1;
    allocator->ref.free  = allocator_free;

    context_get(context);
    allocator->context = context;

    VkPhysicalDevice physical_device = context_get_physical_device(allocator->context);
    vkGetPhysicalDeviceMemoryProperties(physical_device, &allocator->memory_properties);

    return allocator;
  }

  static void device_memory_free(ref_t ref)
  {
    device_memory_t device_memory = container_of(ref, DeviceMemory, ref);

    VkDevice device = context_get_device_handle(device_memory->context);
    vkFreeMemory(device, device_memory->handle, nullptr);
    context_put(device_memory->context);

    delete device_memory;
  }

  template<typename T> static inline bool is_bits_set(T value, T flags) { return (value & flags) == flags; }
  device_memory_t device_memory_allocate(allocator_t allocator, uint32_t type_bits, VkMemoryPropertyFlags memory_properties, VkDeviceSize size)
  {
    device_memory_t device_memory = new DeviceMemory;
    device_memory->ref.count = 1;
    device_memory->ref.free  = device_memory_free;

    context_get(allocator->context);
    device_memory->context = allocator->context;

    VkDevice device = context_get_device_handle(device_memory->context);
    for(uint32_t type_index = 0; type_index < allocator->memory_properties.memoryTypeCount; ++type_index)
    {
      if(!is_bits_set<uint32_t>(type_bits, 1 << type_index))
        continue;

      VkMemoryType memory_type = allocator->memory_properties.memoryTypes[type_index];
      if(!is_bits_set(memory_type.propertyFlags, memory_properties))
        continue;

      VkMemoryAllocateInfo allocate_info = {};
      allocate_info.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
      allocate_info.memoryTypeIndex = type_index;
      allocate_info.allocationSize  = size;
      VK_CHECK(vkAllocateMemory(device, &allocate_info, nullptr, &device_memory->handle));

      device_memory->properties = memory_type.propertyFlags;
      device_memory->size       = size;
      return device_memory;
    }

    fprintf(stderr, "No memory type while allocating device memory");
    abort();
  }

  bool device_memory_mappable(device_memory_t device_memory)
  {
    return device_memory->properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
  }

  void *device_memory_map(device_memory_t device_memory)
  {
    assert(device_memory_mappable(device_memory));

    VkDevice device = context_get_device_handle(device_memory->context);
    void *data;
    vkMapMemory(device, device_memory->handle, 0, VK_WHOLE_SIZE, 0, &data);
    return data;
  }

  void device_memory_unmap(device_memory_t device_memory)
  {
    VkDevice device = context_get_device_handle(device_memory->context);
    vkUnmapMemory(device, device_memory->handle);
  }
}
