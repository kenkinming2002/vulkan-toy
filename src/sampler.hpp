#pragma once

#include "context.hpp"

namespace vulkan
{
  struct Sampler
  {
    VkSampler handle;
  };

  void init_sampler_simple(const Context& context, Sampler& sampler);
  void deinit_sampler(const Context& context, Sampler& sampler);
}
