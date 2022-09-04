#pragma once

#include "ref.hpp"
#include "image.hpp"
#include "image_view.hpp"

namespace vulkan
{
  REF_DECLARE(Texture, texture_t);

  texture_t texture_create(context_t context, image_t image, ImageViewType type);

  image_t texture_get_image(texture_t texture);
  image_view_t texture_get_image_view(texture_t texture);

  texture_t texture_load(command_buffer_t command_buffer, context_t context, allocator_t allocator, const char *file_name);
}
