#include "texture.hpp"

#include "stb_image.h"

#include <algorithm>
#include <bit>

#include <assert.h>

namespace vulkan
{
  struct Texture
  {
    Ref ref;

    image_t      image;
    image_view_t image_view;
  };
  REF_DEFINE(Texture, texture_t, ref);

  static void texture_free(ref_t ref)
  {
    texture_t texture = container_of(ref, Texture, ref);
    put(texture->image);
    put(texture->image_view);
    delete texture;
  }

  texture_t texture_create(context_t context, allocator_t allocator,
      ImageType image_type,
      VkFormat format, size_t width, size_t height, size_t mip_levels,
      ImageViewType image_view_type)
  {
    texture_t texture = new Texture;
    texture->ref.count = 1;
    texture->ref.free  = texture_free;

    texture->image      = image_create(context, allocator, image_type, format, width, height, mip_levels);
    texture->image_view = image_view_create(context, image_view_type, texture->image);

    return texture;
  }

  texture_t present_texture_create(context_t context,
      VkImage handle,
      VkFormat format, size_t width, size_t height, size_t mip_levels,
      ImageViewType image_view_type)
  {
    texture_t texture = new Texture;
    texture->ref.count = 1;
    texture->ref.free  = texture_free;

    texture->image      = present_image_create(context, handle, format, width, height, mip_levels);
    texture->image_view = image_view_create(context, image_view_type, texture->image);

    return texture;
  }

  image_t texture_get_image(texture_t texture) { return texture->image; }
  image_view_t texture_get_image_view(texture_t texture) { return texture->image_view; }

  texture_t texture_load(command_buffer_t command_buffer, context_t context, allocator_t allocator, const char *file_name)
  {
    int x, y, n;
    unsigned char *data = stbi_load(file_name, &x, &y, &n, STBI_rgb_alpha);

    assert(data);
    assert(x>=0 && y>=0);

    unsigned width = x, height = y;
    size_t mip_levels = std::bit_width(std::bit_floor(std::max(width, height)));
    texture_t texture = texture_create(context, allocator, ImageType::TEXTURE, VK_FORMAT_R8G8B8A8_SRGB, width, height, mip_levels, ImageViewType::COLOR);

    image_write(command_buffer, texture->image, data, width, height, width * height * 4);

    stbi_image_free(data);

    return texture;
  }
}
