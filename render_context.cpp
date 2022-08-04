#include "render_context.hpp"

namespace vulkan
{
  RenderContext create_render_context(const Context& context, Allocator& allocator, RenderContextCreateInfo create_info)
  {
    RenderContext render_context = {};

    // 1: Get information about the surface and select the best
    {
      // Capabilities
      {
        VkSurfaceCapabilitiesKHR capabilities = {};
        VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(context.physical_device, context.surface, &capabilities));

        // Image count
        render_context.image_count = capabilities.minImageCount + 1;
        if(capabilities.maxImageCount != 0)
          render_context.image_count = std::min(render_context.image_count, capabilities.maxImageCount);

        // Extent
        render_context.extent = capabilities.currentExtent;
        if(render_context.extent.height == std::numeric_limits<uint32_t>::max() ||
           render_context.extent.width  == std::numeric_limits<uint32_t>::max())
        {
          // This should never happen in practice since this means we did not
          // speicify the window size when we create the window. Just use some
          // hard-coded value in this case
          fprintf(stderr, "Window size not specified. Using a default value of 1080x720\n");
          render_context.extent = { 1080, 720 };
        }
        render_context.extent.width  = std::clamp(render_context.extent.width , capabilities.minImageExtent.width , capabilities.maxImageExtent.width);
        render_context.extent.height = std::clamp(render_context.extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        // Surface transform
        render_context.surface_transform = capabilities.currentTransform;
      }


      // Surface format
      {
        uint32_t count;
        VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(context.physical_device, context.surface, &count, nullptr));
        auto *surface_formats = new VkSurfaceFormatKHR[count];
        VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(context.physical_device, context.surface, &count, surface_formats));
        for(uint32_t i=0; i<count; ++i)
          if(surface_formats[i].format == VK_FORMAT_B8G8R8A8_SRGB && surface_formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
          {
            render_context.surface_format = surface_formats[i];
            goto surface_format_selected;
          }

        render_context.surface_format = surface_formats[0];

surface_format_selected:
        delete[] surface_formats;
        surface_formats = nullptr;
      }

      // Present mode
      {
        uint32_t count;
        VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(context.physical_device, context.surface, &count, nullptr));
        auto *present_modes = new VkPresentModeKHR[count];
        VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(context.physical_device, context.surface, &count, present_modes));
        for(uint32_t i=0; i<count; ++i)
          if(present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
          {
            render_context.present_mode = present_modes[i];
            goto present_mode_selected;
          }

        render_context.present_mode = VK_PRESENT_MODE_FIFO_KHR;

present_mode_selected:
        delete[] present_modes;
        present_modes = nullptr;
      }
    }

    // 2: Create Swapchain creation
    {
      VkSwapchainCreateInfoKHR create_info = {};
      create_info.sType                 = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
      create_info.surface               = context.surface;
      create_info.imageExtent           = render_context.extent;
      create_info.minImageCount         = render_context.image_count;
      create_info.imageFormat           = render_context.surface_format.format;
      create_info.imageColorSpace       = render_context.surface_format.colorSpace;
      create_info.imageArrayLayers      = 1;
      create_info.imageUsage            = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
      create_info.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
      create_info.queueFamilyIndexCount = 0;
      create_info.pQueueFamilyIndices   = nullptr;
      create_info.preTransform          = render_context.surface_transform;
      create_info.compositeAlpha        = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
      create_info.presentMode           = render_context.present_mode;
      create_info.clipped               = VK_TRUE;
      create_info.oldSwapchain          = VK_NULL_HANDLE;
      VK_CHECK(vkCreateSwapchainKHR(context.device, &create_info, nullptr, &render_context.swapchain));
    }

    // 3: Create Render pass
    {
      // Create render pass
      VkAttachmentDescription color_attachment = {};
      color_attachment.format         = render_context.surface_format.format;
      color_attachment.samples        = VK_SAMPLE_COUNT_1_BIT;
      color_attachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
      color_attachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
      color_attachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
      color_attachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
      color_attachment.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

      VkAttachmentDescription depth_attachment = {};
      depth_attachment.format         = VK_FORMAT_D32_SFLOAT;
      depth_attachment.samples        = VK_SAMPLE_COUNT_1_BIT;
      depth_attachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
      depth_attachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
      depth_attachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
      depth_attachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
      depth_attachment.finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

      VkAttachmentDescription attachments[] = { color_attachment, depth_attachment };

      VkAttachmentReference color_attachment_reference = {};
      color_attachment_reference.attachment = 0;
      color_attachment_reference.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

      VkAttachmentReference depth_attachment_reference = {};
      depth_attachment_reference.attachment = 1;
      depth_attachment_reference.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

      VkSubpassDescription subpass_description = {};
      subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
      subpass_description.colorAttachmentCount    = 1;
      subpass_description.pColorAttachments       = &color_attachment_reference;
      subpass_description.pDepthStencilAttachment = &depth_attachment_reference;

      VkRenderPassCreateInfo render_pass_create_info = {};
      render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
      render_pass_create_info.attachmentCount = sizeof attachments / sizeof attachments[0];
      render_pass_create_info.pAttachments    = attachments;
      render_pass_create_info.subpassCount    = 1;
      render_pass_create_info.pSubpasses      = &subpass_description;

      VkSubpassDependency subpass_dependency = {};
      subpass_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
      subpass_dependency.dstSubpass = 0;
      subpass_dependency.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
      subpass_dependency.srcAccessMask = 0;
      subpass_dependency.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
      subpass_dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

      render_pass_create_info.dependencyCount = 1;
      render_pass_create_info.pDependencies   = &subpass_dependency;

      VK_CHECK(vkCreateRenderPass(context.device, &render_pass_create_info, nullptr, &render_context.render_pass));
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

      // We no longer have empty pipeline layout
      VkDescriptorSetLayoutBinding ubo_layout_binding = {};
      ubo_layout_binding.binding = 0;
      ubo_layout_binding.descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      ubo_layout_binding.descriptorCount    = 1;
      ubo_layout_binding.stageFlags         = VK_SHADER_STAGE_VERTEX_BIT;
      ubo_layout_binding.pImmutableSamplers = nullptr;

      VkDescriptorSetLayoutBinding sampler_layout_binding = {};
      sampler_layout_binding.binding = 1;
      sampler_layout_binding.descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      sampler_layout_binding.descriptorCount    = 1;
      sampler_layout_binding.stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT;
      sampler_layout_binding.pImmutableSamplers = nullptr;

      VkDescriptorSetLayoutBinding layout_bindings[] = { ubo_layout_binding, sampler_layout_binding };

      VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info = {};
      descriptor_set_layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
      descriptor_set_layout_create_info.bindingCount = 2;
      descriptor_set_layout_create_info.pBindings    = layout_bindings;
      VK_CHECK(vkCreateDescriptorSetLayout(context.device, &descriptor_set_layout_create_info, nullptr, &render_context.descriptor_set_layout));

      VkPipelineLayoutCreateInfo pipeline_layout_create_info = {};
      pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
      pipeline_layout_create_info.setLayoutCount = 1;
      pipeline_layout_create_info.pSetLayouts    = &render_context.descriptor_set_layout;
      VK_CHECK(vkCreatePipelineLayout(context.device, &pipeline_layout_create_info, nullptr, &render_context.pipeline_layout));

      VkGraphicsPipelineCreateInfo graphics_pipeline_create_info = {};
      graphics_pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
      graphics_pipeline_create_info.stageCount = 2;
      graphics_pipeline_create_info.pStages    = shader_stage_create_infos;
      graphics_pipeline_create_info.pVertexInputState   = &vertex_input_state_create_info;
      graphics_pipeline_create_info.pInputAssemblyState = &input_assembly_state_create_info;
      graphics_pipeline_create_info.pViewportState      = &pipeline_viewport_state_create_info;
      graphics_pipeline_create_info.pRasterizationState = &rasterizer_state_create_info;
      graphics_pipeline_create_info.pMultisampleState   = &multisampling_state_create_info;
      graphics_pipeline_create_info.pDepthStencilState  = &depth_stencil_state_create_info;
      graphics_pipeline_create_info.pColorBlendState    = &color_blending_state_create_info;
      graphics_pipeline_create_info.pDynamicState       = &dynamic_state_create_info;
      graphics_pipeline_create_info.layout              = render_context.pipeline_layout;
      graphics_pipeline_create_info.renderPass = render_context.render_pass;
      graphics_pipeline_create_info.subpass    = 0;
      graphics_pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
      graphics_pipeline_create_info.basePipelineIndex  = -1;
      VK_CHECK(vkCreateGraphicsPipelines(context.device, VK_NULL_HANDLE, 1, &graphics_pipeline_create_info, nullptr, &render_context.pipeline));
    }

    // 5: Create images
    {
      printf("Number of image in swapchain requested = %d\n", render_context.image_count);
      VK_CHECK(vkGetSwapchainImagesKHR(context.device, render_context.swapchain, &render_context.image_count, nullptr));
      printf("Number of image in swapchain got       = %d\n", render_context.image_count);

      render_context.images = new VkImage[render_context.image_count];
      VK_CHECK(vkGetSwapchainImagesKHR(context.device, render_context.swapchain, &render_context.image_count, render_context.images));
    }

    // 6: Create image views
    {
      render_context.image_views = new VkImageView[render_context.image_count];
      for(uint32_t i=0; i<render_context.image_count; ++i)
      {
        VkImageViewCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        create_info.image                           = render_context.images[i];
        create_info.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
        create_info.format                          = render_context.surface_format.format;
        create_info.components.r                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.g                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.b                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.a                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        create_info.subresourceRange.baseMipLevel   = 0;
        create_info.subresourceRange.levelCount     = 1;
        create_info.subresourceRange.baseArrayLayer = 0;
        create_info.subresourceRange.layerCount     = 1;
        VK_CHECK(vkCreateImageView(context.device, &create_info, nullptr, &render_context.image_views[i]));
      }
    }

    // 7: Depth images
    {
      render_context.depth_image_allocations = new ImageAllocation[render_context.image_count];
      for(uint32_t i=0; i<render_context.image_count; ++i)
      {
        render_context.depth_image_allocations[i] = allocate_image2d(context, allocator, VK_FORMAT_D32_SFLOAT,
            render_context.extent.width, render_context.extent.height,
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
      }
    }

    // 8: Depth image views
    {
      render_context.depth_image_views = new VkImageView[render_context.image_count];
      for(uint32_t i=0; i<render_context.image_count; ++i)
      {
        VkImageViewCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        create_info.image                           = render_context.depth_image_allocations[i].image;
        create_info.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
        create_info.format                          = VK_FORMAT_D32_SFLOAT;
        create_info.components.r                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.g                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.b                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.a                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_DEPTH_BIT;
        create_info.subresourceRange.baseMipLevel   = 0;
        create_info.subresourceRange.levelCount     = 1;
        create_info.subresourceRange.baseArrayLayer = 0;
        create_info.subresourceRange.layerCount     = 1;
        VK_CHECK(vkCreateImageView(context.device, &create_info, nullptr, &render_context.depth_image_views[i]));
      }
    }

    // 7: Framebuffers
    {
      render_context.framebuffers = new VkFramebuffer[render_context.image_count];
      for(uint32_t i=0; i<render_context.image_count; ++i)
      {
        VkImageView attachments[] = { render_context.image_views[i], render_context.depth_image_views[i] };

        VkFramebufferCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        create_info.renderPass      = render_context.render_pass;
        create_info.attachmentCount = 2;
        create_info.pAttachments    = attachments;
        create_info.width           = render_context.extent.width;
        create_info.height          = render_context.extent.height;
        create_info.layers          = 1;
        VK_CHECK(vkCreateFramebuffer(context.device, &create_info, nullptr, &render_context.framebuffers[i]));
      }
    }

    return render_context;
  }

  void destroy_render_context(const Context& context, Allocator& allocator, RenderContext& render_context)
  {
    for(uint32_t i=0; i<render_context.image_count; ++i)
      vkDestroyFramebuffer(context.device, render_context.framebuffers[i], nullptr);

    delete[] render_context.framebuffers;

    for(uint32_t i=0; i<render_context.image_count; ++i)
      vkDestroyImageView(context.device, render_context.depth_image_views[i], nullptr);

    delete[] render_context.depth_image_views;

    for(uint32_t i=0; i<render_context.image_count; ++i)
      deallocate_image2d(context, allocator, render_context.depth_image_allocations[i]);

    delete[] render_context.depth_image_allocations;

    for(uint32_t i=0; i<render_context.image_count; ++i)
      vkDestroyImageView(context.device, render_context.image_views[i], nullptr);

    delete[] render_context.image_views;

    delete[] render_context.images;


    vkDestroyPipeline           (context.device, render_context.pipeline,              nullptr);
    vkDestroyPipelineLayout     (context.device, render_context.pipeline_layout,       nullptr);
    vkDestroyDescriptorSetLayout(context.device, render_context.descriptor_set_layout, nullptr);
    vkDestroyRenderPass         (context.device, render_context.render_pass,           nullptr);
    vkDestroySwapchainKHR       (context.device, render_context.swapchain,             nullptr);
  }
}
