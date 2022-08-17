#include "buffer.hpp"
#include "command_buffer.hpp"
#include "context.hpp"
#include "descriptor_set.hpp"
#include "loader.hpp"
#include "model.hpp"
#include "render_target.hpp"
#include "renderer.hpp"
#include "sampler.hpp"
#include "shader.hpp"
#include "vk_check.hpp"

#include "tiny_obj_loader.h"
#include "stb_image.h"

#include <vulkan/vulkan_core.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vulkan/vulkan.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <chrono>

#include <utility>
#include <limits>
#include <fstream>
#include <algorithm>
#include <optional>
#include <vector>
#include <iostream>

#include <assert.h>
#include <string.h>
#include <stdlib.h>

// TODO: Move this outside
struct Matrices
{
  glm::mat4 mvp;
};

// Constant sections
static constexpr vulkan::ContextCreateInfo CONTEXT_CREATE_INFO = {
  .application_name = "Vulkan",
  .engine_name      = "Engine",
  .window_name      = "Vulkan",
  .width            = 1080,
  .height           = 720,
};
static constexpr const char *VERTEX_SHADER_FILE_NAME   = "shaders/vert.spv";
static constexpr const char *FRAGMENT_SHADER_FILE_NAME = "shaders/frag.spv";

static constexpr vulkan::DescriptorBinding DESCRIPTOR_BINDINGS[] = {
  {.type = vulkan::DescriptorType::SAMPLER,        .stage = vulkan::ShaderStage::FRAGMENT },
};

static constexpr vulkan::DescriptorInput DESCRIPTOR_INPUT = {
  .bindings      = DESCRIPTOR_BINDINGS,
  .binding_count = std::size(DESCRIPTOR_BINDINGS),
};

static constexpr vulkan::PushConstantRange PUSH_CONSTANT_RANGES[] = {
  {.offset = 0, .size = sizeof(Matrices), .stage = vulkan::ShaderStage::VERTEX },
};

static constexpr vulkan::PushConstantInput PUSH_CONSTANT_INPUT = {
  .ranges      = PUSH_CONSTANT_RANGES,
  .range_count = std::size(PUSH_CONSTANT_RANGES),
};

static constexpr vulkan::RendererCreateInfo RENDERER_CREATE_INFO = {
  .vertex_shader_file_name   = VERTEX_SHADER_FILE_NAME,
  .fragment_shader_file_name = FRAGMENT_SHADER_FILE_NAME,
  .vertex_input              = vulkan::VERTEX_INPUT,
  .descriptor_input          = DESCRIPTOR_INPUT,
  .push_constant_input       = PUSH_CONSTANT_INPUT,
};


struct Application
{
  vulkan::Context   context;
  vulkan::Allocator allocator;

  vulkan::Image     image;
  vulkan::ImageView image_view;
  vulkan::Model     model;
  vulkan::Sampler   sampler;

  vulkan::DescriptorPool descriptor_pool;
  vulkan::DescriptorSet  descriptor_set;

  vulkan::RenderTarget render_target;
  vulkan::Renderer     renderer;
};

void application_init(Application& application)
{
  vulkan::init_context(CONTEXT_CREATE_INFO, application.context);
  vulkan::init_allocator(application.context, application.allocator);

  vulkan::render_target_init(application.context, application.allocator, application.render_target);
  vulkan::renderer_init(application.context, application.render_target, RENDERER_CREATE_INFO, application.renderer);

  vulkan::load_image(application.context, application.allocator, application.image, "viking_room.png");
  vulkan::init_image_view(application.context, vulkan::ImageViewCreateInfo{
    .type   = vulkan::ImageViewType::COLOR,
    .format = VK_FORMAT_R8G8B8A8_SRGB,
    .image  = application.image,
  }, application.image_view);

  vulkan::init_sampler_simple(application.context, application.sampler);

  vulkan::load_model(application.context, application.allocator, "viking_room.obj", application.model);

  // Descriptor pool
  vulkan::init_descriptor_pool(application.context, vulkan::DescriptorPoolCreateInfo{
    .descriptor_input = DESCRIPTOR_INPUT,
    .count            = 1,
  }, application.descriptor_pool);

  const vulkan::Descriptor descriptors[] = {
    {.type = vulkan::DescriptorType::SAMPLER, .combined_image_sampler = { .image_view = application.image_view, .sampler = application.sampler, }}
  };

  vulkan::allocate_descriptor_set(application.context, application.descriptor_pool, application.renderer.pipeline.descriptor_set_layout, application.descriptor_set);
  vulkan::write_descriptor_set(application.context, application.descriptor_set, vulkan::DescriptorSetWriteInfo{
    .descriptors      = descriptors,
    .descriptor_count = std::size(descriptors),
  });
}

