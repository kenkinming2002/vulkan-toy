#pragma once

#include "core/context.hpp"
#include "ref.hpp"

namespace vulkan
{
  REF_DECLARE(Shader, shader_t);

  shader_t shader_load(context_t, const char *file_name);
  VkShaderModule shader_get_handle(shader_t shader);
}
