#pragma once

#include "context.hpp"
#include "shader_stage.hpp"

#include <vulkan/vulkan.h>

namespace vulkan
{
  struct ShaderCreateInfo
  {
    const char *file_name;
    ShaderStage stage;
  };

  struct Shader
  {
    VkShaderModule module;
  };

  void load_shader(const Context& context, ShaderCreateInfo create_info, Shader& shader);
  void deinit_shader(const Context& context, Shader& shader);
}
