#include "loader.hpp"

#include "stb_image.h"

#include <assert.h>

namespace vulkan
{
  void load_image(const Context& context, Allocator& allocator, Image& image, const char *file_name)
  {
    int width, height, n;
    unsigned char *data = stbi_load(file_name, &width, &height, &n, STBI_rgb_alpha);
    assert(data);

    vulkan::ImageCreateInfo image_create_info = {};
    image_create_info.type          = vulkan::ImageType::TEXTURE;
    image_create_info.format        = VK_FORMAT_R8G8B8A8_SRGB;
    image_create_info.extent.width  = width;
    image_create_info.extent.height = height;
    vulkan::init_image(context, allocator, image_create_info, image);
    vulkan::write_image(context, allocator, image, data, width, height, width * height * 4);

    stbi_image_free(data);
  }
}
