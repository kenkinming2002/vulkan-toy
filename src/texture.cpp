#include "texture.hpp"

#include "stb_image.h"

#include <assert.h>

namespace vulkan
{
  void texture_init(const Context& context, Allocator& allocator, TextureCreateInfo create_info, Texture& texture)
  {
    vulkan::ImageCreateInfo image_create_info = {};
    image_create_info.type          = vulkan::ImageType::TEXTURE;
    image_create_info.format        = VK_FORMAT_R8G8B8A8_SRGB;
    image_create_info.extent.width  = create_info.width;
    image_create_info.extent.height = create_info.height;
    vulkan::init_image(context, allocator, image_create_info, texture.image);
    vulkan::write_image(context, allocator, texture.image, create_info.data, create_info.width, create_info.height, create_info.width * create_info.height * 4);

    vulkan::ImageViewCreateInfo image_view_create_info = {};
    image_view_create_info.type = ImageViewType::COLOR;
    image_view_create_info.format = VK_FORMAT_R8G8B8A8_SRGB;
    image_view_create_info.image  = texture.image;
    init_image_view(context, image_view_create_info, texture.image_view);
  }

  void texture_deinit(const Context& context, Allocator& allocator, Texture& texture)
  {
    deinit_image_view(context, texture.image_view);
    deinit_image(context, allocator, texture.image);
  }

  void texture_load(const Context& context, Allocator& allocator, const char *file_name, Texture& texture)
  {
    int width, height, n;
    unsigned char *data = stbi_load(file_name, &width, &height, &n, STBI_rgb_alpha);
    assert(data);

    TextureCreateInfo create_info = {};
    create_info.data = data;
    create_info.width = width;
    create_info.height = height;
    texture_init(context, allocator, create_info, texture);

    stbi_image_free(data);
  }
}
