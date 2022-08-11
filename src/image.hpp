#pragma once

#include "allocator.hpp"
#include "context.hpp"
#include "swapchain.hpp"

namespace vulkan
{
  enum class ImageType { COLOR, DEPTH, STENCIL };

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
}
