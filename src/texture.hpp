#pragma once

#include "context.hpp"
#include "resources/allocator.hpp"
#include "resources/image.hpp"
#include "resources/image_view.hpp"

namespace vulkan
{
  typedef struct Texture *texture_t;

  texture_t texture_create(const Context *context, Allocator *allocator, ImageType image_type, ImageViewType image_view_type, VkFormat format, size_t width, size_t height);
  ref_t texture_as_ref(texture_t texture);

  inline void texture_get(texture_t texture) { ref_get(texture_as_ref(texture)); }
  inline void texture_put(texture_t texture) { ref_put(texture_as_ref(texture));  }

  image_t texture_get_image(texture_t texture);
  image_view_t texture_get_image_view(texture_t texture);

  void texture_write(command_buffer_t command_buffer, texture_t texture, const void *data, size_t width, size_t height, size_t size);
  texture_t texture_load(command_buffer_t command_buffer, const Context *context, Allocator *allocator, const char *file_name);



  //struct TextureCreateInfo
  //{
  //  const void *data;
  //  size_t width, height;
  //};

  //struct Texture
  //{
  //  image_t      image;
  //  image_view_t image_view;
  //};

  //void texture_init(const Context& context, Allocator& allocator, TextureCreateInfo create_info, Texture& texture);
  //void texture_deinit(const Context& context, Allocator& allocator, Texture& texture);
  //void texture_load(const Context& context, Allocator& allocator, const char *file_name, Texture& texture);
}
