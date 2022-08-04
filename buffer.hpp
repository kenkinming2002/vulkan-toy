#pragma once

#include "vulkan.hpp"
#include "context.hpp"
#include "command_buffer.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define VK_CHECK(expr) do { if(expr != VK_SUCCESS) { fprintf(stderr, "Vulkan pooped itself:%s\n", #expr); } } while(0)

namespace vulkan
{
  struct Allocator
  {
    VkPhysicalDeviceMemoryProperties memory_properties;
  };

  Allocator create_allocator(const Context& context);
  void destroy_allocator(const Allocator& allocator);

  struct BufferAllocation
  {
    VkBuffer buffer;

    VkDeviceMemory memory;
    VkDeviceSize size;
    VkMemoryPropertyFlags memory_properties;
  };

  BufferAllocation allocate_buffer(const vulkan::Context& context, Allocator& allocator, VkDeviceSize size, VkBufferUsageFlags buffer_usage, VkMemoryPropertyFlags memory_properties);
  void deallocate_buffer(const Context& context, const Allocator& allocator, BufferAllocation allocation);

  void write_buffer(const Context& context, Allocator& allocator, BufferAllocation& allocation, const void *data);
}
