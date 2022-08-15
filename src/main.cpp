#include "buffer.hpp"
#include "command_buffer.hpp"
#include "context.hpp"
#include "descriptor_set.hpp"
#include "render_context.hpp"
#include "sampler.hpp"
#include "shader.hpp"
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
struct UniformBufferObject
{
  alignas(16) glm::mat4 model;
  alignas(16) glm::mat4 view;
  alignas(16) glm::mat4 proj;
};

struct Vertex
{
  glm::vec3 pos;
  glm::vec3 color;
  glm::vec2 uv;
};

static constexpr size_t MAX_FRAME_IN_FLIGHT = 4;

struct Texture
{
  vulkan::Image     image;
  vulkan::ImageView image_view;
};

Texture create_texture(const vulkan::Context& context, vulkan::Allocator& allocator, const void *data, size_t width, size_t height)
{
  Texture texture = {};

  vulkan::ImageCreateInfo image_create_info = {};
  image_create_info.type          = vulkan::ImageType::TEXTURE;
  image_create_info.format        = VK_FORMAT_R8G8B8A8_SRGB;
  image_create_info.extent.width  = width;
  image_create_info.extent.height = height;
  vulkan::init_image(context, allocator, image_create_info, texture.image);
  vulkan::write_image(context, allocator, texture.image, data, width, height, width * height * 4);

  vulkan::ImageViewCreateInfo image_view_create_info = {};
  image_view_create_info.type = vulkan::ImageViewType::COLOR;
  image_view_create_info.format = VK_FORMAT_R8G8B8A8_SRGB;
  image_view_create_info.image = texture.image;
  vulkan::init_image_view(context, image_view_create_info, texture.image_view);

  return texture;
}

void destroy_texture(const vulkan::Context& context, vulkan::Allocator& allocator, Texture texture)
{
  vulkan::deinit_image_view(context, texture.image_view);
  vulkan::deinit_image(context, allocator, texture.image);
}

Texture load_texture(const vulkan::Context& context, vulkan::Allocator& allocator, const char *file_name)
{
  int x, y, n;
  unsigned char *data = stbi_load(file_name, &x, &y, &n, STBI_rgb_alpha);
  assert(data);
  Texture texture = create_texture(context, allocator, data, x, y);
  stbi_image_free(data);
  return texture;
}

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

const vulkan::ContextCreateInfo context_create_info = {
  .application_name = "Vulkan",
  .engine_name      = "Engine",
  .window_name      = "Vulkan",
  .width            = 1080,
  .height           = 720,
};

int main()
{
  glfwInit();

  // Context
  vulkan::Context context = {};
  vulkan::init_context(context_create_info, context);

  vulkan::Allocator allocator = {};
  vulkan::init_allocator(context, allocator);

  Texture texture = load_texture(context, allocator, "viking_room.png");
  Model   model   = load_model(context, allocator, "viking_room.obj");

  vulkan::Shader vertex_shader = {};
  vulkan::Shader fragment_shader = {};
  vulkan::load_shader(context, "shaders/vert.spv", vertex_shader);
  vulkan::load_shader(context, "shaders/frag.spv", fragment_shader);

  // May need to be recreated on window resize
  const vulkan::VertexAttribute vertex_attributes[] = {
    { .offset = offsetof(Vertex, pos),   .type = vulkan::VertexAttribute::Type::FLOAT3 },
    { .offset = offsetof(Vertex, color), .type = vulkan::VertexAttribute::Type::FLOAT3 },
    { .offset = offsetof(Vertex, uv),    .type = vulkan::VertexAttribute::Type::FLOAT2 },
  };

  const vulkan::VertexBinding vertex_bindings[] = {{
    .stride          = sizeof(Vertex),
    .attributes      = vertex_attributes,
    .attribute_count = std::size(vertex_attributes),
  }};

  const vulkan::VertexInput vertex_input = {
    .bindings      = vertex_bindings,
    .binding_count = std::size(vertex_bindings),
  };

  const vulkan::RenderContextCreateInfo render_context_create_info{
    .vertex_shader   = vertex_shader,
    .fragment_shader = fragment_shader,
    .vertex_input    = vertex_input,
    .max_frame_in_flight = 4,
  };
  vulkan::RenderContext render_context = {};
  vulkan::init_render_context(context, allocator, render_context_create_info, render_context);

  vulkan::Sampler sampler = {};
  vulkan::init_sampler_simple(context, sampler);

  vulkan::Buffer ubos[MAX_FRAME_IN_FLIGHT];
  for (size_t i = 0; i < MAX_FRAME_IN_FLIGHT; i++)
  {
    vulkan::BufferCreateInfo buffer_create_info = {};
    buffer_create_info.type = vulkan::BufferType::UNIFORM_BUFFER;
    buffer_create_info.size = sizeof(UniformBufferObject);
    vulkan::init_buffer(context, allocator, buffer_create_info, ubos[i]);
  }

  // Descriptor pool
  const vulkan::DescriptorBinding descriptor_infos[] = {
    {.type = vulkan::DescriptorType::UNIFORM_BUFFER, .stage = vulkan::ShaderStage::VERTEX },
    {.type = vulkan::DescriptorType::SAMPLER,        .stage = vulkan::ShaderStage::FRAGMENT },
  };
  vulkan::DescriptorPool descriptor_pool = {};
  vulkan::init_descriptor_pool(context, vulkan::DescriptorPoolCreateInfo{
    .descriptors      = descriptor_infos,
    .descriptor_count = std::size(descriptor_infos),
    .count            = MAX_FRAME_IN_FLIGHT,
  }, descriptor_pool);

  vulkan::DescriptorSet descriptor_sets[MAX_FRAME_IN_FLIGHT];
  for (size_t i = 0; i < MAX_FRAME_IN_FLIGHT; i++)
  {
    const vulkan::Descriptor descriptors[] = {
      {.type = vulkan::DescriptorType::UNIFORM_BUFFER, .uniform_buffer         = { .buffer = ubos[i], .size = sizeof(UniformBufferObject), }},
      {.type = vulkan::DescriptorType::SAMPLER,        .combined_image_sampler = { .image_view = texture.image_view, .sampler = sampler, }}
    };

    vulkan::allocate_descriptor_set(context, descriptor_pool, vulkan::DescriptorSetLayout{render_context.pipeline.descriptor_set_layout}, descriptor_sets[i]);
    vulkan::write_descriptor_set(context, descriptor_sets[i], vulkan::DescriptorSetWriteInfo{
      .descriptors      = descriptors,
      .descriptor_count = std::size(descriptors),
    });
  }

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

    static auto start_time = std::chrono::high_resolution_clock::now();
    auto current_time = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(current_time - start_time).count();

    auto extent = render_context.swapchain.extent;

    UniformBufferObject ubo = {};
    ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.proj = glm::perspective(glm::radians(45.0f), (float)extent.width / (float) extent.height, 0.1f, 10.0f);
    ubo.proj[1][1] *= -1;
    vulkan::write_buffer(context, allocator, ubos[frame_info->frame_index], &ubo, sizeof ubo);

    vkCmdBindDescriptorSets(frame_info->command_buffer.handle,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        render_context.pipeline.pipeline_layout,
        0, 1,
        &descriptor_sets[frame_info->frame_index].handle,
        0, nullptr);

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
