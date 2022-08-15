#pragma once

#include "allocator.hpp"
#include "context.hpp"
#include "swapchain.hpp"

namespace vulkan
{
  enum class ImageType
  {
    TEXTURE,
    COLOR_ATTACHMENT,
    DEPTH_ATTACHMENT,
    STENCIL_ATTACHMENT,
  };

  struct ImageCreateInfo
  {
    ImageType  type;
    VkFormat   format;
    VkExtent2D extent;
  };

  struct Image
  {
    VkImage          handle;
    MemoryAllocation allocation;
  };

  void init_image(const Context& context, Allocator& allocator, ImageCreateInfo create_info, Image& image);
  void deinit_image(const Context& context, Allocator& allocator, Image& image);

  void write_image(const Context& context, Allocator& allocator, Image image, const void *data, size_t width, size_t height, size_t size);
}
