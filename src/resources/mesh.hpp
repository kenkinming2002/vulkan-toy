#pragma once

#include "input.hpp"
#include "resources/buffer.hpp"
#include "command_buffer.hpp"

#include <glm/glm.hpp>

#include <array>

namespace vulkan
{
  struct Vertex
  {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec3 color;
    glm::vec2 uv;
  };

  typedef struct Mesh *mesh_t;

  mesh_t mesh_create(const Context *context, Allocator *allocator, size_t vertex_count, size_t index_count);
  ref_t mesh_as_ref(mesh_t mesh);

  inline void mesh_get(mesh_t mesh) { ref_get(mesh_as_ref(mesh)); }
  inline void mesh_put(mesh_t mesh) { ref_put(mesh_as_ref(mesh));  }

  void mesh_write(command_buffer_t command_buffer, mesh_t mesh, const Vertex *vertices, const uint32_t *indices);
  mesh_t mesh_load(command_buffer_t command_buffer, const Context *context, Allocator *allocator, const char *file_name);

  void mesh_render_simple(command_buffer_t command_buffer, mesh_t mesh);


  static constexpr vulkan::VertexAttribute VERTEX_ATTRIBUTES[] = {
    { .offset = offsetof(Vertex, pos),    .type = vulkan::VertexAttribute::Type::FLOAT3 },
    { .offset = offsetof(Vertex, normal), .type = vulkan::VertexAttribute::Type::FLOAT3 },
    { .offset = offsetof(Vertex, color),  .type = vulkan::VertexAttribute::Type::FLOAT3 },
    { .offset = offsetof(Vertex, uv),     .type = vulkan::VertexAttribute::Type::FLOAT2 },
  };

  static constexpr vulkan::VertexBinding VERTEX_BINDINGS[] = {{
    .stride          = sizeof(Vertex),
      .attributes      = VERTEX_ATTRIBUTES,
      .attribute_count = std::size(VERTEX_ATTRIBUTES),
  }};

  static constexpr vulkan::VertexInput VERTEX_INPUT = {
    .bindings      = VERTEX_BINDINGS,
    .binding_count = std::size(VERTEX_BINDINGS),
  };

}
