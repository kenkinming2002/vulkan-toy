#include "vulkan.hpp"

#include <vulkan/vulkan_core.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vulkan/vulkan.h>

#include <glm/glm.hpp>

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

namespace glfw
{
  std::vector<const char*> get_required_instance_extensions()
  {
    uint32_t instance_extension_count;
    const char** instance_extensions;
    instance_extensions = glfwGetRequiredInstanceExtensions(&instance_extension_count);
    return std::vector<const char*>(&instance_extensions[0], &instance_extensions[instance_extension_count]);
  }
}


template<typename T>
T select(const std::vector<T>& choices, const auto& get_score)
{
  using score_t = std::invoke_result_t<decltype(get_score), T>;

  struct Selection
  {
    score_t score;
    T choice;
  };
  std::optional<Selection> best_selection;
  for(const auto& choice : choices)
  {
    Selection selection = {
      .score  = get_score(choice),
      .choice = choice
    };
    if(!best_selection || selection.score > best_selection->score)
      best_selection = selection;
  }
  assert(best_selection && "No choice given");
  return best_selection->choice;
}

VkExtent2D select_swap_extent(const VkSurfaceCapabilitiesKHR& surface_capabilities, GLFWwindow *window)
{
  if(surface_capabilities.currentExtent.height != std::numeric_limits<uint32_t>::max() &&
     surface_capabilities.currentExtent.width  != std::numeric_limits<uint32_t>::max())
    return surface_capabilities.currentExtent;

  int width, height;
  glfwGetFramebufferSize(window, &width, &height);

  VkExtent2D extent = {};
  extent.width  = width;
  extent.height = height;
  extent.width  = std::clamp(extent.width , surface_capabilities.minImageExtent.width , surface_capabilities.maxImageExtent.width );
  extent.height = std::clamp(extent.height, surface_capabilities.minImageExtent.height, surface_capabilities.maxImageExtent.height);
  return extent;
}

uint32_t select_image_count(const VkSurfaceCapabilitiesKHR& surface_capabilities)
{
  uint32_t image_count = surface_capabilities.minImageCount + 1;
  if(surface_capabilities.maxImageCount != 0)
    image_count = std::min(image_count, surface_capabilities.maxImageCount);
  return image_count;
}

