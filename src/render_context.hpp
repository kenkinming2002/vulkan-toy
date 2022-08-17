#pragma once

#include "buffer.hpp"
#include "command_buffer.hpp"
#include "context.hpp"
#include "framebuffer.hpp"
#include "image_view.hpp"
#include "pipeline.hpp"
#include "render_pass.hpp"
#include "semaphore.hpp"
#include "shader.hpp"
#include "swapchain.hpp"

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
  };

  struct Frame
  {
    CommandBuffer command_buffer;
    Fence fence;
    Semaphore semaphore_image_available;
    Semaphore semaphore_render_finished;
  };

  static constexpr size_t MAX_FRAME_IN_FLIGHT = 2;
  struct RenderContext
  {
    Swapchain  swapchain;
    RenderPass render_pass;
    Pipeline2  pipeline;

    Image          depth_image;
    ImageView      depth_image_view;
    Framebuffer   *framebuffers;
    uint32_t       image_index;

    Frame frames[MAX_FRAME_IN_FLIGHT];
    size_t frame_index;
  };

  void init_render_context(const Context& context, Allocator& allocator, RenderContextCreateInfo create_info, RenderContext& render_context);
  void deinit_render_context(const Context& context, Allocator& allocator, RenderContext& render_context);

  bool begin_render(const Context& context, RenderContext& render_context, Frame& frame);
  bool end_render(const Context& context, RenderContext& render_context, const Frame& frame);
}

