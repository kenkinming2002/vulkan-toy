#include "render_context.hpp"

#include "buffer.hpp"
#include "command_buffer.hpp"
#include "render_pass.hpp"
#include "swapchain.hpp"
#include "attachment.hpp"
#include "framebuffer.hpp"
#include "pipeline_layout.hpp"
#include "vk_check.hpp"

#include <vulkan/vulkan_core.h>

namespace vulkan
{
  // We have one frame associated to each swapchain image
  struct ImageResource
  {
    SwapchainAttachment color_attachment;
    ManagedAttachment   depth_attachment;
    Framebuffer         framebuffer;
  };

  struct FrameResource
  {
    CommandBuffer command_buffer;
    Fence fence;

    VkSemaphore semaphore_image_available;
    VkSemaphore semaphore_render_finished;
  };

  struct RenderContext
  {
    RenderContextCreateInfo create_info;

    Swapchain      swapchain;
    PipelineLayout pipeline_layout;

    // TODO: How to support multiple render pass
    RenderPass render_pass;
    VkPipeline pipeline;

    ImageResource *image_resources;
    FrameResource *frame_resources;

    uint32_t frame_count;
    uint32_t frame_index;
  };

  static void init_render_context(const Context& context, allocator_t allocator, RenderContext& render_context)
  {
    init_swapchain(context, render_context.swapchain);

    {
      RenderPassCreateInfoSimple create_info = {};
      create_info.color_format = render_context.swapchain.surface_format.format;
      create_info.depth_format = VK_FORMAT_D32_SFLOAT;
      init_render_pass_simple(context, create_info, render_context.render_pass);
    }

    // Create pipeline layout
    {
      const DescriptorInfo descriptor_infos[] = {
        {.type = DescriptorType::UNIFORM_BUFFER, .stage = ShaderStage::VERTEX },
        {.type = DescriptorType::SAMPLER,        .stage = ShaderStage::FRAGMENT },
      };

      PipelineLayoutCreateInfo create_info = {};
      create_info.descriptors      = descriptor_infos;
      create_info.descriptor_count = std::size(descriptor_infos);
      create_info.push_constants      = nullptr;
      create_info.push_constant_count = 0;
      init_pipeline_layout(context, create_info, render_context.pipeline_layout);
    }


    // 4: Create Pipeline
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
        for(const auto& vertex_binding_description : render_context.create_info.vertex_binding_descriptions)
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
      vert_shader_stage_create_info.module = render_context.create_info.vert_shader_module;
      vert_shader_stage_create_info.pName = "main";

      VkPipelineShaderStageCreateInfo frag_shader_stage_create_info = {};
      frag_shader_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
      frag_shader_stage_create_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
      frag_shader_stage_create_info.module = render_context.create_info.frag_shader_module;
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
      rasterizer_state_create_info.frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE;
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
      VkPipelineDepthStencilStateCreateInfo depth_stencil_state_create_info = {};
      depth_stencil_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
      depth_stencil_state_create_info.depthTestEnable  = VK_TRUE;
      depth_stencil_state_create_info.depthWriteEnable = VK_TRUE;
      depth_stencil_state_create_info.depthCompareOp   = VK_COMPARE_OP_LESS;
      depth_stencil_state_create_info.depthBoundsTestEnable = VK_FALSE;
      depth_stencil_state_create_info.stencilTestEnable = VK_FALSE;

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

      VkGraphicsPipelineCreateInfo graphics_pipeline_create_info = {};
      graphics_pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
      graphics_pipeline_create_info.stageCount          = 2;
      graphics_pipeline_create_info.pStages             = shader_stage_create_infos;
      graphics_pipeline_create_info.pVertexInputState   = &vertex_input_state_create_info;
      graphics_pipeline_create_info.pInputAssemblyState = &input_assembly_state_create_info;
      graphics_pipeline_create_info.pViewportState      = &pipeline_viewport_state_create_info;
      graphics_pipeline_create_info.pRasterizationState = &rasterizer_state_create_info;
      graphics_pipeline_create_info.pMultisampleState   = &multisampling_state_create_info;
      graphics_pipeline_create_info.pDepthStencilState  = &depth_stencil_state_create_info;
      graphics_pipeline_create_info.pColorBlendState    = &color_blending_state_create_info;
      graphics_pipeline_create_info.pDynamicState       = &dynamic_state_create_info;
      graphics_pipeline_create_info.layout              = render_context.pipeline_layout.pipeline_layout;
      graphics_pipeline_create_info.renderPass          = render_context.render_pass.handle;
      graphics_pipeline_create_info.subpass             = 0;
      graphics_pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
      graphics_pipeline_create_info.basePipelineIndex  = -1;
      VK_CHECK(vkCreateGraphicsPipelines(context.device, VK_NULL_HANDLE, 1, &graphics_pipeline_create_info, nullptr, &render_context.pipeline));
    }