void application_deinit(Application& application)
{
  vkDeviceWaitIdle(application.context.device);

  vulkan::deinit_descriptor_pool(application.context, application.descriptor_pool);

  vulkan::deinit_model(application.context, application.allocator, application.model);
  vulkan::deinit_sampler(application.context, application.sampler);
  vulkan::deinit_image_view(application.context, application.image_view);
  vulkan::deinit_image(application.context, application.allocator, application.image);

  vulkan::renderer_deinit(application.context, application.renderer);
  vulkan::render_target_deinit(application.context, application.allocator, application.render_target);

  vulkan::deinit_allocator(application.context, application.allocator);
  vulkan::deinit_context(application.context);
}

void application_update(Application& application)
{
  vulkan::context_handle_events(application.context);
}

void application_render(Application& application)
{
  // Acquire frame
  vulkan::Frame frame = {};
  while(!vulkan::render_target_begin_frame(application.context, application.render_target, frame))
  {
    vkDeviceWaitIdle(application.context.device);

    vulkan::renderer_deinit(application.context, application.renderer);
    vulkan::render_target_deinit(application.context, application.allocator, application.render_target);
    vulkan::render_target_init(application.context, application.allocator, application.render_target);
    vulkan::renderer_init(application.context, application.render_target, RENDERER_CREATE_INFO, application.renderer);
  }

  // Rendering
  vulkan::renderer_begin_render(application.renderer, frame);
  {
    auto extent = application.render_target.swapchain.extent;
    Matrices matrices = {};
    {
      static auto start_time = std::chrono::high_resolution_clock::now();
      auto current_time = std::chrono::high_resolution_clock::now();
      float time = std::chrono::duration<float, std::chrono::seconds::period>(current_time - start_time).count();

      glm::mat4 model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
      glm::mat4 view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
      glm::mat4 proj = glm::perspective(glm::radians(45.0f), (float)extent.width / (float) extent.height, 0.1f, 10.0f);
      proj[1][1] *= -1;
      matrices.mvp = proj * view * model;
    }
    vulkan::renderer_push_constant(application.renderer, vulkan::ShaderStage::VERTEX, &matrices, 0, sizeof matrices);
    vulkan::renderer_bind_descriptor_set(application.renderer, application.descriptor_set);
    vulkan::renderer_set_viewport_and_scissor(application.renderer, extent);
    vulkan::command_model_render_simple(frame.command_buffer, application.model);
  }
  vulkan::renderer_end_render(application.renderer);

  // Present frame
  if(!vulkan::render_target_end_frame(application.context, application.render_target, frame))
  {
    vkDeviceWaitIdle(application.context.device);

    vulkan::renderer_deinit(application.context, application.renderer);
    vulkan::render_target_deinit(application.context, application.allocator, application.render_target);
    vulkan::render_target_init(application.context, application.allocator, application.render_target);
    vulkan::renderer_init(application.context, application.render_target, RENDERER_CREATE_INFO, application.renderer);
  }
}

void application_run(Application& application)
{
  while(!vulkan::context_should_destroy(application.context))
  {
    application_update(application);
    application_render(application);
  }
}

int main()
{
  glfwInit();

  Application application = {};
  application_init(application);
  application_run(application);
  application_deinit(application);

  glfwTerminate();
}
