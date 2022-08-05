#include "context.hpp"
#include "render_context.hpp"
#include "command_buffer.hpp"
#include "vulkan.hpp"
#include "buffer.hpp"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#define STB_IMAGE_IMPLEMENTATION
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

#define VK_CHECK(expr) do { if(expr != VK_SUCCESS) { fprintf(stderr, "Vulkan pooped itself:%s\n", #expr); abort(); } } while(0)

// How you want to render?
// Unlike OpenGL, you have to prespecify a lot of things
struct FrameInfo
{
  uint32_t image_index;
  VkFramebuffer framebuffer;
};

std::optional<FrameInfo> begin_frame(const vulkan::Context& context, const vulkan::RenderContext& render_context, VkSemaphore semaphore)
{
  FrameInfo frame_info = {};
  auto result = vkAcquireNextImageKHR(context.device, render_context.swapchain, UINT64_MAX, semaphore, VK_NULL_HANDLE, &frame_info.image_index);
  if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
  {
    // The window have probably resized
    return std::nullopt;
  }
  VK_CHECK(result);
  frame_info.framebuffer = render_context.frames[frame_info.image_index].framebuffer;
  return frame_info;
}

bool end_frame(const vulkan::Context& context, const vulkan::RenderContext& render_context, FrameInfo frame_info, VkSemaphore semaphore)
{
  VkPresentInfoKHR present_info = {};
  present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  present_info.waitSemaphoreCount = 1;
  present_info.pWaitSemaphores = &semaphore;

  VkSwapchainKHR swapchains[] = { render_context.swapchain };
  present_info.swapchainCount = 1;
  present_info.pSwapchains    = swapchains;
  present_info.pImageIndices  = &frame_info.image_index;
  present_info.pResults       = nullptr;

  auto result = vkQueuePresentKHR(context.queue, &present_info);
  if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
  {
    // The window have probably resized
    return false;
  }

  VK_CHECK(result);
  return true;
}

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


struct RenderResource
{
  vulkan::CommandBuffer command_buffer;

  VkSemaphore semaphore_image_available;
  VkSemaphore semaphore_render_finished;

  vulkan::MemoryAllocation ubo_allocation;
  VkBuffer                 ubo;
};

RenderResource create_render_resouce(const vulkan::Context& context, vulkan::Allocator& allocator)
{
  RenderResource render_resource = {};
  render_resource.command_buffer            = vulkan::create_command_buffer(context, true);
  render_resource.semaphore_image_available = vulkan::create_semaphore(context.device);
  render_resource.semaphore_render_finished = vulkan::create_semaphore(context.device);

  vulkan::BufferCreateInfo info = {};
  info.usage      = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
  info.properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
  info.size       = sizeof(UniformBufferObject);
  render_resource.ubo = vulkan::create_buffer(context, allocator, info, render_resource.ubo_allocation);
  return render_resource;
}

