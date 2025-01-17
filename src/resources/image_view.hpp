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

  // TODO: May be we do not need to take context in image_view_create
  image_view_t image_view_create(context_t context, ImageViewType type, image_t image);
  VkImageView image_view_get_handle(image_view_t image_view);
}
