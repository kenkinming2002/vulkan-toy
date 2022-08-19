#pragma once

#include "allocator.hpp"
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

  struct ImageCreateInfo
  {
    ImageType  type;
    VkFormat   format;
    VkExtent2D extent;
  };

  typedef struct Image *image_t;

  image_t image_create(const Context *context, Allocator *allocator, ImageType type, VkFormat format, size_t width, size_t height);
  void image_get(image_t image);
  void image_put(image_t image);

  VkImage image_get_handle(image_t image);

  void image_write(VkCommandBuffer command_buffer, image_t image, const void *data, size_t width, size_t height, size_t size);
}
