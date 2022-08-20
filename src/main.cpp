#include "command_buffer.hpp"
#include "context.hpp"
#include "descriptor_set.hpp"
#include "mesh.hpp"
#include "render_target.hpp"
#include "renderer.hpp"
#include "resources/buffer.hpp"
#include "sampler.hpp"
#include "shader.hpp"
#include "texture.hpp"
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
  glm::mat4 model;
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

struct Block
{
  bool valid;
  glm::vec3 color;
};

static constexpr size_t CHUNK_SIZE = 8;
static constexpr float  BLOCK_WIDTH = 0.2f;

struct Chunk
{
  Block blocks[CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE];
};

Chunk *chunk_generate_random()
{
  srand(time(NULL));
  Chunk *chunk = new Chunk;
  for(size_t z=0; z<CHUNK_SIZE; ++z)
    for(size_t y=0; y<CHUNK_SIZE; ++y)
      for(size_t x=0; x<CHUNK_SIZE; ++x)
      {
        chunk->blocks[z][y][z].valid = (unsigned)rand() % 8 == 0;
        chunk->blocks[z][y][z].color = glm::vec3(x, y, z) * 2.0f / (float)CHUNK_SIZE;
      }

  return chunk;
}

vulkan::mesh_t chunk_generate_mesh(const vulkan::Context& context, vulkan::Allocator& allocator, const Chunk& chunk)
{
  vector<vulkan::Vertex> vertices = create_vector<vulkan::Vertex>(1);
  vector<uint32_t>       indices  = create_vector<uint32_t>(1);

  for(size_t z=0; z<CHUNK_SIZE; ++z)
    for(size_t y=0; y<CHUNK_SIZE; ++y)
      for(size_t x=0; x<CHUNK_SIZE; ++x)
      {
        if(!chunk.blocks[z][y][x].valid)
          continue;

        vulkan::Vertex block_vertices[] = {
          {.pos = glm::vec3(x,   y,   z  ) * BLOCK_WIDTH, .normal = {}, .color = chunk.blocks[z][y][x].color, .uv = {}},
          {.pos = glm::vec3(x,   y,   z+1) * BLOCK_WIDTH, .normal = {}, .color = chunk.blocks[z][y][x].color, .uv = {}},
          {.pos = glm::vec3(x,   y+1, z  ) * BLOCK_WIDTH, .normal = {}, .color = chunk.blocks[z][y][x].color, .uv = {}},
          {.pos = glm::vec3(x,   y+1, z+1) * BLOCK_WIDTH, .normal = {}, .color = chunk.blocks[z][y][x].color, .uv = {}},
          {.pos = glm::vec3(x+1, y,   z  ) * BLOCK_WIDTH, .normal = {}, .color = chunk.blocks[z][y][x].color, .uv = {}},
          {.pos = glm::vec3(x+1, y,   z+1) * BLOCK_WIDTH, .normal = {}, .color = chunk.blocks[z][y][x].color, .uv = {}},
          {.pos = glm::vec3(x+1, y+1, z  ) * BLOCK_WIDTH, .normal = {}, .color = chunk.blocks[z][y][x].color, .uv = {}},
          {.pos = glm::vec3(x+1, y+1, z+1) * BLOCK_WIDTH, .normal = {}, .color = chunk.blocks[z][y][x].color, .uv = {}},
        };
        uint32_t block_indices[] = {
          3, 2, 0, 1, 3, 0,
          5, 4, 6, 7, 5, 6,
          7, 6, 2, 3, 7, 2,
          1, 0, 4, 5, 1, 4,
          6, 4, 0, 2, 6, 0,
          1, 5, 7, 1, 7, 3,
        };

        for(size_t i=0; i<std::size(block_indices); ++i)
          vector_resize_push<uint32_t>(indices, size(vertices) + block_indices[i]);

        for(size_t i=0; i<std::size(block_vertices); ++i)
          vector_resize_push(vertices, block_vertices[i]);
      }

  printf("vertices size = %ld\n", size(vertices));
  printf("indices  size = %ld\n", size(indices));

  vulkan::mesh_t mesh = vulkan::mesh_create(&context, &allocator, size(vertices), size(indices));

  vulkan::command_buffer_t command_buffer = command_buffer_create(&context);
  command_buffer_begin(command_buffer);

  vulkan::mesh_write(command_buffer, mesh, data(vertices), data(indices));

  command_buffer_end(command_buffer);

  vulkan::Fence fence = {};
  init_fence(context, fence, false);
  command_buffer_submit(command_buffer, fence);
  fence_wait_and_reset(context, fence);
  deinit_fence(context, fence);

  command_buffer_put(command_buffer);

  destroy_vector(vertices);
  destroy_vector(indices);
  return mesh;
}

struct Application
{
  vulkan::Context   context;
  vulkan::Allocator allocator;

  vulkan::mesh_t  mesh;
  vulkan::Texture texture;
  vulkan::Sampler sampler;

  Chunk          *chunk;
  vulkan::mesh_t  chunk_mesh;

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

  vulkan::command_buffer_t command_buffer = command_buffer_create(&application.context);
  command_buffer_begin(command_buffer);

  application.mesh = vulkan::mesh_load(command_buffer, &application.context, &application.allocator, "viking_room.obj");

  command_buffer_end(command_buffer);

  vulkan::Fence fence = {};
  init_fence(application.context, fence, false);
  command_buffer_submit(command_buffer, fence);
  fence_wait_and_reset(application.context, fence);
  deinit_fence(application.context, fence);

  command_buffer_put(command_buffer);

  vulkan::texture_load(application.context, application.allocator, "viking_room.png", application.texture);
  vulkan::init_sampler_simple(application.context, application.sampler);

  application.chunk = chunk_generate_random();
  application.chunk_mesh = chunk_generate_mesh(application.context, application.allocator, *application.chunk);

  // Descriptor pool
  vulkan::init_descriptor_pool(application.context, vulkan::DescriptorPoolCreateInfo{
    .descriptor_input = DESCRIPTOR_INPUT,
    .count            = 1,
  }, application.descriptor_pool);

  const vulkan::Descriptor descriptors[] = {
    {.type = vulkan::DescriptorType::SAMPLER, .combined_image_sampler = { .image_view = application.texture.image_view, .sampler = application.sampler, }}
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

  vulkan::mesh_put(application.mesh);
  vulkan::mesh_put(application.chunk_mesh);
  vulkan::texture_deinit(application.context, application.allocator, application.texture);
  vulkan::deinit_sampler(application.context, application.sampler);

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
      matrices.model = model;
    }
    vulkan::renderer_set_viewport_and_scissor(application.renderer, extent);

    //vulkan::renderer_push_constant(application.renderer, vulkan::ShaderStage::VERTEX, &matrices, 0, sizeof matrices);
    //vulkan::renderer_bind_descriptor_set(application.renderer, application.descriptor_set);
    //vulkan::mesh_render_simple(frame.command_buffer, application.mesh);

    vulkan::renderer_push_constant(application.renderer, vulkan::ShaderStage::VERTEX, &matrices, 0, sizeof matrices);
    vulkan::renderer_bind_descriptor_set(application.renderer, application.descriptor_set);
    vulkan::mesh_render_simple(frame.command_buffer, application.chunk_mesh);
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
