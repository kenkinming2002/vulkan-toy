#pragma once

#include "allocator.hpp"
#include "core/command_buffer.hpp"
#include "core/context.hpp"
#include "ref.hpp"

namespace vulkan
{
  REF_DECLARE(Buffer, buffer_t);

  enum class BufferType
  {
    STAGING_BUFFER,
    VERTEX_BUFFER,
    INDEX_BUFFER,
    UNIFORM_BUFFER,
  };

  buffer_t buffer_create(context_t context, allocator_t allocator, BufferType type, size_t size);
  VkBuffer buffer_get_handle(buffer_t buffer);
  void buffer_write(command_buffer_t command_buffer, buffer_t buffer, const void *data, size_t size);
}
