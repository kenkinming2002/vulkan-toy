#include "context.hpp"
#include "render_context.hpp"
#include "vulkan.hpp"

#include <vulkan/vulkan_core.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vulkan/vulkan.h>

#define GLM_FORCE_RADIANS
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

#define VK_CHECK(expr) do { if(expr != VK_SUCCESS) { fprintf(stderr, "Vulkan pooped itself:%s\n", #expr); } } while(0)

VkBuffer allocate_buffer(const vulkan::Context& context, VkDeviceSize size, VkBufferUsageFlags buffer_usage, VkMemoryPropertyFlags memory_properties, VkDeviceMemory& device_memory)
{
  VkBufferCreateInfo buffer_create_info = {};
  buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  buffer_create_info.size        = size;
  buffer_create_info.usage       = buffer_usage;
  buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VkBuffer buffer = VK_NULL_HANDLE;
  VK_CHECK(vkCreateBuffer(context.device, &buffer_create_info, nullptr, &buffer));

  VkMemoryRequirements buffer_memory_requirement = {};
  vkGetBufferMemoryRequirements(context.device, buffer, &buffer_memory_requirement);

  VkPhysicalDeviceMemoryProperties physical_device_memory_properties = {};
  vkGetPhysicalDeviceMemoryProperties(context.physical_device, &physical_device_memory_properties);

  // How is it different from picking lowest significant set bit of type filter
  uint32_t type_filter       = buffer_memory_requirement.memoryTypeBits;
  uint32_t memory_type_index = [&]()
  {
    for (uint32_t i = 0; i < physical_device_memory_properties.memoryTypeCount; i++)
      if (type_filter & (1 << i) && (memory_properties & physical_device_memory_properties.memoryTypes[i].propertyFlags) == memory_properties)
        return i;

    fprintf(stderr, "No memory type suitable");
    abort();
  }();

  VkMemoryAllocateInfo allocate_info = {};
  allocate_info.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocate_info.memoryTypeIndex = memory_type_index;
  allocate_info.allocationSize  = buffer_memory_requirement.size;

  VK_CHECK(vkAllocateMemory(context.device, &allocate_info, nullptr, &device_memory));
  VK_CHECK(vkBindBufferMemory(context.device, buffer, device_memory, 0));

  return buffer;
}


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

  frame_info.framebuffer = render_context.framebuffers[frame_info.image_index];
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
  glm::vec2 pos;
  glm::vec3 color;
};


struct RenderResource
{
  VkCommandBuffer command_buffer;

  VkSemaphore semaphore_image_available;
  VkSemaphore semaphore_render_finished;
  VkFence in_flight_fence;

  VkBuffer       ubo_buffer;
  VkDeviceMemory ubo_memory;
};

RenderResource create_render_resouce(const vulkan::Context& context)
{
  RenderResource render_resource = {};
  render_resource.command_buffer            = vulkan::create_command_buffer(context.device, context.command_pool);
  render_resource.semaphore_image_available = vulkan::create_semaphore(context.device);
  render_resource.semaphore_render_finished = vulkan::create_semaphore(context.device);
  render_resource.in_flight_fence           = vulkan::create_fence(context.device, true);
  render_resource.ubo_buffer = allocate_buffer(context, sizeof(UniformBufferObject),
      VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      render_resource.ubo_memory);
  return render_resource;
}

