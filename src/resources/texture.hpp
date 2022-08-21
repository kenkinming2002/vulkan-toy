#pragma once

#include "allocator.hpp"
#include "core/command_buffer.hpp"
#include "core/context.hpp"
#include "image.hpp"
#include "image_view.hpp"
#include "ref.hpp"

namespace vulkan
{
  typedef struct Texture *texture_t;

  texture_t texture_create(context_t context, allocator_t allocator, ImageType image_type, ImageViewType image_view_type, VkFormat format, size_t width, size_t height);
  ref_t texture_as_ref(texture_t texture);

  inline void texture_get(texture_t texture) { ref_get(texture_as_ref(texture)); }
  inline void texture_put(texture_t texture) { ref_put(texture_as_ref(texture));  }

  image_t texture_get_image(texture_t texture);
  image_view_t texture_get_image_view(texture_t texture);

  void texture_write(command_buffer_t command_buffer, texture_t texture, const void *data, size_t width, size_t height, size_t size);
  texture_t texture_load(command_buffer_t command_buffer, context_t context, allocator_t allocator, const char *file_name);
}
