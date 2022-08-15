#include "buffer.hpp"
#include "command_buffer.hpp"
#include "context.hpp"
#include "descriptor_set.hpp"
#include "render_context.hpp"
#include "sampler.hpp"
#include "shader.hpp"
#include "src/image.hpp"
#include "vk_check.hpp"
#include "vulkan.hpp"

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

struct Vertex
{
  glm::vec3 pos;
  glm::vec3 color;
  glm::vec2 uv;
};

// Constant sections
static constexpr size_t MAX_FRAME_IN_FLIGHT = 4;

static constexpr vulkan::ContextCreateInfo CONTEXT_CREATE_INFO = {
  .application_name = "Vulkan",
  .engine_name      = "Engine",
  .window_name      = "Vulkan",
  .width            = 1080,
  .height           = 720,
};
static constexpr const char *VERTEX_SHADER_FILE_NAME   = "shaders/vert.spv";
static constexpr const char *FRAGMENT_SHADER_FILE_NAME = "shaders/frag.spv";

static constexpr vulkan::VertexAttribute VERTEX_ATTRIBUTES[] = {
  { .offset = offsetof(Vertex, pos),   .type = vulkan::VertexAttribute::Type::FLOAT3 },
  { .offset = offsetof(Vertex, color), .type = vulkan::VertexAttribute::Type::FLOAT3 },
  { .offset = offsetof(Vertex, uv),    .type = vulkan::VertexAttribute::Type::FLOAT2 },
};

static constexpr vulkan::VertexBinding VERTEX_BINDINGS[] = {{
  .stride          = sizeof(Vertex),
  .attributes      = VERTEX_ATTRIBUTES,
  .attribute_count = std::size(VERTEX_ATTRIBUTES),
}};

static constexpr vulkan::VertexInput VERTEX_INPUT = {
  .bindings      = VERTEX_BINDINGS,
  .binding_count = std::size(VERTEX_BINDINGS),
};

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

struct Model
{
  size_t vertex_count, index_count;
  vulkan::Buffer vbo, ibo;
};

Model create_model(const vulkan::Context& context, vulkan::Allocator& allocator, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices)
{
  Model model = {};
  model.vertex_count = vertices.size();
  model.index_count  = indices.size();

  vulkan::BufferCreateInfo buffer_create_info = {};

  buffer_create_info.type = vulkan::BufferType::VERTEX_BUFFER;
  buffer_create_info.size = vertices.size() * sizeof vertices[0];
  vulkan::init_buffer(context, allocator, buffer_create_info, model.vbo);
  vulkan::write_buffer(context, allocator, model.vbo, vertices.data(), vertices.size() * sizeof vertices[0]);

  buffer_create_info.type = vulkan::BufferType::INDEX_BUFFER;
  buffer_create_info.size = indices.size() * sizeof indices[0];
  vulkan::init_buffer(context, allocator, buffer_create_info, model.ibo);
  vulkan::write_buffer(context, allocator, model.ibo, indices.data(), indices.size() * sizeof indices[0]);

  return model;
}

void destroy_model(const vulkan::Context& context, vulkan::Allocator& allocator, Model model)
{
  vulkan::deinit_buffer(context, allocator, model.vbo);
  vulkan::deinit_buffer(context, allocator, model.ibo);
}

Model load_model(const vulkan::Context& context, vulkan::Allocator& allocator, const char* file_name)
{
  tinyobj::attrib_t attrib;
  std::vector<tinyobj::shape_t> shapes;
  std::vector<tinyobj::material_t> materials;
  std::string warn, err;
  std::vector<Vertex>   vertices;
  std::vector<uint32_t> indices;

  if(!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, file_name))
  {
    fprintf(stderr, "Failed to load object file:%s", err.c_str());
    abort();
  }
  fprintf(stderr, "Warning:%s", warn.c_str());

  for(const auto& shape : shapes)
    for(const auto& index : shape.mesh.indices)
    {
      Vertex vertex = {};
      vertex.pos = {
        attrib.vertices[3 * index.vertex_index + 0],
        attrib.vertices[3 * index.vertex_index + 1],
        attrib.vertices[3 * index.vertex_index + 2]
      };
      vertex.uv = {
        attrib.texcoords[2 * index.texcoord_index + 0],
        1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
      };
      vertex.color = {1.0f, 1.0f, 1.0f};
      vertices.push_back(vertex);
      indices.push_back(indices.size());
    }

  return create_model(context, allocator, vertices, indices);
}

