#pragma once

#include "shader_stage.hpp"

namespace vulkan
{
  enum class DescriptorType { UNIFORM_BUFFER, SAMPLER };

  struct DescriptorInfo
  {
    DescriptorType type;
    ShaderStage stage;
  };

}
