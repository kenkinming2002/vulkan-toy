#pragma once

#include "allocator.hpp"
#include "command_buffer.hpp"
#include "context.hpp"
#include "ref.hpp"

namespace vulkan
{
  enum class ImageType
  {
    TEXTURE,
    COLOR_ATTACHMENT,
    DEPTH_ATTACHMENT,
    STENCIL_ATTACHMENT,
  };

  typedef struct Image *image_t;

  image_t image_create(const Context *context, Allocator *allocator, ImageType type, VkFormat format, size_t width, size_t height);
  ref_t image_as_ref(image_t image);

  inline void image_get(image_t image) { ref_get(image_as_ref(image)); }
  inline void image_put(image_t image) { ref_put(image_as_ref(image));  }

  VkImage image_get_handle(image_t image);

  void image_write(command_buffer_t command_buffer, image_t image, const void *data, size_t width, size_t height, size_t size);
}
