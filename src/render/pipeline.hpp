#pragma once

#include "core/context.hpp"
#include "input.hpp"
#include "render_pass.hpp"
#include "shader.hpp"

namespace vulkan
{
  struct Pipeline2
  {
    VkDescriptorSetLayout descriptor_set_layout; // TODO: Allow more then 1 descriptor set layout
    VkPipelineLayout      pipeline_layout;
    VkPipeline            handle;
  };

  struct PipelineCreateInfo2
  {
    RenderPass render_pass;

    Shader vertex_shader;
    Shader fragment_shader;

    VertexInput       vertex_input;
    DescriptorInput   descriptor_input;
    PushConstantInput push_constant_input;
  };

  // TODO: Use pipeline cache
  void init_pipeline2(context_t context, PipelineCreateInfo2 create_info, Pipeline2& pipeline);
  void deinit_pipeline2(context_t context, Pipeline2& pipeline);
}
