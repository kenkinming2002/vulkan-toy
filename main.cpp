#include "vulkan.hpp"

#include <vulkan/vulkan_core.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vulkan/vulkan.h>

#include <utility>
#include <limits>
#include <fstream>
#include <algorithm>
#include <optional>
#include <vector>
#include <iostream>

#include <assert.h>
#include <stdlib.h>

template<typename T>
struct Defer
{
  Defer(T func) : func(func) {}
  ~Defer() { func(); }
  T func;
};

#define VK_CHECK(expr) do { if(expr != VK_SUCCESS) { fprintf(stderr, "Vulkan pooped itself:%s\n", #expr); } } while(0)

#define DEFER__(expr, counter) Defer _defer##counter([&](){ expr; })
#define DEFER_(expr, counter) DEFER__(expr, counter)
#define DEFER(expr) DEFER_(expr, __COUNTER__)

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

VkPipeline create_pipeline(VkDevice device, VkRenderPass render_pass)
{
  auto vert_shader_module = vulkan::create_shader_module(device, read_file("shaders/vert.spv"));
  DEFER(vulkan::destroy_shader_module(device, vert_shader_module));

  auto frag_shader_module = vulkan::create_shader_module(device, read_file("shaders/frag.spv"));
  DEFER(vulkan::destroy_shader_module(device, frag_shader_module));

  VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info = {};
  vertex_input_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertex_input_state_create_info.vertexBindingDescriptionCount   = 0;
  vertex_input_state_create_info.pVertexBindingDescriptions      = nullptr;
  vertex_input_state_create_info.vertexAttributeDescriptionCount = 0;
  vertex_input_state_create_info.pVertexAttributeDescriptions    = nullptr;

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
  DEFER(vulkan::destroy_pipeline_layout(device, pipeline_layout));

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
  return pipeline;
}

void destroy_pipeline(VkDevice device, VkPipeline pipeline)
{
  vkDestroyPipeline(device, pipeline, nullptr);
}

struct Context
{
  VkInstance instance;
  VkDevice   device;
  VkQueue    queue;

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

  auto physical_device = vulkan::enumerate_physical_devices(context.instance).at(0);
  auto queue_family_indices = std::vector<uint32_t>{0};
  context.device = vulkan::create_device(
      physical_device, queue_family_indices, {},
      required_device_extensions, required_layers
  );
  context.queue = vulkan::device_get_queue(context.device, 0, 0);

  auto capabilities    = vulkan::get_physical_device_surface_capabilities_khr(physical_device, context.surface);
  auto surface_formats = vulkan::get_physical_device_surface_formats_khr(physical_device, context.surface);
  auto present_modes   = vulkan::get_physical_device_surface_present_modes_khr(physical_device, context.surface);

  auto extent         = select_swap_extent(capabilities, create_info.window);
  auto image_count    = select_image_count(capabilities);
  auto surface_format = select_surface_format_khr(surface_formats);
  auto present_mode   = select_present_mode_khr(present_modes);
  context.swapchain = vulkan::create_swapchain_khr(context.device, context.surface, extent, image_count, surface_format, present_mode, capabilities.currentTransform);
  context.format = surface_format.format;
  context.extent = extent;

  context.render_pass = create_render_pass(context.device, context.format);
  context.pipeline    = create_pipeline(context.device, context.render_pass);

  context.images = vulkan::swapchain_get_images(context.device, context.swapchain);
  for(const auto& image : context.images)
    context.image_views.push_back(vulkan::create_image_view(context.device, image, context.format));

  for(const auto& image_view : context.image_views)
    context.framebuffers.push_back(vulkan::create_framebuffer(context.device, context.render_pass, image_view, extent));

  context.command_pool = vulkan::create_command_pool(context.device, 0);

  return context;
}

struct RenderInfo
{
  uint32_t image_index;
  VkFramebuffer framebuffer;
};

RenderInfo begin_render_frame(const Context& context, VkSemaphore semaphore)
{
  RenderInfo render_info = {};
  vkAcquireNextImageKHR(context.device, context.swapchain, UINT64_MAX, semaphore, VK_NULL_HANDLE, &render_info.image_index);
  render_info.framebuffer = context.framebuffers[render_info.image_index];
  return render_info;
}

