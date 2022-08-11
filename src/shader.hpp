#pragma once

#include "context.hpp"
#include "shader_stage.hpp"

#include <vulkan/vulkan.h>

namespace vulkan
{
  struct ShaderLoadInfo
  {
    const char *file_name;
    ShaderStage stage;
  };

  struct Shader
  {
    VkShaderModule handle;
  };

  void load_shader(const Context& context, ShaderLoadInfo load_info, Shader& shader);
  void deinit_shader(const Context& context, Shader& shader);
}
