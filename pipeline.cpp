#include "pipeline.hpp"

#include "vk_check.hpp"

namespace vulkan
{
  static VkFormat to_vulkan_format(VertexAttribute::Type type)
  {
    switch(type)
    {
    case VertexAttribute::Type::FLOAT1: return VK_FORMAT_R32_SFLOAT;
    case VertexAttribute::Type::FLOAT2: return VK_FORMAT_R32G32_SFLOAT;
    case VertexAttribute::Type::FLOAT3: return VK_FORMAT_R32G32B32_SFLOAT;
    case VertexAttribute::Type::FLOAT4: return VK_FORMAT_R32G32B32A32_SFLOAT;
    default:
      fprintf(stderr, "Unknown vertex attribute type\n");
      abort();
    }
  }

  void init_pipeline(const Context& context, PipelineCreateInfo create_info, Pipeline& pipeline)
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
      const VertexInput vertex_input = create_info.vertex_input;
      for(uint32_t binding = 0; binding < create_info.vertex_input.binding_count; ++binding)
      {
        const VertexBinding vertex_binding = vertex_input.bindings[binding];
        for(uint32_t location = 0; location < vertex_binding.attribute_count; ++location)
        {
          const VertexAttribute vertex_attribute = vertex_binding.attributes[location];

          VkVertexInputAttributeDescription vertex_input_attribute_description = {};
          vertex_input_attribute_description.binding  = binding;
          vertex_input_attribute_description.location = location;
          vertex_input_attribute_description.format   = to_vulkan_format(vertex_attribute.type);
          vertex_input_attribute_description.offset   = vertex_attribute.offset;
          vertex_input_attribute_descriptions.push_back(vertex_input_attribute_description);
        }

        VkVertexInputBindingDescription vertex_input_binding_description = {};
        vertex_input_binding_description.binding   = binding;
        vertex_input_binding_description.stride    = vertex_binding.stride;
        vertex_input_binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        vertex_input_binding_descriptions.push_back(vertex_input_binding_description);
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
    vert_shader_stage_create_info.module = create_info.vertex_shader.handle;
    vert_shader_stage_create_info.pName = "main";

    VkPipelineShaderStageCreateInfo frag_shader_stage_create_info = {};
    frag_shader_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    frag_shader_stage_create_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    frag_shader_stage_create_info.module = create_info.fragment_shader.handle;
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
    graphics_pipeline_create_info.layout              = create_info.pipeline_layout.pipeline_layout;
    graphics_pipeline_create_info.renderPass          = create_info.render_pass.handle;
    graphics_pipeline_create_info.subpass             = 0;
    graphics_pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
    graphics_pipeline_create_info.basePipelineIndex  = -1;
    VK_CHECK(vkCreateGraphicsPipelines(context.device, VK_NULL_HANDLE, 1, &graphics_pipeline_create_info, nullptr, &pipeline.handle));
  }

  void deinit_pipeline(const Context& context, Pipeline& pipeline)
  {
    vkDestroyPipeline(context.device, pipeline.handle, nullptr);
  }
}
