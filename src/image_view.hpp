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

  struct ImageView
  {
    VkImageView      handle;
  };

  void init_image_view(const Context& context, ImageViewCreateInfo create_info, ImageView& image_view);
  void deinit_image_view(const Context& context, ImageView& image_view);
}
