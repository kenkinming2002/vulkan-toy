#pragma once

#include "core/context.hpp"

namespace vulkan
{
  typedef struct Allocator    *allocator_t;
  typedef struct DeviceMemory *device_memory_t;

  ref_t allocator_as_ref(allocator_t allocator);

  inline void allocator_get(allocator_t allocator) { ref_get(allocator_as_ref(allocator)); }
  inline void allocator_put(allocator_t allocator) { ref_put(allocator_as_ref(allocator));  }

  ref_t device_memory_as_ref(device_memory_t device_memory);

  inline void device_memory_get(device_memory_t device_memory) { ref_get(device_memory_as_ref(device_memory)); }
  inline void device_memory_put(device_memory_t device_memory) { ref_put(device_memory_as_ref(device_memory));  }

  VkDeviceMemory device_memory_get_handle(device_memory_t device_memory);

  allocator_t allocator_create(context_t context);
  device_memory_t device_memory_allocate(allocator_t allocator, uint32_t type_bits, VkMemoryPropertyFlags memory_properties, VkDeviceSize size);

  bool device_memory_mappable(device_memory_t device_memory);
  void *device_memory_map(device_memory_t device_memory);
  void device_memory_unmap(device_memory_t device_memory);
}
