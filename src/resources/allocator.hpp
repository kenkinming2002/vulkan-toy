#pragma once

#include "core/context.hpp"

namespace vulkan
{
  struct Allocator
  {
    VkPhysicalDeviceMemoryProperties memory_properties;
  };

  void init_allocator(context_t context, Allocator& allocator);
  void deinit_allocator(context_t context, Allocator& allocator);

  struct MemoryAllocationInfo
  {
    uint32_t              type_bits;
    VkMemoryPropertyFlags properties;
    VkDeviceSize          size;
  };

  struct MemoryAllocation
  {
    VkDeviceMemory memory;
    uint32_t       type_index;
    VkDeviceSize   size;
  };

  void allocate_memory(context_t context, Allocator& allocator, MemoryAllocationInfo info, MemoryAllocation& allocation);
  void deallocate_memory(context_t context, Allocator& allocator, MemoryAllocation& allocation);
}