void end_render_frame(const Context& context, RenderInfo render_info, VkSemaphore semaphore)
{
  VkPresentInfoKHR present_info = {};
  present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  present_info.waitSemaphoreCount = 1;
  present_info.pWaitSemaphores = &semaphore;

  VkSwapchainKHR swapchains[] = { context.swapchain };
  present_info.swapchainCount = 1;
  present_info.pSwapchains    = swapchains;
  present_info.pImageIndices  = &render_info.image_index;
  present_info.pResults       = nullptr;

  vkQueuePresentKHR(context.queue, &present_info);
}

int main()
{
  glfwInit();

  // Window and KHR API
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
  GLFWwindow *window = glfwCreateWindow(800, 600, "Vulkan", nullptr, nullptr);

  static constexpr size_t MAX_FRAME_IN_FLIGHT = 4;

  ContextCreateInfo create_info = {};
  create_info.application_name    = "Vulkan";
  create_info.application_version = VK_MAKE_VERSION(1, 0, 0);
  create_info.engine_name         = "Engine";
  create_info.engine_version      = VK_MAKE_VERSION(1, 0, 0);
  create_info.window              = window;
  auto context = create_context(create_info);

  VkCommandBuffer command_buffers[MAX_FRAME_IN_FLIGHT]        = {};
  VkSemaphore image_avail_semaphores[MAX_FRAME_IN_FLIGHT]     = {};
  VkSemaphore render_finished_semaphores[MAX_FRAME_IN_FLIGHT] = {};
  VkFence in_flight_fences[MAX_FRAME_IN_FLIGHT]               = {};
  for(size_t i=0; i<MAX_FRAME_IN_FLIGHT; ++i)
  {
    command_buffers[i]            = vulkan::create_command_buffer(context.device, context.command_pool);
    image_avail_semaphores[i]     = vulkan::create_semaphore(context.device);
    render_finished_semaphores[i] = vulkan::create_semaphore(context.device);
    in_flight_fences[i]           = vulkan::create_fence(context.device, true);
  }

  while(!glfwWindowShouldClose(window))
    for(size_t i=0; i<MAX_FRAME_IN_FLIGHT; ++i)
    {
      glfwPollEvents();

      vkWaitForFences(context.device, 1, &in_flight_fences[i], VK_TRUE, UINT64_MAX);
      vkResetFences(context.device, 1, &in_flight_fences[i]);

      auto render_info = begin_render_frame(context, image_avail_semaphores[i]);
      {
        // Record the command buffer
        VK_CHECK(vkResetCommandBuffer(command_buffers[i], 0));

        VkCommandBufferBeginInfo begin_info = {};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        VK_CHECK(vkBeginCommandBuffer(command_buffers[i], &begin_info));

        VkRenderPassBeginInfo render_pass_begin_info = {};
        render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        render_pass_begin_info.renderPass        = context.render_pass;
        render_pass_begin_info.framebuffer       = render_info.framebuffer;
        render_pass_begin_info.renderArea.offset = {0, 0};
        render_pass_begin_info.renderArea.extent = context.extent;

        VkClearValue clear_color = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
        render_pass_begin_info.clearValueCount = 1;
        render_pass_begin_info.pClearValues = &clear_color;

        vkCmdBeginRenderPass(command_buffers[i], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, context.pipeline);

        VkViewport viewport = {};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width  = context.extent.width;
        viewport.height = context.extent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(command_buffers[i], 0, 1, &viewport);

        VkRect2D scissor = {};
        scissor.offset = {0, 0};
        scissor.extent = context.extent;
        vkCmdSetScissor(command_buffers[i], 0, 1, &scissor);

        vkCmdDraw(command_buffers[i], 3, 1, 0, 0);

        vkCmdEndRenderPass(command_buffers[i]);

        VK_CHECK(vkEndCommandBuffer(command_buffers[i]));

        // Submit the command buffer
        VkSemaphore wait_semaphores[]   = { image_avail_semaphores[i] };
        VkSemaphore signal_semaphores[] = { render_finished_semaphores[i] };

        VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

        VkSubmitInfo submit_info = {};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        submit_info.waitSemaphoreCount = 1;
        submit_info.pWaitSemaphores    = wait_semaphores;

        submit_info.pWaitDstStageMask  = wait_stages;

        submit_info.signalSemaphoreCount = 1;
        submit_info.pSignalSemaphores    = signal_semaphores;

        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers    = &command_buffers[i];

        VK_CHECK(vkQueueSubmit(context.queue, 1, &submit_info, in_flight_fences[i]));

        end_render_frame(context, render_info, render_finished_semaphores[i]);
      }

    }

  vkDeviceWaitIdle(context.device);

  glfwDestroyWindow(window);
  glfwTerminate();
}
