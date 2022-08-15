#pragma once

#include "context.hpp"
#include "input.hpp"
#include "pipeline_layout.hpp"
#include "render_pass.hpp"
#include "shader.hpp"

namespace vulkan
{
  struct PipelineCreateInfo
  {
    RenderPass     render_pass;

    Shader vertex_shader;
    Shader fragment_shader;

    VertexInput    vertex_input;
    PipelineLayout pipeline_layout;
  };

  struct Pipeline
  {
    VkPipeline handle;
  };

  void init_pipeline(const Context& context, PipelineCreateInfo create_info, Pipeline& pipeline);
  void deinit_pipeline(const Context& context, Pipeline& pipeline);

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
  void init_pipeline2(const Context& context, PipelineCreateInfo2 create_info, Pipeline2& pipeline);
  void deinit_pipeline2(const Context& context, Pipeline2& pipeline);
}
