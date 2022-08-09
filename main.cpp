#include "context.hpp"
#include "render_context.hpp"
#include "command_buffer.hpp"
#include "vulkan.hpp"
#include "vk_check.hpp"
#include "buffer.hpp"

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

// TODO: Move this outside
inline std::vector<char> read_file(const char* file_name)
{
  std::ifstream file(file_name, std::ios::ate | std::ios::binary);
  assert(file.is_open());

  auto end = file.tellg();
  file.seekg(0);
  auto begin = file.tellg();

  const size_t file_size = end - begin;
  auto file_content = std::vector<char>(file_size);
  file.read(file_content.data(), file_content.size());
  return file_content;
}

static constexpr size_t MAX_FRAME_IN_FLIGHT = 4;

struct Texture
{
  vulkan::MemoryAllocation allocation;
  VkImage     image;
  VkImageView image_view;
};

Texture create_texture(const vulkan::Context& context, vulkan::allocator_t allocator, const void *data, size_t width, size_t height)
{
  Texture texture = {};

  {
    vulkan::Image2dCreateInfo info = {};
    info.usage      = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    info.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    info.format     = VK_FORMAT_R8G8B8A8_SRGB;
    info.width      = width;
    info.height     = height;
    texture.image = vulkan::create_image2d(context, allocator, info, texture.allocation);
    vulkan::write_image2d(context, allocator, texture.image, width, height, texture.allocation, data);
  }

  {
    VkImageViewCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    create_info.image                           = texture.image;
    create_info.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
    create_info.format                          = VK_FORMAT_R8G8B8A8_SRGB;
    create_info.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    create_info.subresourceRange.baseMipLevel   = 0;
    create_info.subresourceRange.levelCount     = 1;
    create_info.subresourceRange.baseArrayLayer = 0;
    create_info.subresourceRange.layerCount     = 1;
    VK_CHECK(vkCreateImageView(context.device, &create_info, nullptr, &texture.image_view));
  }

  return texture;
}

void destroy_texture(const vulkan::Context& context, vulkan::allocator_t allocator, Texture texture)
{
  vkDestroyImageView(context.device, texture.image_view, nullptr);
  vkDestroyImage(context.device, texture.image, nullptr);
  vulkan::deallocate_memory(context, allocator, texture.allocation);
}

Texture load_texture(const vulkan::Context& context, vulkan::allocator_t allocator, const char *file_name)
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
  size_t vertex_count;
  size_t index_count;

  vulkan::MemoryAllocation vbo_allocation;
  vulkan::MemoryAllocation ibo_allocation;

  VkBuffer vbo;
  VkBuffer ibo;

};

Model create_model(const vulkan::Context& context, vulkan::allocator_t allocator, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices)
{
  Model model = {};
  model.vertex_count = vertices.size();
  model.index_count  = indices.size();

  {
    vulkan::BufferCreateInfo info = {};
    info.usage      = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    info.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    info.size       = vertices.size() * sizeof vertices[0];
    model.vbo = vulkan::create_buffer(context, allocator, info, model.vbo_allocation);
    vulkan::write_buffer(context, allocator, model.vbo, model.vbo_allocation, vertices.data());
  }

  {
    vulkan::BufferCreateInfo info = {};
    info.usage      = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    info.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    info.size       = indices.size() * sizeof indices[0];
    model.ibo = vulkan::create_buffer(context, allocator, info, model.ibo_allocation);
    vulkan::write_buffer(context, allocator, model.ibo, model.ibo_allocation, indices.data());
  }

  return model;
}

void destroy_model(const vulkan::Context& context, vulkan::allocator_t allocator, Model model)
{
  vkDestroyBuffer(context.device, model.vbo, nullptr);
  vkDestroyBuffer(context.device, model.ibo, nullptr);

  vulkan::deallocate_memory(context, allocator, model.vbo_allocation);
  vulkan::deallocate_memory(context, allocator, model.ibo_allocation);
}