VkSurfaceFormatKHR select_surface_format_khr(const std::vector<VkSurfaceFormatKHR>& surface_formats)
{
  return select(surface_formats, [](const VkSurfaceFormatKHR& surface_format)
  {
    if(surface_format.format == VK_FORMAT_B8G8R8A8_SRGB && surface_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
      return 1;

    return 0;
  });
}

VkPresentModeKHR select_present_mode_khr(const std::vector<VkPresentModeKHR>& present_modes)
{
  return select(present_modes, [](const VkPresentModeKHR& present_mode)
  {
    switch(present_mode)
    {
    case VK_PRESENT_MODE_IMMEDIATE_KHR:    return 0;
    case VK_PRESENT_MODE_FIFO_RELAXED_KHR: return 1;
    case VK_PRESENT_MODE_FIFO_KHR:         return 2;
    case VK_PRESENT_MODE_MAILBOX_KHR:      return 3;
    default: return -1;
    }
  });
}

std::vector<char> read_file(const char* file_name)
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

VkRenderPass create_render_pass(VkDevice device, VkFormat format)
{
  VkAttachmentDescription color_attachment = {};
  color_attachment.format         = format;
  color_attachment.samples        = VK_SAMPLE_COUNT_1_BIT;
  color_attachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
  color_attachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
  color_attachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  color_attachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
  color_attachment.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentReference color_attachment_reference = {};
  color_attachment_reference.attachment = 0;
  color_attachment_reference.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass_description = {};
  subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass_description.colorAttachmentCount = 1;
  subpass_description.pColorAttachments    = &color_attachment_reference;

  VkRenderPassCreateInfo render_pass_create_info = {};
  render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  render_pass_create_info.attachmentCount = 1;
  render_pass_create_info.pAttachments    = &color_attachment;
  render_pass_create_info.subpassCount    = 1;
  render_pass_create_info.pSubpasses      = &subpass_description;

  VkSubpassDependency subpass_dependency = {};
  subpass_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  subpass_dependency.dstSubpass = 0;
  subpass_dependency.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  subpass_dependency.srcAccessMask = 0;
  subpass_dependency.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  subpass_dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  render_pass_create_info.dependencyCount = 1;
  render_pass_create_info.pDependencies   = &subpass_dependency;

  VkRenderPass render_pass = VK_NULL_HANDLE;
  VK_CHECK(vkCreateRenderPass(device, &render_pass_create_info, nullptr, &render_pass));
  return render_pass;
}

void destroy_render_pass(VkDevice device, VkRenderPass render_pass)
{
  vkDestroyRenderPass(device, render_pass, nullptr);
}

// Question: What even is important when creating a pipeline
// Answer:
// *: Render subpass
//
// 1: vertex layout description
// 2: shaders
// 3: A lot of misc state config
//  - input assembly
//  - viewport
//  - rasterizer
//  - multi-sampling
//  - blending
//  - dynamic state
//
//

// Describe layout of vertex attribute in a single buffer
struct VertexAttributeDescription
{
  enum class Type
  {
    FLOAT1, FLOAT2, FLOAT3, FLOAT4 // Which maniac who not use float as vertex input anyway?
  };

  size_t offset;
  Type type;
};

// You need multiple binding description if you use multiple buffer
struct VertexBindingDescription
{
  size_t stride;
  std::vector<VertexAttributeDescription> attribute_descriptions;
};

VkPipeline create_pipeline(VkDevice device, VkRenderPass render_pass, const std::vector<VertexBindingDescription>& vertex_binding_descriptions)
{
  auto vert_shader_module = vulkan::create_shader_module(device, read_file("shaders/vert.spv"));
  auto frag_shader_module = vulkan::create_shader_module(device, read_file("shaders/frag.spv"));

  VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info = {};
  vertex_input_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertex_input_state_create_info.vertexBindingDescriptionCount   = 0;
  vertex_input_state_create_info.pVertexBindingDescriptions      = nullptr;
  vertex_input_state_create_info.vertexAttributeDescriptionCount = 0;
  vertex_input_state_create_info.pVertexAttributeDescriptions    = nullptr;

  std::vector<VkVertexInputBindingDescription>   vertex_input_binding_descriptions;
  std::vector<VkVertexInputAttributeDescription> vertex_input_attribute_descriptions;
  {
    uint32_t binding = 0;
    for(const auto& vertex_binding_description : vertex_binding_descriptions)
    {
      uint32_t location = 0;
      for(const auto& vertex_attribute_description : vertex_binding_description.attribute_descriptions)
      {
        // Vertex input attribute
        VkVertexInputAttributeDescription vertex_input_attribute_description = {};
        vertex_input_attribute_description.binding  = binding;
        vertex_input_attribute_description.location = location;

        switch(vertex_attribute_description.type)
        {
        case VertexAttributeDescription::Type::FLOAT1: vertex_input_attribute_description.format = VK_FORMAT_R32_SFLOAT;          break;
        case VertexAttributeDescription::Type::FLOAT2: vertex_input_attribute_description.format = VK_FORMAT_R32G32_SFLOAT;       break;
        case VertexAttributeDescription::Type::FLOAT3: vertex_input_attribute_description.format = VK_FORMAT_R32G32B32_SFLOAT;    break;
        case VertexAttributeDescription::Type::FLOAT4: vertex_input_attribute_description.format = VK_FORMAT_R32G32B32A32_SFLOAT; break;
        default:
          fprintf(stderr, "Unknown vertex attribute type\n");
          abort();
        }
        vertex_input_attribute_description.offset   = vertex_attribute_description.offset;
        vertex_input_attribute_descriptions.push_back(vertex_input_attribute_description);

        ++location;
      }

      VkVertexInputBindingDescription vertex_input_binding_description = {};
      vertex_input_binding_description.binding   = binding;
      vertex_input_binding_description.stride    = vertex_binding_description.stride;
      vertex_input_binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
      vertex_input_binding_descriptions.push_back(vertex_input_binding_description);

      ++binding;
    }
  }

  vertex_input_state_create_info.vertexBindingDescriptionCount = vertex_input_binding_descriptions.size();
  vertex_input_state_create_info.pVertexBindingDescriptions    = vertex_input_binding_descriptions.data();

  vertex_input_state_create_info.vertexAttributeDescriptionCount = vertex_input_attribute_descriptions.size();
  vertex_input_state_create_info.pVertexAttributeDescriptions    = vertex_input_attribute_descriptions.data();

  VkPipelineInputAssemblyStateCreateInfo input_assembly_state_create_info = {};
  input_assembly_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  input_assembly_state_create_info.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  input_assembly_state_create_info.primitiveRestartEnable = VK_FALSE;

  VkPipelineShaderStageCreateInfo vert_shader_stage_create_info = {};
  vert_shader_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vert_shader_stage_create_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
  vert_shader_stage_create_info.module = vert_shader_module;
  vert_shader_stage_create_info.pName = "main";

  VkPipelineShaderStageCreateInfo frag_shader_stage_create_info = {};
  frag_shader_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  frag_shader_stage_create_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  frag_shader_stage_create_info.module = frag_shader_module;
  frag_shader_stage_create_info.pName = "main";

  VkPipelineShaderStageCreateInfo shader_stage_create_infos[] = {
    vert_shader_stage_create_info,
    frag_shader_stage_create_info
  };

  VkPipelineViewportStateCreateInfo pipeline_viewport_state_create_info = {};
  pipeline_viewport_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  pipeline_viewport_state_create_info.viewportCount = 1;
  pipeline_viewport_state_create_info.scissorCount  = 1;

  VkPipelineRasterizationStateCreateInfo rasterizer_state_create_info = {};
  rasterizer_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizer_state_create_info.depthClampEnable        = VK_FALSE;
  rasterizer_state_create_info.polygonMode             = VK_POLYGON_MODE_FILL;
  rasterizer_state_create_info.lineWidth               = 1.0f;
  rasterizer_state_create_info.cullMode                = VK_CULL_MODE_BACK_BIT;
  rasterizer_state_create_info.frontFace               = VK_FRONT_FACE_CLOCKWISE;
  rasterizer_state_create_info.depthBiasEnable         = VK_FALSE;
  rasterizer_state_create_info.depthBiasConstantFactor = 0.0f; // Optional
  rasterizer_state_create_info.depthBiasClamp          = 0.0f; // Optional
  rasterizer_state_create_info.depthBiasSlopeFactor    = 0.0f; // Optional

  VkPipelineMultisampleStateCreateInfo multisampling_state_create_info = {};
  multisampling_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampling_state_create_info.sampleShadingEnable   = VK_FALSE;
  multisampling_state_create_info.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT;
  multisampling_state_create_info.minSampleShading      = 1.0f;     // Optional
  multisampling_state_create_info.pSampleMask           = nullptr;  // Optional
  multisampling_state_create_info.alphaToCoverageEnable = VK_FALSE; // Optional
  multisampling_state_create_info.alphaToOneEnable      = VK_FALSE; // Optiona

  // No need for depth and stencil testing using VkPipelineDepthStencilStateCreateInfo

  VkPipelineColorBlendAttachmentState color_blend_attachment_state = {};
  color_blend_attachment_state.colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  color_blend_attachment_state.blendEnable         = VK_FALSE;
  color_blend_attachment_state.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
  color_blend_attachment_state.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
  color_blend_attachment_state.colorBlendOp        = VK_BLEND_OP_ADD; // Optional
  color_blend_attachment_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
  color_blend_attachment_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
  color_blend_attachment_state.alphaBlendOp        = VK_BLEND_OP_ADD; // Optional

  VkPipelineColorBlendStateCreateInfo color_blending_state_create_info = {};
  color_blending_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  color_blending_state_create_info.logicOpEnable     = VK_FALSE;
  color_blending_state_create_info.logicOp           = VK_LOGIC_OP_COPY; // Optional
  color_blending_state_create_info.attachmentCount   = 1;
  color_blending_state_create_info.pAttachments      = &color_blend_attachment_state;
  color_blending_state_create_info.blendConstants[0] = 0.0f; // Optional
  color_blending_state_create_info.blendConstants[1] = 0.0f; // Optional
  color_blending_state_create_info.blendConstants[2] = 0.0f; // Optional
  color_blending_state_create_info.blendConstants[3] = 0.0f; // Optional

  auto dynamic_states = std::vector<VkDynamicState>{
    VK_DYNAMIC_STATE_VIEWPORT,
    VK_DYNAMIC_STATE_SCISSOR
  };

  VkPipelineDynamicStateCreateInfo dynamic_state_create_info = {};
  dynamic_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamic_state_create_info.dynamicStateCount = dynamic_states.size();
  dynamic_state_create_info.pDynamicStates    = dynamic_states.data();

  auto pipeline_layout = vulkan::create_empty_pipeline_layout(device);

  VkGraphicsPipelineCreateInfo graphics_pipeline_create_info = {};
  graphics_pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  graphics_pipeline_create_info.stageCount = 2;
  graphics_pipeline_create_info.pStages    = shader_stage_create_infos;
  graphics_pipeline_create_info.pVertexInputState   = &vertex_input_state_create_info;
  graphics_pipeline_create_info.pInputAssemblyState = &input_assembly_state_create_info;
  graphics_pipeline_create_info.pViewportState      = &pipeline_viewport_state_create_info;
  graphics_pipeline_create_info.pRasterizationState = &rasterizer_state_create_info;
  graphics_pipeline_create_info.pMultisampleState   = &multisampling_state_create_info;
  graphics_pipeline_create_info.pDepthStencilState  = nullptr;
  graphics_pipeline_create_info.pColorBlendState    = &color_blending_state_create_info;
  graphics_pipeline_create_info.pDynamicState       = &dynamic_state_create_info;
  graphics_pipeline_create_info.layout              = pipeline_layout;

  graphics_pipeline_create_info.renderPass = render_pass;
  graphics_pipeline_create_info.subpass    = 0;

  graphics_pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
  graphics_pipeline_create_info.basePipelineIndex  = -1;

  VkPipeline pipeline = VK_NULL_HANDLE;
  VK_CHECK(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &graphics_pipeline_create_info, nullptr, &pipeline));
  vulkan::destroy_shader_module(device, vert_shader_module);
  vulkan::destroy_shader_module(device, frag_shader_module);
  vulkan::destroy_pipeline_layout(device, pipeline_layout);
  return pipeline;
}

