#pragma once

#include "vulkan.hpp"
#include "context.hpp"
#include "allocator.hpp"
#include "command_buffer.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace vulkan
{
  struct BufferCreateInfo
  {
    VkBufferUsageFlags usage;
    VkMemoryPropertyFlags properties;
    size_t size;
  };
  VkBuffer create_buffer(const Context& context, Allocator& allocator, BufferCreateInfo info, MemoryAllocation& allocation);

  struct Image2dCreateInfo
  {
    VkImageUsageFlags usage;
    VkMemoryPropertyFlags properties;
    VkFormat format;
    size_t width, height;
  };
  VkImage create_image2d(const Context& context, Allocator& allocator, Image2dCreateInfo info, MemoryAllocation& allocation);

  void write_buffer(const Context& context, Allocator& allocator, VkBuffer buffer, MemoryAllocation allocation, const void *data);
  void write_image2d(const Context& context, Allocator& allocator, VkImage image, size_t width, size_t height, MemoryAllocation allocation, const void *data);
}