Model load_model(const vulkan::Context& context, vulkan::allocator_t allocator, const char* file_name)
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
  {
    vulkan::ContextCreateInfo context_create_info = {};
    context_create_info.application_name    = "Vulkan";
    context_create_info.engine_name         = "Engine";
    context_create_info.window_name         = "Vulkan";
    context_create_info.width               = 1080;
    context_create_info.height              = 720;
    vulkan::init_context(context_create_info, context);
  }
  vulkan::allocator_t allocator = vulkan::create_allocator(context);

  Texture texture = load_texture(context, allocator, "viking_room.png");
  Model model = load_model(context, allocator, "viking_room.obj");

  // May need to be recreated on window resize
  vulkan::RenderContextCreateInfo render_context_create_info = {};
  render_context_create_info.vert_shader_module = vulkan::create_shader_module(context.device, read_file("shaders/vert.spv"));
  render_context_create_info.frag_shader_module = vulkan::create_shader_module(context.device, read_file("shaders/frag.spv"));
  render_context_create_info.vertex_binding_descriptions = {
    vulkan::VertexBindingDescription{
      .stride = sizeof(Vertex),
      .attribute_descriptions = {
        vulkan::VertexAttributeDescription{ .offset = offsetof(Vertex, pos),   .type = vulkan::VertexAttributeDescription::Type::FLOAT3 },
        vulkan::VertexAttributeDescription{ .offset = offsetof(Vertex, color), .type = vulkan::VertexAttributeDescription::Type::FLOAT3 },
        vulkan::VertexAttributeDescription{ .offset = offsetof(Vertex, uv),    .type = vulkan::VertexAttributeDescription::Type::FLOAT2 },
      }
    }
  };
  render_context_create_info.max_frame_in_flight = 4;

  vulkan::render_context_t render_context = create_render_context(context, allocator, render_context_create_info);

  VkSampler sampler;
  {
    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(context.physical_device, &properties);

    VkSamplerCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    create_info.magFilter = VK_FILTER_LINEAR;
    create_info.minFilter = VK_FILTER_LINEAR;
    create_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    create_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    create_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    create_info.anisotropyEnable = VK_TRUE;
    create_info.maxAnisotropy    = properties.limits.maxSamplerAnisotropy;
    create_info.borderColor      = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    create_info.unnormalizedCoordinates = VK_FALSE;
    create_info.compareEnable           = VK_FALSE;
    create_info.compareOp               = VK_COMPARE_OP_ALWAYS;
    create_info.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    create_info.mipLodBias              = 0.0f;
    create_info.minLod                  = 0.0f;
    create_info.maxLod                  = 0.0f;
    VK_CHECK(vkCreateSampler(context.device, &create_info, nullptr, &sampler));
  }

  // Descriptor pool
  VkDescriptorPoolSize pool_sizes[2] = {};
  pool_sizes[0].type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  pool_sizes[0].descriptorCount = MAX_FRAME_IN_FLIGHT;
  pool_sizes[1].type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  pool_sizes[1].descriptorCount = MAX_FRAME_IN_FLIGHT;

  VkDescriptorPoolCreateInfo pool_create_info = {};
  pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  pool_create_info.poolSizeCount = 2;
  pool_create_info.pPoolSizes = pool_sizes;
  pool_create_info.maxSets    = MAX_FRAME_IN_FLIGHT;

  VkDescriptorPool descriptor_pool = {};
  VK_CHECK(vkCreateDescriptorPool(context.device, &pool_create_info, nullptr, &descriptor_pool));

  VkDescriptorSetLayout descriptor_set_layouts[MAX_FRAME_IN_FLIGHT];
  VkDescriptorSet descriptor_sets[MAX_FRAME_IN_FLIGHT];

  std::fill(std::begin(descriptor_set_layouts), std::end(descriptor_set_layouts), vulkan::render_context_get_descriptor_set_layout(render_context));

  VkDescriptorSetAllocateInfo alloc_info = {};
  alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  alloc_info.descriptorPool     = descriptor_pool;
  alloc_info.descriptorSetCount = MAX_FRAME_IN_FLIGHT;
  alloc_info.pSetLayouts        = descriptor_set_layouts;

  VK_CHECK(vkAllocateDescriptorSets(context.device, &alloc_info, descriptor_sets));

  VkBuffer                 ubos[MAX_FRAME_IN_FLIGHT];
  vulkan::MemoryAllocation ubo_allocations[MAX_FRAME_IN_FLIGHT];
  for (size_t i = 0; i < MAX_FRAME_IN_FLIGHT; i++)
  {
    vulkan::BufferCreateInfo create_info = {};
    create_info.usage      = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    create_info.properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    create_info.size       = sizeof(UniformBufferObject);
    ubos[i] = vulkan::create_buffer(context, allocator, create_info, ubo_allocations[i]);
  }

  for (size_t i = 0; i < MAX_FRAME_IN_FLIGHT; i++)
  {
    VkDescriptorBufferInfo buffer_info = {};
    buffer_info.buffer = ubos[i];
    buffer_info.offset = 0;
    buffer_info.range  = sizeof(UniformBufferObject);

    VkDescriptorImageInfo image_info = {};
    image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    image_info.imageView   = texture.image_view;
    image_info.sampler     = sampler;

    VkWriteDescriptorSet write_descriptors[2] = {};
    write_descriptors[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_descriptors[0].dstSet          = descriptor_sets[i];
    write_descriptors[0].dstBinding      = 0;
    write_descriptors[0].dstArrayElement = 0;
    write_descriptors[0].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    write_descriptors[0].descriptorCount = 1;
    write_descriptors[0].pBufferInfo     = &buffer_info;

    write_descriptors[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_descriptors[1].dstSet          = descriptor_sets[i];
    write_descriptors[1].dstBinding      = 1;
    write_descriptors[1].dstArrayElement = 0;
    write_descriptors[1].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write_descriptors[1].descriptorCount = 1;
    write_descriptors[1].pImageInfo      = &image_info;

    vkUpdateDescriptorSets(context.device, 2, write_descriptors, 0, nullptr);
  }


  while(!vulkan::context_should_destroy(context))
  {
    vulkan::context_handle_events(context);

    auto frame_info = vulkan::begin_render(context, render_context);
    while(!frame_info)
    {
      std::cout << "Recreating render context\n";

      vkDeviceWaitIdle(context.device);
      vulkan::destroy_render_context(context, allocator, render_context);
      render_context = vulkan::create_render_context(context, allocator, render_context_create_info);

      frame_info = vulkan::begin_render(context, render_context);
    }

    static auto start_time = std::chrono::high_resolution_clock::now();
    auto current_time = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(current_time - start_time).count();

    auto extent = vulkan::render_context_get_extent(render_context);

    UniformBufferObject ubo = {};
    ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.proj = glm::perspective(glm::radians(45.0f), (float)extent.width / (float) extent.height, 0.1f, 10.0f);
    ubo.proj[1][1] *= -1;
    vulkan::write_buffer(context, allocator, ubos[frame_info->frame_index], ubo_allocations[frame_info->frame_index], &ubo);

    vkCmdBindDescriptorSets(frame_info->command_buffer.handle,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        vulkan::render_context_get_pipeline_layout(render_context),
        0, 1,
        &descriptor_sets[frame_info->frame_index],
        0, nullptr);

    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(frame_info->command_buffer.handle, 0, 1, &model.vbo, offsets);
    vkCmdBindIndexBuffer(frame_info->command_buffer.handle, model.ibo, 0, VK_INDEX_TYPE_UINT32);

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
      vulkan::destroy_render_context(context, allocator, render_context);
      render_context = vulkan::create_render_context(context, allocator, render_context_create_info);
    }
  }

  vkDeviceWaitIdle(context.device);

  vulkan::destroy_render_context(context, allocator, render_context);
  vulkan::destroy_allocator(context, allocator);
  vulkan::deinit_context(context);
  glfwTerminate();
}
