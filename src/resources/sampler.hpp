#pragma once

#include "core/context.hpp"
#include "ref.hpp"

namespace vulkan
{
  REF_DECLARE(Sampler, sampler_t);

  sampler_t sampler_create_simple(context_t context);
  VkSampler sampler_get_handle(sampler_t sampler);
}
