#pragma once

#include "context.hpp"
#include "vulkan.hpp"

#include <glm/glm.hpp>

#include <optional>
#include <algorithm>
#include <fstream>
#include <limits>

#include <stdio.h>
#include <assert.h>

namespace vulkan
{
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

  inline VkExtent2D select_swap_extent(VkSurfaceCapabilitiesKHR surface_capabilities)
  {
    VkExtent2D extent = surface_capabilities.currentExtent;
    if(extent.height == std::numeric_limits<uint32_t>::max() ||
       extent.width  == std::numeric_limits<uint32_t>::max())
    {
      // This should never happen in practice since this means we did not
      // speicify the window size when we create the window. Just use some
      // hard-coded value in this case
      fprintf(stderr, "Window size not specified. Using a default value of 1080x720\n");
      extent = { 1080, 720 };
    }
    extent.width  = std::clamp(extent.width , surface_capabilities.minImageExtent.width , surface_capabilities.maxImageExtent.width );
    extent.height = std::clamp(extent.height, surface_capabilities.minImageExtent.height, surface_capabilities.maxImageExtent.height);
    return extent;
  }

  inline uint32_t select_image_count(const VkSurfaceCapabilitiesKHR& surface_capabilities)
  {
    uint32_t image_count = surface_capabilities.minImageCount + 1;
    if(surface_capabilities.maxImageCount != 0)
      image_count = std::min(image_count, surface_capabilities.maxImageCount);
    return image_count;
  }

  inline VkSurfaceFormatKHR select_surface_format_khr(const std::vector<VkSurfaceFormatKHR>& surface_formats)
  {
    return select(surface_formats, [](const VkSurfaceFormatKHR& surface_format)
    {
      if(surface_format.format == VK_FORMAT_B8G8R8A8_SRGB && surface_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        return 1;

      return 0;
    });
  }

  inline VkPresentModeKHR select_present_mode_khr(const std::vector<VkPresentModeKHR>& present_modes)
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

  inline VkRenderPass create_render_pass(VkDevice device, VkFormat format)
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

  inline void destroy_render_pass(VkDevice device, VkRenderPass render_pass)
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

  struct PipelineCreateInfo
  {
    VkShaderModule vert_shader_module;
    VkShaderModule frag_shader_module;
    std::vector<VertexBindingDescription> vertex_binding_descriptions;
  };

  inline VkPipeline create_pipeline(VkDevice device, VkRenderPass render_pass, PipelineCreateInfo create_info)
  {
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
      for(const auto& vertex_binding_description : create_info.vertex_binding_descriptions)
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
    vert_shader_stage_create_info.module = create_info.vert_shader_module;
    vert_shader_stage_create_info.pName = "main";

    VkPipelineShaderStageCreateInfo frag_shader_stage_create_info = {};
    frag_shader_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    frag_shader_stage_create_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    frag_shader_stage_create_info.module = create_info.frag_shader_module;
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
    vulkan::destroy_pipeline_layout(device, pipeline_layout);
    return pipeline;
  }

  inline void destroy_pipeline(VkDevice device, VkPipeline pipeline)
  {
    vkDestroyPipeline(device, pipeline, nullptr);
  }

  struct RenderContext
  {
    VkSwapchainKHR swapchain;
    VkFormat       format;
    VkExtent2D     extent;

    // TODO: How to support multiple render pass
    VkRenderPass render_pass;
    VkPipeline   pipeline;

    std::vector<VkImage>       images;
    std::vector<VkImageView>   image_views;
    std::vector<VkFramebuffer> framebuffers;
  };

  struct RenderContextCreateInfo
  {
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

  // TODO: Move this outside
  struct Vertex
  {
    glm::vec2 pos;
    glm::vec3 color;
  };

  inline RenderContext create_render_context(const Context& context, RenderContextCreateInfo create_info)
  {
    RenderContext render_context = {};

    auto capabilities    = vulkan::get_physical_device_surface_capabilities_khr(context.physical_device, context.surface);
    auto surface_formats = vulkan::get_physical_device_surface_formats_khr(context.physical_device, context.surface);
    auto present_modes   = vulkan::get_physical_device_surface_present_modes_khr(context.physical_device, context.surface);

    auto extent         = select_swap_extent(capabilities);
    auto image_count    = select_image_count(capabilities);
    auto surface_format = select_surface_format_khr(surface_formats);
    auto present_mode   = select_present_mode_khr(present_modes);

    render_context.swapchain = vulkan::create_swapchain_khr(context.device, context.surface, extent, image_count, surface_format, present_mode, capabilities.currentTransform);
    render_context.format = surface_format.format;
    render_context.extent = extent;
    render_context.render_pass = create_render_pass(context.device, render_context.format);

    PipelineCreateInfo pipeline_create_info = {};
    pipeline_create_info.vert_shader_module = vulkan::create_shader_module(context.device, read_file("shaders/vert.spv"));
    pipeline_create_info.frag_shader_module = vulkan::create_shader_module(context.device, read_file("shaders/frag.spv"));
    pipeline_create_info.vertex_binding_descriptions = {
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
    };
    render_context.pipeline = create_pipeline(context.device, render_context.render_pass, pipeline_create_info);

    render_context.images = vulkan::swapchain_get_images(context.device, render_context.swapchain);
    for(const auto& image : render_context.images)
      render_context.image_views.push_back(vulkan::create_image_view(context.device, image, render_context.format));

    for(const auto& image_view : render_context.image_views)
      render_context.framebuffers.push_back(vulkan::create_framebuffer(context.device, render_context.render_pass, image_view, render_context.extent));

    return render_context;
  }


}
