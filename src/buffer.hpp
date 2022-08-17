#pragma once

#include "allocator.hpp"
#include "context.hpp"

namespace vulkan
{
  enum class BufferType
  {
    STAGING_BUFFER,
    VERTEX_BUFFER,
    INDEX_BUFFER,
    UNIFORM_BUFFER,
  };

  struct BufferCreateInfo
  {
    BufferType type;
    VkDeviceSize size;
  };

  struct Buffer
  {
    VkBuffer         handle;
    MemoryAllocation allocation;
  };

  void init_buffer(const Context& context, Allocator& allocator, BufferCreateInfo create_info, Buffer& buffer);
  void deinit_buffer(const Context& context, Allocator& allocator, Buffer& buffer);

  void write_buffer(const Context& context, Allocator& allocator, Buffer& buffer, const void *data, size_t size);
}