void destroy_pipeline(VkDevice device, VkPipeline pipeline)
{
  vkDestroyPipeline(device, pipeline, nullptr);
}

struct Context
{
  VkInstance       instance;

  VkPhysicalDevice physical_device;
  uint32_t         queue_family_index;

  VkDevice device;
  VkQueue  queue;

  VkSurfaceKHR   surface;
  VkSwapchainKHR swapchain;
  VkFormat       format;
  VkExtent2D     extent;

  VkRenderPass render_pass;
  VkPipeline   pipeline;

  std::vector<VkImage>       images;
  std::vector<VkImageView>   image_views;
  std::vector<VkFramebuffer> framebuffers;

  VkCommandPool command_pool;
};

struct ContextCreateInfo
{
  const char *application_name;
  uint32_t    application_version;

  const char *engine_name;
  uint32_t    engine_version;

  GLFWwindow *window;
};

struct Vertex
{
  glm::vec2 pos;
  glm::vec3 color;
};

Context create_context(ContextCreateInfo create_info)
{
  auto required_layers              = std::vector<const char*>{"VK_LAYER_KHRONOS_validation"};
  auto required_instance_extensions = glfw::get_required_instance_extensions();
  auto required_device_extensions   = std::vector<const char*>{VK_KHR_SWAPCHAIN_EXTENSION_NAME};

  Context context = {};
  context.instance = vulkan::create_instance(
      "Vulkan", VK_MAKE_VERSION(1, 0, 0),
      "Engine", VK_MAKE_VERSION(1, 0, 0),
      required_instance_extensions, required_layers
  );

  context.surface = vulkan::create_surface(context.instance, create_info.window);
  context.physical_device    = vulkan::enumerate_physical_devices(context.instance).at(0);
  context.queue_family_index = 0;
  context.device = vulkan::create_device(
      context.physical_device, { context.queue_family_index }, {},
      required_device_extensions, required_layers
  );
  context.queue = vulkan::device_get_queue(context.device, context.queue_family_index, 0);

  auto capabilities    = vulkan::get_physical_device_surface_capabilities_khr(context.physical_device, context.surface);
  auto surface_formats = vulkan::get_physical_device_surface_formats_khr(context.physical_device, context.surface);
  auto present_modes   = vulkan::get_physical_device_surface_present_modes_khr(context.physical_device, context.surface);

  auto extent         = select_swap_extent(capabilities, create_info.window);
  auto image_count    = select_image_count(capabilities);
  auto surface_format = select_surface_format_khr(surface_formats);
  auto present_mode   = select_present_mode_khr(present_modes);
  context.swapchain = vulkan::create_swapchain_khr(context.device, context.surface, extent, image_count, surface_format, present_mode, capabilities.currentTransform);
  context.format = surface_format.format;
  context.extent = extent;

  context.render_pass = create_render_pass(context.device, context.format);
  context.pipeline    = create_pipeline(context.device, context.render_pass, {
    VertexBindingDescription{
      .stride = sizeof(Vertex),
      .attribute_descriptions = {
        VertexAttributeDescription{
          .offset = offsetof(Vertex, pos),
          .type = VertexAttributeDescription::Type::FLOAT2,
        },
        VertexAttributeDescription{
          .offset = offsetof(Vertex, color),
          .type = VertexAttributeDescription::Type::FLOAT3,
        },
      }
    }
  });

  context.images = vulkan::swapchain_get_images(context.device, context.swapchain);
  for(const auto& image : context.images)
    context.image_views.push_back(vulkan::create_image_view(context.device, image, context.format));

  for(const auto& image_view : context.image_views)
    context.framebuffers.push_back(vulkan::create_framebuffer(context.device, context.render_pass, image_view, extent));

  context.command_pool = vulkan::create_command_pool(context.device, 0);

  return context;
}

