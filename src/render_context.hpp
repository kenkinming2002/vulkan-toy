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
  // Double buffering
  static constexpr size_t MAX_FRAME_IN_FLIGHT = 2;

  struct Frame
  {
    CommandBuffer command_buffer;
    Fence fence;
    Semaphore semaphore_image_available;
    Semaphore semaphore_render_finished;
  };

  struct RenderTarget
  {
    Swapchain swapchain;

    RenderPass   render_pass;
    Image        depth_image;
    ImageView    depth_image_view;
    Framebuffer *framebuffers;

    uint32_t image_index;
    uint32_t image_count;

    Frame frames[MAX_FRAME_IN_FLIGHT];
    size_t frame_index;
  };

  void render_target_init(const Context& context, Allocator& allocator, RenderTarget& render_target);
  void render_target_deinit(const Context& context, Allocator& allocator, RenderTarget& render_target);

  bool render_target_begin_frame(const Context& context, RenderTarget& render_target, Frame& frame);
  bool render_target_end_frame(const Context& context, RenderTarget& render_target, const Frame& frame);

  struct RendererCreateInfo
  {
    const char* vertex_shader_file_name;
    const char* fragment_shader_file_name;

    VertexInput       vertex_input;
    DescriptorInput   descriptor_input;
    PushConstantInput push_constant_input;
  };

  struct Renderer
  {
    Pipeline2  pipeline;
  };

  void renderer_init(const Context& context, const RenderTarget& render_target, RendererCreateInfo create_info, Renderer& renderer);
  void renderer_deinit(const Context& context, Renderer& renderer);

  void renderer_begin_render(Renderer& renderer, const Frame& frame);
  void renderer_end_render(Renderer& renderer, const Frame& frame);
}

