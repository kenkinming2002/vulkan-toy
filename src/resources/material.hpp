#pragma once

#include "allocator.hpp"
#include "core/command_buffer.hpp"
#include "core/context.hpp"
#include "image.hpp"
#include "image_view.hpp"
#include "ref.hpp"
#include "sampler.hpp"

#include <stddef.h>

namespace vulkan
{
  struct MaterialLayoutDescription
  {
    // TODO: Support multiple textures and other type of parameters for material
  };

  typedef struct MaterialLayout *material_layout_t;

  material_layout_t material_layout_compile(context_t context, const MaterialLayoutDescription *material_layout_description);
  ref_t material_layout_as_ref(material_layout_t material_layout);

  inline void material_layout_get(material_layout_t material_layout) { ref_get(material_layout_as_ref(material_layout)); }
  inline void material_layout_put(material_layout_t material_layout) { ref_put(material_layout_as_ref(material_layout));  }

  material_layout_t material_layout_create_default(context_t context);

  VkDescriptorSetLayout material_layout_get_descriptor_set_layout(material_layout_t material_layout);

  typedef struct Material *material_t;

  material_t material_create(context_t context, material_layout_t material_layout, image_t image, image_view_t image_view, sampler_t sampler);
  ref_t material_as_ref(material_t material);

  inline void material_get(material_t material) { ref_get(material_as_ref(material)); }
  inline void material_put(material_t material) { ref_put(material_as_ref(material));  }

  material_t material_load(command_buffer_t command_buffer, context_t context, allocator_t allocator, const char *file_name);

  VkDescriptorSet material_get_descriptor_set(material_t material);
}
