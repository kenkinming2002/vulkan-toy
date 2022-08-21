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

  static void texture_free(ref_t ref)
  {
    texture_t texture = container_of(ref, Texture, ref);
    image_put(texture->image);
    image_view_put(texture->image_view);
    delete texture;
  }

  texture_t texture_create(context_t context, allocator_t allocator, ImageType image_type, ImageViewType image_view_type, VkFormat format, size_t width, size_t height)
  {
    texture_t texture = new Texture {};
    texture->ref.count = 1;
    texture->ref.free  = texture_free;

    size_t mip_level = std::bit_width(std::bit_floor(std::max(width, height)));

    texture->image      = image_create(context, allocator, image_type, format, width, height, mip_level);
    texture->image_view = image_view_create(context, image_view_type, format, mip_level, texture->image);

    return texture;
  }

  ref_t texture_as_ref(texture_t texture)
  {
    return &texture->ref;
  }

  image_t texture_get_image(texture_t texture)
  {
    return texture->image;
  }

  image_view_t texture_get_image_view(texture_t texture)
  {
    return texture->image_view;
  }

  void texture_write(command_buffer_t command_buffer, texture_t texture, const void *data, size_t width, size_t height, size_t size)
  {
    image_write(command_buffer, texture->image, data, width, height, size);
  }

  texture_t texture_load(command_buffer_t command_buffer, context_t context, allocator_t allocator, const char *file_name)
  {
    int width, height, n;
    unsigned char *data = stbi_load(file_name, &width, &height, &n, STBI_rgb_alpha);
    assert(data);

    texture_t texture = texture_create(context, allocator, ImageType::TEXTURE, ImageViewType::COLOR, VK_FORMAT_R8G8B8A8_SRGB, width, height);
    texture_write(command_buffer, texture, data, width, height, width * height * 4);

    stbi_image_free(data);

    return texture;
  }
}
