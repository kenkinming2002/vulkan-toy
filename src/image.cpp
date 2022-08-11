#include "image.hpp"

#include "allocator.hpp"
#include "buffer.hpp"
#include "vk_check.hpp"

namespace vulkan
{
  void init_image(const Context& context, Allocator& allocator, ImageCreateInfo create_info, Image& image)
  {
    image = {};

    Image2dCreateInfo image_create_info = {};
    switch(create_info.type)
    {
      case ImageType::COLOR:
        image_create_info.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        break;
      case ImageType::DEPTH:
      case ImageType::STENCIL:
        image_create_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        break;
    }
    image_create_info.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    image_create_info.format     = create_info.format;
    image_create_info.width      = create_info.extent.width;
    image_create_info.height     = create_info.extent.height;
    image.handle = create_image2d(context, allocator, image_create_info, image.allocation);
  }

  void deinit_image(const Context& context, Allocator& allocator, Image& image)
  {
    vkDestroyImage(context.device, image.handle, nullptr);
    deallocate_memory(context, allocator, image.allocation);
  }
}
