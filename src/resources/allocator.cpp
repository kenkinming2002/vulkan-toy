#include "allocator.hpp"

#include "vk_check.hpp"

#include <stdio.h>
#include <stdlib.h>

namespace vulkan
{
  void init_allocator(context_t context, Allocator& allocator)
  {
    VkPhysicalDevice physical_device = context_get_physical_device(context);
    vkGetPhysicalDeviceMemoryProperties(physical_device, &allocator.memory_properties);
  }

  void deinit_allocator(context_t context, Allocator& allocator)
  {
    (void)context;
    allocator = {};
  }

  template<typename T>
  static bool is_bits_set(T value, T flags)
  {
    return (value & flags) == flags;
  }

  void allocate_memory(context_t context, Allocator& allocator, MemoryAllocationInfo info, MemoryAllocation& allocation)
  {
    VkDevice device = context_get_device_handle(context);

    for(uint32_t i = 0; i < allocator.memory_properties.memoryTypeCount; i++)
    {
      if(!is_bits_set<uint32_t>(info.type_bits, 1 << i))
        continue;

      VkMemoryType memory_type = allocator.memory_properties.memoryTypes[i];
      if(!is_bits_set(memory_type.propertyFlags, info.properties))
        continue;

      VkMemoryAllocateInfo allocate_info = {};
      allocate_info.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
      allocate_info.memoryTypeIndex = i;
      allocate_info.allocationSize  = info.size;
      VK_CHECK(vkAllocateMemory(device, &allocate_info, nullptr, &allocation.memory));

      allocation.type_index = i;
      allocation.size       = info.size;
      return;
    }

    fprintf(stderr, "No memory type while allocating device memory");
    abort();
  }

  void deallocate_memory(context_t context, Allocator& allocator, MemoryAllocation& allocation)
  {
    VkDevice device = context_get_device_handle(context);

    (void)allocator;
    vkFreeMemory(device, allocation.memory, nullptr);
  }
}
