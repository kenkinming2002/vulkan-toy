#pragma once

#include "context.hpp"
#include "resources/image.hpp"

namespace vulkan
{
  enum class ImageViewType
  {
    COLOR,
    DEPTH,
    STENCIL
  };

  struct ImageViewCreateInfo
  {
    ImageViewType type;
    VkFormat      format;
    image_t       image;
  };

  typedef struct ImageView *image_view_t;

  image_view_t image_view_create(const Context *context, ImageViewType type, VkFormat format, image_t image);
  image_view_t image_view_create(const Context *context, ImageViewType type, VkFormat format, VkImage image);
  ref_t image_view_as_ref(image_view_t image_view);

  inline void image_view_get(image_view_t image_view) { ref_get(image_view_as_ref(image_view)); }
  inline void image_view_put(image_view_t image_view) { ref_put(image_view_as_ref(image_view));  }

  VkImageView image_view_get_handle(image_view_t image_view);
}
