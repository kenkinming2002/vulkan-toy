#pragma once

#include "allocator.hpp"
#include "command_buffer.hpp"
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

  typedef struct Buffer *buffer_t;

  buffer_t buffer_create(const Context *context, Allocator *allocator, BufferType type, size_t size);
  ref_t buffer_as_ref(buffer_t buffer);

  inline void buffer_get(buffer_t buffer) { ref_get(buffer_as_ref(buffer)); }
  inline void buffer_put(buffer_t buffer) { ref_put(buffer_as_ref(buffer));  }

  VkBuffer buffer_get_handle(buffer_t buffer);

  void buffer_write(command_buffer_t command_buffer, buffer_t buffer, const void *data, size_t size);
}
