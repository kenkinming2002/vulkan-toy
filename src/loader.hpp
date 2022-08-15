#pragma once

#include "allocator.hpp"
#include "context.hpp"
#include "image.hpp"

namespace vulkan
{
  void load_image(const Context& context, Allocator& allocator, Image& image, const char *file_name);
}