struct FrameInfo
{
  uint32_t image_index;
  VkFramebuffer framebuffer;
};

FrameInfo begin_frame(const Context& context, VkSemaphore semaphore)
{
  FrameInfo frame_info = {};
  vkAcquireNextImageKHR(context.device, context.swapchain, UINT64_MAX, semaphore, VK_NULL_HANDLE, &frame_info.image_index);
  frame_info.framebuffer = context.framebuffers[frame_info.image_index];
  return frame_info;
}

void end_frame(const Context& context, FrameInfo frame_info, VkSemaphore semaphore)
{
  VkPresentInfoKHR present_info = {};
  present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  present_info.waitSemaphoreCount = 1;
  present_info.pWaitSemaphores = &semaphore;

  VkSwapchainKHR swapchains[] = { context.swapchain };
  present_info.swapchainCount = 1;
  present_info.pSwapchains    = swapchains;
  present_info.pImageIndices  = &frame_info.image_index;
  present_info.pResults       = nullptr;

  vkQueuePresentKHR(context.queue, &present_info);
}

struct RenderResource
{
  VkCommandBuffer command_buffer;
  VkSemaphore semaphore_image_available;
  VkSemaphore semaphore_render_finished;
  VkFence in_flight_fence;
};

RenderResource create_render_resouce(const Context& context)
{
  RenderResource render_resource = {};
  render_resource.command_buffer            = vulkan::create_command_buffer(context.device, context.command_pool);
  render_resource.semaphore_image_available = vulkan::create_semaphore(context.device);
  render_resource.semaphore_render_finished = vulkan::create_semaphore(context.device);
  render_resource.in_flight_fence           = vulkan::create_fence(context.device, true);
  return render_resource;
}

