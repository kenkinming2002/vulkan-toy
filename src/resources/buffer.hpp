#pragma once

#include "allocator.hpp"
#include "context.hpp"
#include "ref.hpp"

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

  typedef struct Buffer *buffer_t;

  buffer_t buffer_create(const Context *context, Allocator *allocator, BufferType type, size_t size);
  void buffer_get(buffer_t buffer);
  void buffer_put(buffer_t buffer);

  VkBuffer buffer_get_handle(buffer_t buffer);

  void buffer_copy(VkCommandBuffer command_buffer, buffer_t src, buffer_t dst, size_t size);
  void buffer_write(VkCommandBuffer command_buffer, buffer_t buffer, const void *data, size_t size);
}
