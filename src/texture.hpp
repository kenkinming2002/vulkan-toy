#pragma once

#include "allocator.hpp"
#include "context.hpp"

#include "image.hpp"
#include "image_view.hpp"

namespace vulkan
{
  struct TextureCreateInfo
  {
    const void *data;
    size_t width, height;
  };

  struct Texture
  {
    Image     image;
    ImageView image_view;
  };

  void texture_init(const Context& context, Allocator& allocator, TextureCreateInfo create_info, Texture& texture);
  void texture_deinit(const Context& context, Allocator& allocator, Texture& texture);
  void texture_load(const Context& context, Allocator& allocator, const char *file_name, Texture& texture);
}
