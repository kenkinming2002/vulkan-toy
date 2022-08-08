#include "render_graph.hpp"

#include <stdio.h>

namespace rg
{
  static const char* to_string(AttachmentType type)
  {
    switch(type)
    {
    case AttachmentType::SWAPCHAIN: return "swapchain";
    case AttachmentType::DEPTH:     return "depth";
    default: return "unknown";
    }
  }

  static void debug_print(unsigned short i, const AttachmentDescription& description)
  {
    printf("attachment %hu: type = %s\n", i, to_string(description.type));
  }

  static void debug_print(unsigned short i, const ShaderDescription& description)
  {
    printf("shader %hu: file name = %s\n", i, description.file_name);
  }

  static void debug_print(const ResourcesDescription& description)
  {
    for(unsigned short i=0; i<description.attachment_description_count; ++i)
      debug_print(i, description.attachment_descriptions[i]);

    for(unsigned short i=0; i<description.shader_description_count; ++i)
      debug_print(i, description.shader_descriptions[i]);
  }

  static void debug_print(const RenderPassDescription& description)
  {
    for(unsigned short i=0; i<description.attachment_binding_count; ++i)
      printf("render pass: attachment %hu binding index = %hu\n", i, description.attachment_binding_indices[i]);
  }

  static const char* to_string(DescriptorBinding::Type type)
  {
    switch(type)
    {
    case DescriptorBinding::Type::TEXTURE:        return "texture";
    case DescriptorBinding::Type::UNIFORM_BUFFER: return "uniform buffer";
    default: return "unknown";
    }
  }

  static const char* to_string(DescriptorBinding::Stage stage)
  {
    switch(stage)
    {
    case DescriptorBinding::Stage::VERTEX:   return "vertex";
    case DescriptorBinding::Stage::FRAGMENT: return "fragment";
    default: return "unknown";
    }
  }

  static void debug_print(const DescriptorBinding& descriptor_binding)
  {
    printf("  binding: type = %s, stage = %s\n", to_string(descriptor_binding.type), to_string(descriptor_binding.stage));
  }

  static void debug_print(const DescriptorInput& descriptor_input)
  {
    printf("descriptor input:\n");
    for(unsigned short i=0; i<descriptor_input.binding_count; ++i)
      debug_print(descriptor_input.bindings[i]);
  }

  static const char* to_string(DataType data_type)
  {
    switch(data_type)
    {
    case DataType::FLOAT1: return "float1";
    case DataType::FLOAT2: return "float2";
    case DataType::FLOAT3: return "float3";
    case DataType::FLOAT4: return "float4";
    default: return "unknown";
    }
  }

  static void debug_print(const VertexAttribute& vertex_attribute)
  {
    printf("  attribute: offset = %lu, data type = %s\n", vertex_attribute.offset, to_string(vertex_attribute.data_type));
  }

  static void debug_print(const VertexBinding& vertex_binding)
  {
    printf("  binding: stride = %lu\n", vertex_binding.stride);
    for(unsigned short i=0; i<vertex_binding.attribute_count; ++i)
      debug_print(vertex_binding.attributes[i]);
  }

  static void debug_print(const VertexInput& vertex_input)
  {
    printf("vertex input:\n");
    for(unsigned short i=0; i<vertex_input.binding_count; ++i)
      debug_print(vertex_input.bindings[i]);
  }

  static void debug_print(const PipelineDescription& description)
  {
    printf("pipeline: vertex shader binding index   = %hu\n", description.vert_shader_binding_index);
    printf("pipeline: fragment shader binding index = %hu\n", description.frag_shader_binding_index);

    debug_print(description.descriptor_input);
    debug_print(description.vertex_input);

    printf("pipeline: vertex buffer binding index = %hu\n", description.vertex_buffer_binding_index);
    printf("pipeline: index buffer binding index  = %hu\n", description.index_buffer_binding_index);
  }

  void debug_print(const RenderGraph& render_graph)
  {
    debug_print(render_graph.resources_description);
    debug_print(render_graph.render_pass_description);
    debug_print(render_graph.pipeline_description);
  }
}
