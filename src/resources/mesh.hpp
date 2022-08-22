#pragma once

#include "buffer.hpp"
#include "core/command_buffer.hpp"
#include "ref.hpp"
#include "render/input.hpp"

#include <glm/glm.hpp>

#include <array>

namespace vulkan
{
  // Description
  struct VertexAttributeDescription
  {
    enum class Type
    {
      FLOAT1, FLOAT2, FLOAT3, FLOAT4 // Which maniac who not use float as vertex input anyway?
    };

    size_t offset;
    Type type;
  };

  struct VertexBindingDescription
  {
    size_t stride;

    const VertexAttributeDescription *attributes;
    size_t                            attribute_count;
  };

  struct VertexLayoutDescription
  {
    const VertexBindingDescription *bindings;
    size_t                          binding_count;
  };

  struct MeshLayoutDescription
  {
    VertexLayoutDescription vertex_layout;
  };

  // Layout
  typedef struct MeshLayout *mesh_layout_t;

  // TODO: We do not need to take a context_t but it may be a good idea to do
  //       so for consistency with material layout.
  mesh_layout_t mesh_layout_compile(const MeshLayoutDescription *mesh_layout_description);
  ref_t mesh_layout_as_ref(mesh_layout_t mesh_layout);

  inline void mesh_layout_get(mesh_layout_t mesh_layout) { ref_get(mesh_layout_as_ref(mesh_layout)); }
  inline void mesh_layout_put(mesh_layout_t mesh_layout) { ref_put(mesh_layout_as_ref(mesh_layout));  }

  const VkPipelineVertexInputStateCreateInfo *mesh_layout_get_vulkan_pipeline_vertex_input_state_create_info(mesh_layout_t mesh_layout);

  mesh_layout_t mesh_layout_create_default();

  struct Vertex
  {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec3 color;
    glm::vec2 uv;
  };

  // Mesh
  // TODO: Make mesh independent of the vertex type used
  typedef struct Mesh *mesh_t;

  mesh_t mesh_create(context_t context, allocator_t allocator, mesh_layout_t mesh_layout, size_t vertex_count, size_t index_count);
  ref_t mesh_as_ref(mesh_t mesh);

  inline void mesh_get(mesh_t mesh) { ref_get(mesh_as_ref(mesh)); }
  inline void mesh_put(mesh_t mesh) { ref_put(mesh_as_ref(mesh));  }

  void mesh_write(command_buffer_t command_buffer, mesh_t mesh, const void **vertices, const uint32_t *indices);
  mesh_t mesh_load(command_buffer_t command_buffer, context_t context, allocator_t allocator, const char *file_name);

  void mesh_render_simple(command_buffer_t command_buffer, mesh_t mesh);
}
