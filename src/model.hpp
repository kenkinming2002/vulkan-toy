#pragma once

#include "input.hpp"
#include "buffer.hpp"
#include "command_buffer.hpp"

#include <glm/glm.hpp>

#include <array>

namespace vulkan
{
  struct Vertex
  {
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 uv;
  };

  static constexpr vulkan::VertexAttribute VERTEX_ATTRIBUTES[] = {
    { .offset = offsetof(Vertex, pos),   .type = vulkan::VertexAttribute::Type::FLOAT3 },
    { .offset = offsetof(Vertex, color), .type = vulkan::VertexAttribute::Type::FLOAT3 },
    { .offset = offsetof(Vertex, uv),    .type = vulkan::VertexAttribute::Type::FLOAT2 },
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

  struct ModelCreateInfo
  {
    const Vertex *vertices;
    size_t        vertex_count;

    const uint32_t *indices;
    size_t          index_count;
  };

  struct Model
  {
    size_t vertex_count;
    size_t index_count;

    Buffer vertex_buffer;
    Buffer index_buffer;
  };

  void init_model(const Context& context, Allocator& allocator, ModelCreateInfo create_info, Model& model);
  void deinit_model(const Context& context, Allocator& allocator, Model& model);
  void load_model(const Context& context, Allocator& allocator, const char *file_name, Model& model);

  void command_model_render_simple(VkCommandBuffer command_buffer, Model model);
}
