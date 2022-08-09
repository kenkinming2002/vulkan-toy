#pragma once

#include "vulkan.hpp"
#include "context.hpp"
#include "command_buffer.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace vulkan
{
  typedef struct Allocator *allocator_t;

  allocator_t create_allocator(const Context& context);
  void destroy_allocator(const Context& context, allocator_t allocator);

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

  MemoryAllocation allocate_memory(const Context& context, allocator_t allocator, MemoryAllocationInfo info);
  void deallocate_memory(const Context& context, allocator_t allocator, MemoryAllocation allocation);

  struct BufferCreateInfo
  {
    VkBufferUsageFlags usage;
    VkMemoryPropertyFlags properties;
    size_t size;
  };
  VkBuffer create_buffer(const Context& context, allocator_t allocator, BufferCreateInfo info, MemoryAllocation& allocation);

  struct Image2dCreateInfo
  {
    VkImageUsageFlags usage;
    VkMemoryPropertyFlags properties;
    VkFormat format;
    size_t width, height;
  };
  VkImage create_image2d(const Context& context, allocator_t allocator, Image2dCreateInfo info, MemoryAllocation& allocation);

  void write_buffer(const Context& context, allocator_t allocator, VkBuffer buffer, MemoryAllocation allocation, const void *data);
  void write_image2d(const Context& context, allocator_t allocator, VkImage image, size_t width, size_t height, MemoryAllocation allocation, const void *data);
}
