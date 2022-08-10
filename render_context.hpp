#pragma once

#include "context.hpp"
#include "vulkan.hpp"
#include "shader.hpp"
#include "buffer.hpp"
#include "pipeline.hpp"

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

  struct RenderContextCreateInfo
  {
    Shader vertex_shader;
    Shader fragment_shader;

    VertexInput    vertex_input;

    uint32_t max_frame_in_flight;
  };

  typedef struct RenderContext *render_context_t;

  render_context_t create_render_context(const Context& context, allocator_t allocator, RenderContextCreateInfo create_info);
  void destroy_render_context(const Context& context, allocator_t allocator, render_context_t render_context);

  struct RenderInfo
  {
    uint32_t      frame_index;
    uint32_t      image_index;

    CommandBuffer command_buffer;
    VkSemaphore semaphore_image_available;
    VkSemaphore semaphore_render_finished;
  };

  std::optional<RenderInfo> begin_render(const Context& context, render_context_t render_context);
  bool end_render(const Context& context, render_context_t render_context, RenderInfo info);

  VkExtent2D render_context_get_extent(render_context_t render_context);

  VkDescriptorSetLayout render_context_get_descriptor_set_layout(render_context_t render_context);
  VkPipelineLayout render_context_get_pipeline_layout(render_context_t render_context);

  VkRenderPass render_context_get_render_pass(render_context_t render_context);
  VkPipeline render_context_get_pipeline(render_context_t render_context);
}

