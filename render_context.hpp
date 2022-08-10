#pragma once

#include "attachment.hpp"
#include "buffer.hpp"
#include "command_buffer.hpp"
#include "context.hpp"
#include "framebuffer.hpp"
#include "pipeline.hpp"
#include "pipeline_layout.hpp"
#include "render_pass.hpp"
#include "shader.hpp"
#include "swapchain.hpp"
#include "vulkan.hpp"

#include <glm/glm.hpp>

#include <algorithm>
#include <fstream>
#include <limits>
#include <optional>

#include <stdio.h>
#include <assert.h>

namespace vulkan
{
  struct ImageResource
  {
    SwapchainAttachment color_attachment;
    ManagedAttachment   depth_attachment;
    Framebuffer         framebuffer;
  };

  struct FrameResource
  {
    CommandBuffer command_buffer;
    Fence fence;

    VkSemaphore semaphore_image_available;
    VkSemaphore semaphore_render_finished;
  };

  struct RenderContextCreateInfo
  {
    Shader vertex_shader;
    Shader fragment_shader;

    VertexInput vertex_input;

    uint32_t max_frame_in_flight;
  };

  struct RenderContext
  {
    Swapchain      swapchain;
    PipelineLayout pipeline_layout;

    // TODO: How to support multiple render pass
    RenderPass render_pass;
    Pipeline   pipeline;

    ImageResource *image_resources;
    FrameResource *frame_resources;

    uint32_t frame_count;
    uint32_t frame_index;
  };

  void init_render_context(const Context& context, Allocator& allocator, RenderContextCreateInfo create_info, RenderContext& render_context);
  void deinit_render_context(const Context& context, Allocator& allocator, RenderContext& render_context);

  struct RenderInfo
  {
    uint32_t      frame_index;
    uint32_t      image_index;

    CommandBuffer command_buffer;
    VkSemaphore semaphore_image_available;
    VkSemaphore semaphore_render_finished;
  };

  std::optional<RenderInfo> begin_render(const Context& context, RenderContext& render_context);
  bool end_render(const Context& context, RenderContext& render_context, RenderInfo info);
}