    // 5: Get image from swapchain
    VK_CHECK(vkGetSwapchainImagesKHR(context.device, render_context.swapchain.handle, &render_context.swapchain.image_count, nullptr));
    VkImage *swapchain_images = new VkImage[render_context.swapchain.image_count];
    VK_CHECK(vkGetSwapchainImagesKHR(context.device, render_context.swapchain.handle, &render_context.swapchain.image_count, swapchain_images));

    // 5: Create image resources
    {
      render_context.image_resources = new ImageResource[render_context.swapchain.image_count];
      for(uint32_t i=0; i<render_context.swapchain.image_count; ++i)
      {
        ImageResource image_resource = {};

        {
          SwapchainAttachmentCreateInfo create_info = {};
          create_info.swapchain = render_context.swapchain;
          create_info.index     = i;
          init_attachment_swapchain(context, create_info, image_resource.color_attachment);
        }

        {
          ManagedAttachmentCreateInfo create_info = {};
          create_info.type   = AttachmentType::DEPTH;
          create_info.extent = render_context.swapchain.extent;
          create_info.format = VK_FORMAT_D32_SFLOAT;
          init_attachment_managed(context, allocator, create_info, image_resource.depth_attachment);
        }

        {
          const Attachment attachments[] = {
            to_attachment(image_resource.color_attachment),
            to_attachment(image_resource.depth_attachment),
          };
          FramebufferCreateInfo create_info = {};
          create_info.render_pass      = render_context.render_pass.handle;
          create_info.extent           = render_context.swapchain.extent;
          create_info.attachments      = attachments;
          create_info.attachment_count = std::size(attachments);
          init_framebuffer(context, create_info, image_resource.framebuffer);
        }

        render_context.image_resources[i] = image_resource;
      }
    }

