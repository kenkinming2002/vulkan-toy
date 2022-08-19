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

  struct MeshCreateInfo
  {
    const Vertex *vertices;
    size_t        vertex_count;

    const uint32_t *indices;
    size_t          index_count;
  };

  struct Mesh
  {
    size_t vertex_count;
    size_t index_count;

    buffer_t vertex_buffer;
    buffer_t index_buffer;
  };

  void mesh_init(const Context& context, Allocator& allocator, MeshCreateInfo create_info, Mesh& mesh);
  void mesh_deinit(const Context& context, Allocator& allocator, Mesh& model);
  void mesh_load(const Context& context, Allocator& allocator, const char *file_name, Mesh& mesh);

  void mesh_render_simple(VkCommandBuffer command_buffer, Mesh mesh);
}
