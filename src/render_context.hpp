#pragma once

#include "buffer.hpp"
#include "command_buffer.hpp"
#include "context.hpp"
#include "framebuffer.hpp"
#include "image_view.hpp"
#include "pipeline.hpp"
#include "render_pass.hpp"
#include "shader.hpp"
#include "swapchain.hpp"
#include "vulkan.hpp"
#include <glm/glm.hpp>

#include <algorithm>
#include <fstream>
#include <limits>
#include <optional>

#include <assert.h>
#include <stdio.h>

namespace vulkan
{
  struct RenderContextCreateInfo
  {
    const char* vertex_shader_file_name;
    const char* fragment_shader_file_name;

    VertexInput       vertex_input;
    DescriptorInput   descriptor_input;
    PushConstantInput push_constant_input;

    uint32_t max_frame_in_flight;
  };

  struct Frame
  {
    CommandBuffer command_buffer;
    Fence fence;

    VkSemaphore semaphore_image_available;
    VkSemaphore semaphore_render_finished;
  };

  struct RenderContext
  {
    Swapchain  swapchain;
    RenderPass render_pass;
    Pipeline2  pipeline;

    Image          depth_image;
    ImageView      depth_image_view;
    Framebuffer   *framebuffers;
    uint32_t       image_index;

    Frame *frames;
    uint32_t frame_count;
    uint32_t frame_index;
  };

  void init_render_context(const Context& context, Allocator& allocator, RenderContextCreateInfo create_info, RenderContext& render_context);
  void deinit_render_context(const Context& context, Allocator& allocator, RenderContext& render_context);

  bool begin_render(const Context& context, RenderContext& render_context, Frame& frame);
  bool end_render(const Context& context, RenderContext& render_context, const Frame& frame);
}