    // 7: Create frame resources
    {
      render_context.frame_resources = new FrameResource[render_context.create_info.max_frame_in_flight];
      for(uint32_t i=0; i<render_context.create_info.max_frame_in_flight; ++i)
      {
        FrameResource frame_resource = {};
        init_command_buffer(context, frame_resource.command_buffer);
        init_fence(context, frame_resource.fence, true);
        frame_resource.semaphore_image_available = vulkan::create_semaphore(context.device);
        frame_resource.semaphore_render_finished = vulkan::create_semaphore(context.device);
        render_context.frame_resources[i] = frame_resource;
      }
      render_context.frame_count = render_context.create_info.max_frame_in_flight;
      render_context.frame_index = 0;
    }
  }

  static void deinit_render_context(const Context& context, allocator_t allocator, RenderContext& render_context)
  {
    (void)allocator;

    for(uint32_t i=0; i<render_context.swapchain.image_count; ++i)
    {
      ImageResource& frame = render_context.image_resources[i];
      deinit_framebuffer(context, frame.framebuffer);
      deinit_attachment_swapchain(context, frame.color_attachment);
      deinit_attachment_managed(context, allocator, frame.depth_attachment);
    }
    delete[] render_context.image_resources;

    vkDestroyPipeline(context.device, render_context.pipeline, nullptr);
    deinit_pipeline_layout(context, render_context.pipeline_layout);
    deinit_render_pass(context, render_context.render_pass);
    deinit_swapchain(context, render_context.swapchain);
  }

  render_context_t create_render_context(const Context& context, allocator_t allocator, RenderContextCreateInfo create_info)
  {
    render_context_t render_context = new RenderContext{};
    render_context->create_info = create_info;
    init_render_context(context, allocator, *render_context);
    return render_context;
  }

  void destroy_render_context(const Context& context, allocator_t allocator, render_context_t render_context)
  {
    deinit_render_context(context, allocator, *render_context);
    delete render_context;
  }

  std::optional<RenderInfo> begin_render(const Context& context, render_context_t render_context)
  {
    // Render info
    RenderInfo info  = {};

    // Acquire frame resource
    info.frame_index = render_context->frame_index;
    render_context->frame_index = (render_context->frame_index + 1) % render_context->frame_count;
    FrameResource frame_resource = render_context->frame_resources[info.frame_index];

    // Acquire image resource
    auto result = vkAcquireNextImageKHR(context.device, render_context->swapchain.handle, UINT64_MAX, frame_resource.semaphore_image_available, VK_NULL_HANDLE, &info.image_index);
    if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) return std::nullopt;
    VK_CHECK(result);
    ImageResource image_resource = render_context->image_resources[info.image_index];

    // Wait and begin commannd buffer recording
    fence_wait_and_reset(context, frame_resource.fence);
    command_buffer_begin(frame_resource.command_buffer);

    VkRenderPassBeginInfo render_pass_begin_info = {};
    render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_begin_info.renderPass        = render_context->render_pass.handle;
    render_pass_begin_info.framebuffer       = image_resource.framebuffer.handle;
    render_pass_begin_info.renderArea.offset = {0, 0};
    render_pass_begin_info.renderArea.extent = render_context->swapchain.extent;

    // TODO: Take this as argument
    VkClearValue clear_values[2] = {};
    clear_values[0].color        = {{ 0.0f, 0.0f, 0.0f, 1.0f }};
    clear_values[1].depthStencil = { 1.0f, 0 };

    render_pass_begin_info.clearValueCount = 2;
    render_pass_begin_info.pClearValues = clear_values;

    vkCmdBeginRenderPass(frame_resource.command_buffer.handle, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(frame_resource.command_buffer.handle, VK_PIPELINE_BIND_POINT_GRAPHICS, vulkan::render_context_get_pipeline(render_context));

    info.semaphore_image_available = frame_resource.semaphore_image_available;
    info.semaphore_render_finished = frame_resource.semaphore_render_finished;
    info.command_buffer            = frame_resource.command_buffer;
    return info;
  }

  bool end_render(const Context& context, render_context_t render_context, RenderInfo info)
  {
    FrameResource frame_resource = render_context->frame_resources[info.frame_index];

    vkCmdEndRenderPass(frame_resource.command_buffer.handle);

    // End command buffer recording and submit
    command_buffer_end(frame_resource.command_buffer);
    command_buffer_submit(context, frame_resource.command_buffer, frame_resource.fence,
        frame_resource.semaphore_image_available, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        frame_resource.semaphore_render_finished);

    // Present
    VkPresentInfoKHR present_info = {};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = &frame_resource.semaphore_render_finished;
    present_info.swapchainCount = 1;
    present_info.pSwapchains    = &render_context->swapchain.handle;
    present_info.pImageIndices  = &info.image_index;
    present_info.pResults       = nullptr;

    auto result = vkQueuePresentKHR(context.queue, &present_info);
    if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
      return false;

    VK_CHECK(result);
    return true;
  }

  VkExtent2D render_context_get_extent(render_context_t render_context) { return render_context->swapchain.extent; }

  VkDescriptorSetLayout render_context_get_descriptor_set_layout(render_context_t render_context) { return render_context->pipeline_layout.descriptor_set_layout; }
  VkPipelineLayout render_context_get_pipeline_layout(render_context_t render_context) { return render_context->pipeline_layout.pipeline_layout; }

  VkRenderPass render_context_get_render_pass(render_context_t render_context) { return render_context->render_pass.handle; }
  VkPipeline render_context_get_pipeline(render_context_t render_context) { return render_context->pipeline; }
}
