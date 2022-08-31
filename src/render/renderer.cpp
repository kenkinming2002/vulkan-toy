#include "renderer.hpp"

#include "vk_check.hpp"

namespace vulkan
{
  struct Renderer
  {
    context_t context;

    VkPipelineLayout pipeline_layout;
    VkPipeline       pipeline;

    const Frame *current_frame;
  };

  renderer_t renderer_create(
    context_t context,
    render_target_t render_target,
    mesh_layout_t mesh_layout,
    material_layout_t material_layout,
    const char *vertex_shader_file_name,
    const char *fragment_shader_file_name)
  {
    renderer_t renderer = new Renderer {};

    get(context);
    renderer->context = context;

    // TODO: Load this outside
    shader_t vertex_shader   = shader_load(context, vertex_shader_file_name);
    shader_t fragment_shader = shader_load(context, fragment_shader_file_name);

    VkDevice device = context_get_device_handle(renderer->context);

    // Pipeline Layout
    VkPushConstantRange push_constant_range = {};
    push_constant_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    push_constant_range.offset     = 0;
    push_constant_range.size       = sizeof(CameraMatrices);

    VkDescriptorSetLayout descriptor_set_layout = material_layout_get_descriptor_set_layout(material_layout);

    VkPipelineLayoutCreateInfo pipeline_layout_create_info = {};
    pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_create_info.setLayoutCount         = 1;
    pipeline_layout_create_info.pSetLayouts            = &descriptor_set_layout;
    pipeline_layout_create_info.pushConstantRangeCount = 1;
    pipeline_layout_create_info.pPushConstantRanges    = &push_constant_range;
    VK_CHECK(vkCreatePipelineLayout(device, &pipeline_layout_create_info, nullptr, &renderer->pipeline_layout));

    // Pipeline
    const VkPipelineVertexInputStateCreateInfo *vertex_input_state_create_info = mesh_layout_get_vulkan_pipeline_vertex_input_state_create_info(mesh_layout);

    VkPipelineInputAssemblyStateCreateInfo input_assembly_state_create_info = {};
    input_assembly_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly_state_create_info.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly_state_create_info.primitiveRestartEnable = VK_FALSE;

    VkPipelineShaderStageCreateInfo vert_shader_stage_create_info = {};
    vert_shader_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vert_shader_stage_create_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vert_shader_stage_create_info.module = shader_get_handle(vertex_shader);
    vert_shader_stage_create_info.pName = "main";

    VkPipelineShaderStageCreateInfo frag_shader_stage_create_info = {};
    frag_shader_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    frag_shader_stage_create_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    frag_shader_stage_create_info.module = shader_get_handle(fragment_shader);
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

    VkDynamicState dynamic_states[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

    VkPipelineDynamicStateCreateInfo dynamic_state_create_info = {};
    dynamic_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state_create_info.dynamicStateCount = std::size(dynamic_states);
    dynamic_state_create_info.pDynamicStates    = dynamic_states;

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
    graphics_pipeline_create_info.layout              = renderer->pipeline_layout;
    graphics_pipeline_create_info.renderPass          = render_target_get_render_pass(render_target);
    graphics_pipeline_create_info.subpass             = 0;
    graphics_pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
    graphics_pipeline_create_info.basePipelineIndex  = -1;
    VK_CHECK(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &graphics_pipeline_create_info, nullptr, &renderer->pipeline));

    put(vertex_shader);
    put(fragment_shader);

    return renderer;
  }


  void renderer_destroy(renderer_t renderer)
  {
    VkDevice device = context_get_device_handle(renderer->context);

    vkDestroyPipelineLayout(device, renderer->pipeline_layout, nullptr);
    vkDestroyPipeline(device, renderer->pipeline, nullptr);

    put(renderer->context);
    delete renderer;
  }

  void renderer_begin_render(renderer_t renderer, const Frame *frame)
  {
    renderer->current_frame = frame;

    VkCommandBuffer handle = command_buffer_get_handle(renderer->current_frame->command_buffer);
    vkCmdBindPipeline(handle, VK_PIPELINE_BIND_POINT_GRAPHICS, renderer->pipeline);
  }

  void renderer_end_render(renderer_t renderer)
  {
    renderer->current_frame = nullptr;
  }

  void renderer_set_viewport_and_scissor(renderer_t renderer, VkExtent2D extent)
  {
    VkCommandBuffer handle = command_buffer_get_handle(renderer->current_frame->command_buffer);

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width  = extent.width;
    viewport.height = extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(handle, 0, 1, &viewport);

    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = extent;
    vkCmdSetScissor(handle, 0, 1, &scissor);
  }

  void renderer_use_camera(renderer_t renderer, const Camera& camera)
  {
    VkCommandBuffer handle = command_buffer_get_handle(renderer->current_frame->command_buffer);

    vulkan::CameraMatrices camera_matrices = vulkan::camera_compute_matrices(camera);
    vkCmdPushConstants(handle, renderer->pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof camera_matrices, &camera_matrices);
  }

  void renderer_draw(renderer_t renderer, material_t material, mesh_t mesh)
  {
    VkCommandBuffer handle = command_buffer_get_handle(renderer->current_frame->command_buffer);

    VkDescriptorSet descriptor_set = material_get_descriptor_set(material);
    vkCmdBindDescriptorSets(handle, VK_PIPELINE_BIND_POINT_GRAPHICS, renderer->pipeline_layout, 0, 1, &descriptor_set, 0, nullptr);

    size_t vertex_buffer_count;
    buffer_t *vertex_buffers = mesh_get_vertex_buffers(mesh, vertex_buffer_count);
    for(size_t i=0; i<vertex_buffer_count; ++i)
    {
      VkDeviceSize offsets[] = {0};

      buffer_t vertex_buffer        = vertex_buffers[i];
      VkBuffer vertex_buffer_handle = buffer_get_handle(vertex_buffer);
      vkCmdBindVertexBuffers(handle, i, 1, &vertex_buffer_handle, offsets);
      command_buffer_use(renderer->current_frame->command_buffer, as_ref(vertex_buffer));
    }

    buffer_t index_buffer = mesh_get_index_buffer(mesh);
    VkBuffer index_buffer_handle = buffer_get_handle(index_buffer);
    vkCmdBindIndexBuffer(handle, index_buffer_handle, 0, VK_INDEX_TYPE_UINT32);
    command_buffer_use(renderer->current_frame->command_buffer, as_ref(index_buffer));

    // Index buffers
    size_t index_count = mesh_get_index_count(mesh);
    vkCmdDrawIndexed(handle, index_count, 1, 0, 0, 0);
  }
}
