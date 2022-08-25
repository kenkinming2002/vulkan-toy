#include "pipeline.hpp"

#include "utils.hpp"
#include "vk_check.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <vector>

namespace vulkan
{
  // Descriptor set layout
  static VkDescriptorType to_vulkan_descriptor_type(DescriptorType type)
  {
    switch(type)
    {
    case DescriptorType::UNIFORM_BUFFER: return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    case DescriptorType::SAMPLER:        return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    default: assert(false && "Unreachable");
    }
  }

  static VkShaderStageFlags to_vulkan_stage_flags(ShaderStage stage)
  {
    switch(stage)
    {
    case ShaderStage::VERTEX:   return VK_SHADER_STAGE_VERTEX_BIT;
    case ShaderStage::FRAGMENT: return VK_SHADER_STAGE_FRAGMENT_BIT;
    default: assert(false && "Unreachable");
    }
  }

  static inline void init_pipeline2_pipeline_layout(context_t context, PipelineCreateInfo2 create_info, Pipeline2& pipeline)
  {
    VkDevice device = context_get_device_handle(context);

    dynarray<VkPushConstantRange> push_constant_ranges = create_dynarray<VkPushConstantRange>(create_info.push_constant_input.range_count);
    for(uint32_t i=0; i<create_info.push_constant_input.range_count; ++i)
    {
      VkPushConstantRange push_constant_range = {};
      push_constant_range.stageFlags = to_vulkan_stage_flags(create_info.push_constant_input.ranges[i].stage);
      push_constant_range.offset     = create_info.push_constant_input.ranges[i].offset;
      push_constant_range.size       = create_info.push_constant_input.ranges[i].size;
      push_constant_ranges[i] = push_constant_range;
      assert(push_constant_ranges[i].stageFlags == VK_SHADER_STAGE_VERTEX_BIT);
    }

    VkDescriptorSetLayout descriptor_set_layout = material_layout_get_descriptor_set_layout(create_info.material_layout);

    VkPipelineLayoutCreateInfo pipeline_layout_create_info = {};
    pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_create_info.setLayoutCount         = 1;
    pipeline_layout_create_info.pSetLayouts            = &descriptor_set_layout;
    pipeline_layout_create_info.pushConstantRangeCount = size(push_constant_ranges);
    pipeline_layout_create_info.pPushConstantRanges    = data(push_constant_ranges);
    VK_CHECK(vkCreatePipelineLayout(device, &pipeline_layout_create_info, nullptr, &pipeline.pipeline_layout));

    destroy_dynarray(push_constant_ranges);
  }

  void init_pipeline2(context_t context, PipelineCreateInfo2 create_info, Pipeline2& pipeline)
  {
    VkDevice device = context_get_device_handle(context);

    init_pipeline2_pipeline_layout(context, create_info, pipeline);

    const VkPipelineVertexInputStateCreateInfo *vertex_input_state_create_info = mesh_layout_get_vulkan_pipeline_vertex_input_state_create_info(create_info.mesh_layout);

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
    graphics_pipeline_create_info.pVertexInputState   = vertex_input_state_create_info;
    graphics_pipeline_create_info.pInputAssemblyState = &input_assembly_state_create_info;
    graphics_pipeline_create_info.pViewportState      = &pipeline_viewport_state_create_info;
    graphics_pipeline_create_info.pRasterizationState = &rasterizer_state_create_info;
    graphics_pipeline_create_info.pMultisampleState   = &multisampling_state_create_info;
    graphics_pipeline_create_info.pDepthStencilState  = &depth_stencil_state_create_info;
    graphics_pipeline_create_info.pColorBlendState    = &color_blending_state_create_info;
    graphics_pipeline_create_info.pDynamicState       = &dynamic_state_create_info;
    graphics_pipeline_create_info.layout              = pipeline.pipeline_layout;
    graphics_pipeline_create_info.renderPass          = create_info.render_pass.handle;
    graphics_pipeline_create_info.subpass             = 0;
    graphics_pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
    graphics_pipeline_create_info.basePipelineIndex  = -1;
    VK_CHECK(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &graphics_pipeline_create_info, nullptr, &pipeline.handle));
  }

  void deinit_pipeline2(context_t context, Pipeline2& pipeline)
  {
    VkDevice device = context_get_device_handle(context);

    vkDestroyPipeline(device, pipeline.handle, nullptr);
    vkDestroyPipelineLayout(device, pipeline.pipeline_layout, nullptr);
  }
}
