#pragma once

#include "context.hpp"

namespace vulkan
{
  struct Shader
  {
    VkShaderModule handle;
  };

  void load_shader(const Context& context, const char *file_name, Shader& shader);
  void deinit_shader(const Context& context, Shader& shader);
}
