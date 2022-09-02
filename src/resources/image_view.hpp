#pragma once

#include "core/context.hpp"
#include "image.hpp"
#include "ref.hpp"

namespace vulkan
{
  enum class ImageViewType
  {
    COLOR,
    DEPTH,
    STENCIL
  };

  typedef struct ImageView *image_view_t;

  REF_DECLARE(ImageView, image_view_t);

  image_view_t image_view_create(context_t context, ImageViewType type, VkFormat format, size_t mip_levels, image_t image);
  VkImageView image_view_get_handle(image_view_t image_view);
}
