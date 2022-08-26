#pragma once

#include "allocator.hpp"
#include "core/command_buffer.hpp"
#include "core/context.hpp"
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

  REF_DECLARE(Image, image_t);

  image_t image_create(context_t context, allocator_t allocator, ImageType type, VkFormat format, size_t width, size_t height, size_t mip_levels);
  VkImage image_get_handle(image_t image);
  void image_write(command_buffer_t command_buffer, image_t image, const void *data, size_t width, size_t height, size_t size);
}