void begin_render(const Context& context, const RenderResource& render_resource, const FrameInfo& frame_info)
{
  vkWaitForFences(context.device, 1, &render_resource.in_flight_fence, VK_TRUE, UINT64_MAX);
  vkResetFences(context.device, 1, &render_resource.in_flight_fence);

  VK_CHECK(vkResetCommandBuffer(render_resource.command_buffer, 0));

  VkCommandBufferBeginInfo begin_info = {};
  begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  VK_CHECK(vkBeginCommandBuffer(render_resource.command_buffer, &begin_info));

  VkRenderPassBeginInfo render_pass_begin_info = {};
  render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  render_pass_begin_info.renderPass        = context.render_pass;
  render_pass_begin_info.framebuffer       = frame_info.framebuffer;
  render_pass_begin_info.renderArea.offset = {0, 0};
  render_pass_begin_info.renderArea.extent = context.extent;

  VkClearValue clear_color = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
  render_pass_begin_info.clearValueCount = 1;
  render_pass_begin_info.pClearValues = &clear_color;

  vkCmdBeginRenderPass(render_resource.command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
  vkCmdBindPipeline(render_resource.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, context.pipeline);
}

void end_render(const Context& context, const RenderResource& render_resource)
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

int main()
{
  glfwInit();

  // Window and KHR API
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
  GLFWwindow *window = glfwCreateWindow(800, 600, "Vulkan", nullptr, nullptr);

  ContextCreateInfo create_info = {};
  create_info.application_name    = "Vulkan";
  create_info.application_version = VK_MAKE_VERSION(1, 0, 0);
  create_info.engine_name         = "Engine";
  create_info.engine_version      = VK_MAKE_VERSION(1, 0, 0);
  create_info.window              = window;
  auto context = create_context(create_info);

  const std::vector<Vertex> vertices = {
    {{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
    {{0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}},
    {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
  };

  // Create the vertex buffer
  VkBufferCreateInfo buffer_create_info = {};
  buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  buffer_create_info.size        = sizeof vertices[0] * vertices.size();
  buffer_create_info.usage       = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
  buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VkBuffer buffer = VK_NULL_HANDLE;
  VK_CHECK(vkCreateBuffer(context.device, &buffer_create_info, nullptr, &buffer));

  VkMemoryRequirements buffer_memory_requirement = {};
  vkGetBufferMemoryRequirements(context.device, buffer, &buffer_memory_requirement);

  VkPhysicalDeviceMemoryProperties physical_device_memory_properties = {};
  vkGetPhysicalDeviceMemoryProperties(context.physical_device, &physical_device_memory_properties);

  // How is it different from picking lowest significant set bit of type filter
  uint32_t              type_filter       = buffer_memory_requirement.memoryTypeBits;
  VkMemoryPropertyFlags memory_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

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

  VkDeviceMemory device_memory = VK_NULL_HANDLE;
  VK_CHECK(vkAllocateMemory(context.device, &allocate_info, nullptr, &device_memory));
  VK_CHECK(vkBindBufferMemory(context.device, buffer, device_memory, 0));

  void *data;
  VK_CHECK(vkMapMemory(context.device, device_memory, 0, buffer_memory_requirement.size, 0, &data));
  memcpy(data, vertices.data(), buffer_create_info.size);
  vkUnmapMemory(context.device, device_memory);

  static constexpr size_t MAX_FRAME_IN_FLIGHT = 4;
  RenderResource render_resources[MAX_FRAME_IN_FLIGHT];
  for(size_t i=0; i<MAX_FRAME_IN_FLIGHT; ++i)
    render_resources[i] = create_render_resouce(context);

  while(!glfwWindowShouldClose(window))
    for(const auto& render_resource : render_resources)
    {
      glfwPollEvents();

      auto frame_info = begin_frame(context, render_resource.semaphore_image_available);
      {
        begin_render(context, render_resource, frame_info);

        VkBuffer buffers[] = { buffer };
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(render_resource.command_buffer, 0, 1, buffers, offsets);

        VkViewport viewport = {};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width  = context.extent.width;
        viewport.height = context.extent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(render_resource.command_buffer, 0, 1, &viewport);

        VkRect2D scissor = {};
        scissor.offset = {0, 0};
        scissor.extent = context.extent;
        vkCmdSetScissor(render_resource.command_buffer, 0, 1, &scissor);

        vkCmdDraw(render_resource.command_buffer, 3, 1, 0, 0);

        end_render(context, render_resource);
      }
      end_frame(context, frame_info, render_resource.semaphore_render_finished);

    }

  vkDeviceWaitIdle(context.device);

  glfwDestroyWindow(window);
  glfwTerminate();
}