int main()
{
  glfwInit();

  // Context
  vulkan::Context context = {};
  vulkan::init_context(CONTEXT_CREATE_INFO, context);

  vulkan::Allocator allocator = {};
  vulkan::init_allocator(context, allocator);

  vulkan::Image image = {};
  vulkan::load_image(context, allocator, image, "viking_room.png");

  vulkan::ImageView image_view = {};
  vulkan::init_image_view(context, vulkan::ImageViewCreateInfo{
    .type   = vulkan::ImageViewType::COLOR,
    .format = VK_FORMAT_R8G8B8A8_SRGB,
    .image  = image,
  }, image_view);

  Model   model   = load_model(context, allocator, "viking_room.obj");

  // May need to be recreated on window resize
  const vulkan::RenderContextCreateInfo render_context_create_info{
    .vertex_shader_file_name   = VERTEX_SHADER_FILE_NAME,
    .fragment_shader_file_name = FRAGMENT_SHADER_FILE_NAME,
    .vertex_input              = VERTEX_INPUT,
    .descriptor_input          = DESCRIPTOR_INPUT,
    .push_constant_input       = PUSH_CONSTANT_INPUT,
    .max_frame_in_flight       = MAX_FRAME_IN_FLIGHT,
  };

  vulkan::RenderContext render_context = {};
  vulkan::init_render_context(context, allocator, render_context_create_info, render_context);

  vulkan::Sampler sampler = {};
  vulkan::init_sampler_simple(context, sampler);

  // Descriptor pool
  vulkan::DescriptorPool descriptor_pool = {};
  vulkan::init_descriptor_pool(context, vulkan::DescriptorPoolCreateInfo{
    .descriptor_input = DESCRIPTOR_INPUT,
    .count            = 1,
  }, descriptor_pool);

  vulkan::DescriptorSet descriptor_set;
  const vulkan::Descriptor descriptors[] = {
    {.type = vulkan::DescriptorType::SAMPLER, .combined_image_sampler = { .image_view = image_view, .sampler = sampler, }}
  };

  vulkan::allocate_descriptor_set(context, descriptor_pool, render_context.pipeline.descriptor_set_layout, descriptor_set);
  vulkan::write_descriptor_set(context, descriptor_set, vulkan::DescriptorSetWriteInfo{
    .descriptors      = descriptors,
    .descriptor_count = std::size(descriptors),
  });

  while(!vulkan::context_should_destroy(context))
  {
    vulkan::context_handle_events(context);

    auto frame_info = vulkan::begin_render(context, render_context);
    while(!frame_info)
    {
      std::cout << "Recreating render context\n";

      vkDeviceWaitIdle(context.device);
      vulkan::deinit_render_context(context, allocator, render_context);
      vulkan::init_render_context(context, allocator, render_context_create_info, render_context);
      frame_info = vulkan::begin_render(context, render_context);
    }

    auto extent = render_context.swapchain.extent;

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
    vulkan::command_push_constant(frame_info->command_buffer, render_context.pipeline, vulkan::ShaderStage::VERTEX, &matrices, 0, sizeof matrices);
    vulkan::command_bind_descriptor_set(frame_info->command_buffer, render_context.pipeline, descriptor_set);

    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(frame_info->command_buffer.handle, 0, 1, &model.vbo.handle, offsets);
    vkCmdBindIndexBuffer(frame_info->command_buffer.handle, model.ibo.handle, 0, VK_INDEX_TYPE_UINT32);

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width  = extent.width;
    viewport.height = extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(frame_info->command_buffer.handle, 0, 1, &viewport);

    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = extent;
    vkCmdSetScissor(frame_info->command_buffer.handle, 0, 1, &scissor);

    vkCmdDrawIndexed(frame_info->command_buffer.handle, model.index_count, 1, 0, 0, 0);

    if(!vulkan::end_render(context, render_context, *frame_info))
    {
      std::cout << "Recreating render context\n";

      vkDeviceWaitIdle(context.device);
      vulkan::deinit_render_context(context, allocator, render_context);
      vulkan::init_render_context(context, allocator, render_context_create_info, render_context);
    }
  }

  vkDeviceWaitIdle(context.device);

  vulkan::deinit_render_context(context, allocator, render_context);
  vulkan::deinit_allocator(context, allocator);
  vulkan::deinit_context(context);
  glfwTerminate();
}
