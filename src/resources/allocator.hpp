#pragma once

#include "core/context.hpp"

namespace vulkan
{
  struct Allocator
  {
    VkPhysicalDeviceMemoryProperties memory_properties;
  };

  void init_allocator(const Context& context, Allocator& allocator);
  void deinit_allocator(const Context& context, Allocator& allocator);

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

  void allocate_memory(const Context& context, Allocator& allocator, MemoryAllocationInfo info, MemoryAllocation& allocation);
  void deallocate_memory(const Context& context, Allocator& allocator, MemoryAllocation& allocation);
}
