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

  texture_t texture_create(context_t context, image_t image, ImageViewType type)
  {
    texture_t texture = new Texture;
    texture->ref.count = 1;
    texture->ref.free  = texture_free;

    get(image);
    texture->image = image;
    texture->image_view = image_view_create(context, type, image);

    return texture;
  }

  image_t texture_get_image(texture_t texture) { return texture->image; }
  image_view_t texture_get_image_view(texture_t texture) { return texture->image_view; }

  texture_t texture_load(command_buffer_t command_buffer, context_t context, allocator_t allocator, const char *file_name)
  {
    image_t image = image_load(command_buffer, context, allocator, file_name);
    texture_t texture = texture_create(context, image, ImageViewType::COLOR);
    put(image);
    return texture;
  }
}
