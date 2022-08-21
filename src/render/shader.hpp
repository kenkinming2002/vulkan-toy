#pragma once

#include "core/context.hpp"

namespace vulkan
{
  struct Shader
  {
    VkShaderModule handle;
  };

  void load_shader(context_t context, const char *file_name, Shader& shader);
  void deinit_shader(context_t context, Shader& shader);
}
