#pragma once

#include "core/context.hpp"

namespace vulkan
{
  REF_DECLARE(Allocator,    allocator_t);
  REF_DECLARE(DeviceMemory, device_memory_t);

  allocator_t allocator_create(context_t context);
  device_memory_t device_memory_allocate(allocator_t allocator, uint32_t type_bits, VkMemoryPropertyFlags memory_properties, VkDeviceSize size);

  VkDeviceMemory device_memory_get_handle(device_memory_t device_memory);

  bool device_memory_mappable(device_memory_t device_memory);
  void *device_memory_map(device_memory_t device_memory);
  void device_memory_unmap(device_memory_t device_memory);
}
