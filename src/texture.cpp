#include "texture.hpp"

#include "stb_image.h"
#include "command_buffer.hpp"

#include <assert.h>

namespace vulkan
{
  void texture_init(const Context& context, Allocator& allocator, TextureCreateInfo create_info, Texture& texture)
  {
    texture.image = image_create(&context, &allocator, ImageType::TEXTURE, VK_FORMAT_R8G8B8A8_SRGB, create_info.width, create_info.height);

    command_buffer_t command_buffer = command_buffer_create(&context);
    command_buffer_begin(command_buffer);
    image_write(command_buffer, texture.image, create_info.data, create_info.width, create_info.height, create_info.width * create_info.height * 4);
    command_buffer_end(command_buffer);

    Fence fence = {};
    init_fence(context, fence, false);
    command_buffer_submit(context, command_buffer, fence);
    fence_wait_and_reset(context, fence);
    deinit_fence(context, fence);

    command_buffer_put(command_buffer);

    vulkan::ImageViewCreateInfo image_view_create_info = {};
    image_view_create_info.type = ImageViewType::COLOR;
    image_view_create_info.format = VK_FORMAT_R8G8B8A8_SRGB;
    image_view_create_info.image  = texture.image;
    init_image_view(context, image_view_create_info, texture.image_view);

  }

  void texture_deinit(const Context& context, Allocator& allocator, Texture& texture)
  {
    (void)allocator;
    deinit_image_view(context, texture.image_view);
    image_put(texture.image);
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
