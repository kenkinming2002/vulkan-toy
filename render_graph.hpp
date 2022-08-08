#pragma once

#include <stddef.h>

namespace rg
{
  using binding_index_t = unsigned short;

  // Basic resources binding, i.e. things that could be reused in a a render graph
  enum class AttachmentType { SWAPCHAIN, DEPTH };

  struct AttachmentDescription
  {
    AttachmentType type;
  };

  struct ShaderDescription
  {
    const char *file_name;
  };

  struct ResourcesDescription
  {
    const AttachmentDescription *attachment_descriptions;
    unsigned short               attachment_description_count;

    const ShaderDescription *shader_descriptions;
    unsigned short           shader_description_count;
  };

  // TODO: Subpass
  struct RenderPassDescription
  {
    const binding_index_t *attachment_binding_indices;
    unsigned short         attachment_binding_count;
  };

  struct DescriptorBinding
  {
    enum class Type { TEXTURE, UNIFORM_BUFFER } type;
    enum class Stage { VERTEX, FRAGMENT } stage;
    binding_index_t binding_index;
  };

  struct DescriptorInput
  {
    const DescriptorBinding *bindings;
    unsigned short           binding_count;
  };

  enum class DataType { FLOAT1, FLOAT2, FLOAT3, FLOAT4 };

  struct VertexAttribute
  {
    size_t   offset;
    DataType data_type;
  };

  struct VertexBinding
  {
    size_t stride;

    const VertexAttribute *attributes;
    unsigned short         attribute_count;
  };

  struct VertexInput
  {
    const VertexBinding *bindings;
    unsigned short       binding_count;
  };

  struct PipelineDescription
  {
    // Shaders
    binding_index_t vert_shader_binding_index;
    binding_index_t frag_shader_binding_index;

    // Inputs
    DescriptorInput descriptor_input;
    VertexInput     vertex_input;

    // Vertices
    // TODO: Support multiple vertex buffer
    binding_index_t vertex_buffer_binding_index;
    binding_index_t index_buffer_binding_index;
  };

  struct RenderGraph
  {
    ResourcesDescription  resources_description;
    RenderPassDescription render_pass_description;
    PipelineDescription   pipeline_description;
  };

  void debug_print(const RenderGraph& render_graph);
}
