#pragma once

#include "image.hpp"
#include "context.hpp"

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
    Image         image;
  };

  struct ImageView
  {
    VkImageView      handle;
  };

  void init_image_view(const Context& context, ImageViewCreateInfo create_info, ImageView& image_view);
  void deinit_image_view(const Context& context, ImageView& image_view);
}
