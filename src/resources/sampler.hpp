#pragma once

#include "core/context.hpp"
#include "ref.hpp"

namespace vulkan
{
  typedef struct Sampler *sampler_t;

  sampler_t sampler_create_simple(const Context *context);
  ref_t sampler_as_ref(sampler_t sampler);

  inline void sampler_get(sampler_t sampler) { ref_get(sampler_as_ref(sampler)); }
  inline void sampler_put(sampler_t sampler) { ref_put(sampler_as_ref(sampler));  }

  VkSampler sampler_get_handle(sampler_t sampler);
}