void begin_render(const vulkan::Context& context, const vulkan::RenderContext& render_context, const RenderResource& render_resource, const FrameInfo& frame_info)
{
  vkWaitForFences(context.device, 1, &render_resource.in_flight_fence, VK_TRUE, UINT64_MAX);
  vkResetFences(context.device, 1, &render_resource.in_flight_fence);

  VK_CHECK(vkResetCommandBuffer(render_resource.command_buffer, 0));

  VkCommandBufferBeginInfo begin_info = {};
  begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  VK_CHECK(vkBeginCommandBuffer(render_resource.command_buffer, &begin_info));

  VkRenderPassBeginInfo render_pass_begin_info = {};
  render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  render_pass_begin_info.renderPass        = render_context.render_pass;
  render_pass_begin_info.framebuffer       = frame_info.framebuffer;
  render_pass_begin_info.renderArea.offset = {0, 0};
  render_pass_begin_info.renderArea.extent = render_context.extent;

  VkClearValue clear_color = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
  render_pass_begin_info.clearValueCount = 1;
  render_pass_begin_info.pClearValues = &clear_color;

  vkCmdBeginRenderPass(render_resource.command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
  vkCmdBindPipeline(render_resource.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, render_context.pipeline);
}

void end_render(const vulkan::Context& context, const RenderResource& render_resource)
{
  vkCmdEndRenderPass(render_resource.command_buffer);
  VK_CHECK(vkEndCommandBuffer(render_resource.command_buffer));

  VkSubmitInfo submit_info = {};
  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

  submit_info.waitSemaphoreCount = 1;
  submit_info.pWaitSemaphores    = &render_resource.semaphore_image_available;

  VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
  submit_info.pWaitDstStageMask  = wait_stages;

  submit_info.signalSemaphoreCount = 1;
  submit_info.pSignalSemaphores    = &render_resource.semaphore_render_finished;

  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers    = &render_resource.command_buffer;

  VK_CHECK(vkQueueSubmit(context.queue, 1, &submit_info, render_resource.in_flight_fence));
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

  vulkan::ContextCreateInfo context_create_info = {};
  context_create_info.application_name    = "Vulkan";
  context_create_info.application_version = VK_MAKE_VERSION(1, 0, 0);
  context_create_info.engine_name         = "Engine";
  context_create_info.engine_version      = VK_MAKE_VERSION(1, 0, 0);
  context_create_info.window              = window;
  auto context = create_context(context_create_info);

  // May need to be recreated on window resize
  vulkan::RenderContextCreateInfo render_context_create_info = {};
  vulkan::PipelineCreateInfo pipeline_create_info = {};
  pipeline_create_info.vert_shader_module = vulkan::create_shader_module(context.device, read_file("shaders/vert.spv"));
  pipeline_create_info.frag_shader_module = vulkan::create_shader_module(context.device, read_file("shaders/frag.spv"));
  pipeline_create_info.vertex_binding_descriptions = {
    vulkan::VertexBindingDescription{
      .stride = sizeof(Vertex),
      .attribute_descriptions = {
        vulkan::VertexAttributeDescription{
          .offset = offsetof(Vertex, pos),
          .type = vulkan::VertexAttributeDescription::Type::FLOAT2,
        },
        vulkan::VertexAttributeDescription{
          .offset = offsetof(Vertex, color),
          .type = vulkan::VertexAttributeDescription::Type::FLOAT3,
        },
      }
    }
  };
  render_context_create_info.pipeline = pipeline_create_info;

  auto render_context = create_render_context(context, render_context_create_info);
  const std::vector<Vertex> vertices = {
    {{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
    {{0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}},
    {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
  };

  // Create the vertex buffer
  const size_t buffer_size = sizeof vertices[0] * vertices.size();

  VkDeviceMemory staging_buffer_memory = VK_NULL_HANDLE;
  VkDeviceMemory buffer_memory = VK_NULL_HANDLE;

  auto staging_buffer = allocate_buffer(context, buffer_size,
      VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      staging_buffer_memory);

  void *data;
  VK_CHECK(vkMapMemory(context.device, staging_buffer_memory, 0, buffer_size, 0, &data));
  memcpy(data, vertices.data(), buffer_size);
  vkUnmapMemory(context.device, staging_buffer_memory);

  auto buffer = allocate_buffer(context, buffer_size,
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      buffer_memory);

  // Single shot buffer command
  {
    auto single_shot_command_buffer = vulkan::create_command_buffer(context.device, context.command_pool);

    VkCommandBufferBeginInfo commad_buffer_begin_info{};
    commad_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    commad_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(single_shot_command_buffer, &commad_buffer_begin_info);

    VkBufferCopy buffer_copy = {};
    buffer_copy.srcOffset = 0;
    buffer_copy.dstOffset = 0;
    buffer_copy.size      = buffer_size;
    vkCmdCopyBuffer(single_shot_command_buffer, staging_buffer, buffer, 1, &buffer_copy);

    vkEndCommandBuffer(single_shot_command_buffer);

    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers    = &single_shot_command_buffer;

    VK_CHECK(vkQueueSubmit(context.queue, 1, &submit_info, VK_NULL_HANDLE));
    VK_CHECK(vkQueueWaitIdle(context.queue));

    vulkan::destroy_command_buffer(context.device, context.command_pool, single_shot_command_buffer);
  }

  vkDestroyBuffer(context.device, staging_buffer, nullptr);
  vkFreeMemory(context.device, staging_buffer_memory, nullptr);

  static constexpr size_t MAX_FRAME_IN_FLIGHT = 4;
  RenderResource render_resources[MAX_FRAME_IN_FLIGHT];
  for(size_t i=0; i<MAX_FRAME_IN_FLIGHT; ++i)
    render_resources[i] = create_render_resouce(context);

  // Descriptor pool
  VkDescriptorPoolSize pool_size = {};
  pool_size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  pool_size.descriptorCount = MAX_FRAME_IN_FLIGHT;

  VkDescriptorPoolCreateInfo pool_create_info = {};
  pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  pool_create_info.poolSizeCount = 1;
  pool_create_info.pPoolSizes = &pool_size;
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
    buffer_info.buffer = render_resources[i].ubo_buffer;
    buffer_info.offset = 0;
    buffer_info.range  = sizeof(UniformBufferObject);

    VkWriteDescriptorSet descriptor_write{};
    descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptor_write.dstSet          = descriptor_sets[i];
    descriptor_write.dstBinding      = 0;
    descriptor_write.dstArrayElement = 0;
    descriptor_write.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptor_write.descriptorCount = 1;
    descriptor_write.pBufferInfo     = &buffer_info;

    vkUpdateDescriptorSets(context.device, 1, &descriptor_write, 0, nullptr);
  }


  while(!glfwWindowShouldClose(window))
  {
    size_t i = 0;
    for(const auto& render_resource : render_resources)
    {
      glfwPollEvents();

      auto frame_info = begin_frame(context, render_context, render_resource.semaphore_image_available);
      while(!frame_info)
      {
        std::cout << "Recreating render context\n";
        vkDeviceWaitIdle(context.device);
        vulkan::destroy_render_context(context, render_context);
        render_context = vulkan::create_render_context(context, render_context_create_info);
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

      void* data;
      vkMapMemory(context.device, render_resource.ubo_memory, 0, sizeof(ubo), 0, &data);
      memcpy(data, &ubo, sizeof(ubo));
      vkUnmapMemory(context.device, render_resource.ubo_memory);

      {
        // A single frame to have multiple render pass and each render pass would need multiple pipeline
        // so we probably should not bind them in begin render
        // A problem is that to create the framebuffer, we need the render pass
        // so how to abstract over them?
        // Do we actually need multiple render pass?
        begin_render(context, render_context, render_resource, *frame_info);

        vkCmdBindDescriptorSets(render_resource.command_buffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS, render_context.pipeline_layout, 0, 1,
            &descriptor_sets[i], 0, nullptr);

        VkBuffer buffers[] = { buffer };
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(render_resource.command_buffer, 0, 1, buffers, offsets);

        VkViewport viewport = {};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width  = render_context.extent.width;
        viewport.height = render_context.extent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(render_resource.command_buffer, 0, 1, &viewport);

        VkRect2D scissor = {};
        scissor.offset = {0, 0};
        scissor.extent = render_context.extent;
        vkCmdSetScissor(render_resource.command_buffer, 0, 1, &scissor);

        vkCmdDraw(render_resource.command_buffer, 3, 1, 0, 0);

        end_render(context, render_resource);
      }

      if(!end_frame(context, render_context, *frame_info, render_resource.semaphore_render_finished))
      {
        std::cout << "Recreating render context\n";
        vkDeviceWaitIdle(context.device);
        vulkan::destroy_render_context(context, render_context);
        render_context = vulkan::create_render_context(context, render_context_create_info);
        frame_info = begin_frame(context, render_context, render_resource.semaphore_image_available);
      }

      ++i;
    }
  }

  vkDeviceWaitIdle(context.device);

  glfwDestroyWindow(window);
  glfwTerminate();
}