void begin_render(const vulkan::Context& context, const vulkan::RenderContext& render_context, const RenderResource& render_resource, const FrameInfo& frame_info)
{
  vulkan::command_buffer_wait(context, render_resource.command_buffer);
  vulkan::command_buffer_begin(render_resource.command_buffer);

  VkRenderPassBeginInfo render_pass_begin_info = {};
  render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  render_pass_begin_info.renderPass        = render_context.render_pass;
  render_pass_begin_info.framebuffer       = frame_info.framebuffer;
  render_pass_begin_info.renderArea.offset = {0, 0};
  render_pass_begin_info.renderArea.extent = render_context.extent;

  VkClearValue clear_values[2] = {};
  clear_values[0].color        = {{ 0.0f, 0.0f, 0.0f, 1.0f }};
  clear_values[1].depthStencil = { 1.0f, 0 };

  render_pass_begin_info.clearValueCount = 2;
  render_pass_begin_info.pClearValues = clear_values;

  vkCmdBeginRenderPass(render_resource.command_buffer.handle, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
  vkCmdBindPipeline(render_resource.command_buffer.handle, VK_PIPELINE_BIND_POINT_GRAPHICS, render_context.pipeline);
}

void end_render(const vulkan::Context& context, const RenderResource& render_resource)
{
  vkCmdEndRenderPass(render_resource.command_buffer.handle);

  vulkan::command_buffer_end(render_resource.command_buffer);
  vulkan::command_buffer_submit(context, render_resource.command_buffer,
      render_resource.semaphore_image_available, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
      render_resource.semaphore_render_finished);
}

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

int main()
{
  glfwInit();

  // Window and KHR API
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  //glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
  GLFWwindow *window = glfwCreateWindow(800, 600, "Vulkan", nullptr, nullptr);

  // Context
  vulkan::ContextCreateInfo context_create_info = {};
  context_create_info.application_name    = "Vulkan";
  context_create_info.application_version = VK_MAKE_VERSION(1, 0, 0);
  context_create_info.engine_name         = "Engine";
  context_create_info.engine_version      = VK_MAKE_VERSION(1, 0, 0);
  context_create_info.window              = window;
  auto context   = create_context(context_create_info);
  auto allocator = vulkan::create_allocator(context);

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

  auto render_context = create_render_context(context, allocator, render_context_create_info);

  int x, y, n;
  unsigned char *data = stbi_load("viking_room.png", &x, &y, &n, STBI_rgb_alpha);
  assert(data);

  vulkan::MemoryAllocation image_allocation = {};
  VkImage image = VK_NULL_HANDLE;
  {
    vulkan::Image2dCreateInfo info = {};
    info.usage      = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    info.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    info.format     = VK_FORMAT_R8G8B8A8_SRGB;
    info.width      = x;
    info.height     = y;
    image = vulkan::create_image2d(context, allocator, info, image_allocation);
    vulkan::write_image2d(context, allocator, image, x, y, image_allocation, data);
  }

  stbi_image_free(data);

  tinyobj::attrib_t attrib;
  std::vector<tinyobj::shape_t> shapes;
  std::vector<tinyobj::material_t> materials;
  std::string warn, err;

  std::vector<Vertex>   vertices;
  std::vector<uint32_t> indices;

  auto result = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, "viking_room.obj");
  assert(result);
  std::cout << "Warning:" << warn << '\n';
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

  vulkan::MemoryAllocation vbo_allocation = {};
  VkBuffer vbo = VK_NULL_HANDLE;
  {
    vulkan::BufferCreateInfo info = {};
    info.usage      = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    info.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    info.size       = vertices.size() * sizeof vertices[0];
    vbo = vulkan::create_buffer(context, allocator, info, vbo_allocation);
    vulkan::write_buffer(context, allocator, vbo, vbo_allocation, vertices.data());
  }

  vulkan::MemoryAllocation ibo_allocation = {};
  VkBuffer ibo = VK_NULL_HANDLE;
  {
    vulkan::BufferCreateInfo info = {};
    info.usage      = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    info.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    info.size       = indices.size() * sizeof indices[0];
    ibo = vulkan::create_buffer(context, allocator, info, ibo_allocation);
    vulkan::write_buffer(context, allocator, ibo, ibo_allocation, indices.data());
  }

  VkImageView texture_image_view;
  {
    VkImageViewCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    create_info.image                           = image;
    create_info.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
    create_info.format                          = VK_FORMAT_R8G8B8A8_SRGB;
    create_info.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    create_info.subresourceRange.baseMipLevel   = 0;
    create_info.subresourceRange.levelCount     = 1;
    create_info.subresourceRange.baseArrayLayer = 0;
    create_info.subresourceRange.layerCount     = 1;
    VK_CHECK(vkCreateImageView(context.device, &create_info, nullptr, &texture_image_view));
  }

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

  static constexpr size_t MAX_FRAME_IN_FLIGHT = 4;
  RenderResource render_resources[MAX_FRAME_IN_FLIGHT];
  for(size_t i=0; i<MAX_FRAME_IN_FLIGHT; ++i)
    render_resources[i] = create_render_resouce(context, allocator);

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

  std::fill(std::begin(descriptor_set_layouts), std::end(descriptor_set_layouts), render_context.descriptor_set_layout);

  VkDescriptorSetAllocateInfo alloc_info = {};
  alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  alloc_info.descriptorPool     = descriptor_pool;
  alloc_info.descriptorSetCount = MAX_FRAME_IN_FLIGHT;
  alloc_info.pSetLayouts        = descriptor_set_layouts;

  VK_CHECK(vkAllocateDescriptorSets(context.device, &alloc_info, descriptor_sets));

  for (size_t i = 0; i < MAX_FRAME_IN_FLIGHT; i++)
  {
    VkDescriptorBufferInfo buffer_info = {};
    buffer_info.buffer = render_resources[i].ubo;
    buffer_info.offset = 0;
    buffer_info.range  = sizeof(UniformBufferObject);

    VkDescriptorImageInfo image_info = {};
    image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    image_info.imageView   = texture_image_view;
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


  while(!glfwWindowShouldClose(window))
  {
    size_t i = 0;
    for(auto& render_resource : render_resources)
    {
      glfwPollEvents();

      auto frame_info = begin_frame(context, render_context, render_resource.semaphore_image_available);
      while(!frame_info)
      {
        std::cout << "Recreating render context\n";
        vkDeviceWaitIdle(context.device);
        vulkan::destroy_render_context(context, allocator, render_context);
        render_context = vulkan::create_render_context(context, allocator, render_context_create_info);
        frame_info = begin_frame(context, render_context, render_resource.semaphore_image_available);
      }

      static auto start_time = std::chrono::high_resolution_clock::now();
      auto current_time = std::chrono::high_resolution_clock::now();
      float time = std::chrono::duration<float, std::chrono::seconds::period>(current_time - start_time).count();

      UniformBufferObject ubo = {};
      ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
      ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
      ubo.proj = glm::perspective(glm::radians(45.0f), render_context.extent.width / (float) render_context.extent.height, 0.1f, 10.0f);
      ubo.proj[1][1] *= -1;
      vulkan::write_buffer(context, allocator, render_resource.ubo, render_resource.ubo_allocation, &ubo);

      {
        // A single frame to have multiple render pass and each render pass would need multiple pipeline
        // so we probably should not bind them in begin render
        // A problem is that to create the framebuffer, we need the render pass
        // so how to abstract over them?
        // Do we actually need multiple render pass?
        begin_render(context, render_context, render_resource, *frame_info);

        vkCmdBindDescriptorSets(render_resource.command_buffer.handle,
            VK_PIPELINE_BIND_POINT_GRAPHICS, render_context.pipeline_layout, 0, 1,
            &descriptor_sets[i], 0, nullptr);

        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(render_resource.command_buffer.handle, 0, 1, &vbo, offsets);
        vkCmdBindIndexBuffer(render_resource.command_buffer.handle, ibo, 0, VK_INDEX_TYPE_UINT32);

        VkViewport viewport = {};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width  = render_context.extent.width;
        viewport.height = render_context.extent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(render_resource.command_buffer.handle, 0, 1, &viewport);

        VkRect2D scissor = {};
        scissor.offset = {0, 0};
        scissor.extent = render_context.extent;
        vkCmdSetScissor(render_resource.command_buffer.handle, 0, 1, &scissor);

        vkCmdDrawIndexed(render_resource.command_buffer.handle, indices.size(), 1, 0, 0, 0);

        end_render(context, render_resource);
      }

      if(!end_frame(context, render_context, *frame_info, render_resource.semaphore_render_finished))
      {
        std::cout << "Recreating render context\n";
        vkDeviceWaitIdle(context.device);
        vulkan::destroy_render_context(context, allocator, render_context);
        render_context = vulkan::create_render_context(context, allocator, render_context_create_info);
        frame_info = begin_frame(context, render_context, render_resource.semaphore_image_available);
      }

      ++i;
    }
  }

  vkDeviceWaitIdle(context.device);

  glfwDestroyWindow(window);
  glfwTerminate();
}
