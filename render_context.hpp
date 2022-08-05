#pragma once

#include "context.hpp"
#include "vulkan.hpp"
#include "buffer.hpp"

#include <glm/glm.hpp>

#include <optional>
#include <algorithm>
#include <fstream>
#include <limits>

#include <stdio.h>
#include <assert.h>
#include <vulkan/vulkan_core.h>

namespace vulkan
{
  // Describe layout of vertex attribute in a single buffer
  struct VertexAttributeDescription
  {
    enum class Type
    {
      FLOAT1, FLOAT2, FLOAT3, FLOAT4 // Which maniac who not use float as vertex input anyway?
    };

    size_t offset;
    Type type;
  };

  // You need multiple binding description if you use multiple buffer
  struct VertexBindingDescription
  {
    size_t stride;
    std::vector<VertexAttributeDescription> attribute_descriptions;
  };

  struct RenderContextCreateInfo
  {
    VkShaderModule vert_shader_module;
    VkShaderModule frag_shader_module;
    std::vector<VertexBindingDescription> vertex_binding_descriptions;

    uint32_t max_frame_in_flight;
  };

  typedef struct RenderContext *render_context_t;

  render_context_t create_render_context(context_t context, allocator_t allocator, RenderContextCreateInfo create_info);
  void destroy_render_context(context_t context, allocator_t allocator, render_context_t render_context);

  struct RenderInfo
  {
    uint32_t      frame_index;
    uint32_t      image_index;

    command_buffer_t command_buffer;
    VkSemaphore semaphore_image_available;
    VkSemaphore semaphore_render_finished;
  };

  std::optional<RenderInfo> begin_render(context_t context, render_context_t render_context);
  bool end_render(context_t context, render_context_t render_context, RenderInfo info);

  VkExtent2D render_context_get_extent(render_context_t render_context);

  VkDescriptorSetLayout render_context_get_descriptor_set_layout(render_context_t render_context);
  VkPipelineLayout render_context_get_pipeline_layout(render_context_t render_context);

  VkRenderPass render_context_get_render_pass(render_context_t render_context);
  VkPipeline render_context_get_pipeline(render_context_t render_context);
}

