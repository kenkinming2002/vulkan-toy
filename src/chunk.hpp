#pragma once

#include "resources/mesh.hpp"
#include "utils.hpp"

#include <glm/glm.hpp>

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

struct Block
{
  bool valid;
  glm::vec3 color;
};

static constexpr size_t CHUNK_SIZE = 8;
static constexpr float  BLOCK_WIDTH = 0.2f;


struct Chunk
{
  Block blocks[CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE];
};

inline Chunk *chunk_generate_random()
{
  srand(time(NULL));
  Chunk *chunk = new Chunk;
  for(size_t z=0; z<CHUNK_SIZE; ++z)
    for(size_t y=0; y<CHUNK_SIZE; ++y)
      for(size_t x=0; x<CHUNK_SIZE; ++x)
      {
        chunk->blocks[z][y][x].valid = (unsigned)rand() % 8 == 0;
        chunk->blocks[z][y][x].color = glm::vec3(x, y, z) * 2.0f / (float)CHUNK_SIZE;
      }

  return chunk;
}

inline vulkan::mesh_t chunk_generate_mesh(vulkan::context_t context, vulkan::allocator_t allocator, const Chunk& chunk)
{
  vector<vulkan::Vertex> vertices = create_vector<vulkan::Vertex>(1);
  vector<uint32_t>       indices  = create_vector<uint32_t>(1);

  for(size_t z=0; z<CHUNK_SIZE; ++z)
    for(size_t y=0; y<CHUNK_SIZE; ++y)
      for(size_t x=0; x<CHUNK_SIZE; ++x)
      {
        if(!chunk.blocks[z][y][x].valid)
          continue;

        vulkan::Vertex block_vertices[] = {
          {.pos = glm::vec3(x,   y,   z  ) * BLOCK_WIDTH, .normal = {}, .color = chunk.blocks[z][y][x].color, .uv = {}},
          {.pos = glm::vec3(x,   y,   z+1) * BLOCK_WIDTH, .normal = {}, .color = chunk.blocks[z][y][x].color, .uv = {}},
          {.pos = glm::vec3(x,   y+1, z  ) * BLOCK_WIDTH, .normal = {}, .color = chunk.blocks[z][y][x].color, .uv = {}},
          {.pos = glm::vec3(x,   y+1, z+1) * BLOCK_WIDTH, .normal = {}, .color = chunk.blocks[z][y][x].color, .uv = {}},
          {.pos = glm::vec3(x+1, y,   z  ) * BLOCK_WIDTH, .normal = {}, .color = chunk.blocks[z][y][x].color, .uv = {}},
          {.pos = glm::vec3(x+1, y,   z+1) * BLOCK_WIDTH, .normal = {}, .color = chunk.blocks[z][y][x].color, .uv = {}},
          {.pos = glm::vec3(x+1, y+1, z  ) * BLOCK_WIDTH, .normal = {}, .color = chunk.blocks[z][y][x].color, .uv = {}},
          {.pos = glm::vec3(x+1, y+1, z+1) * BLOCK_WIDTH, .normal = {}, .color = chunk.blocks[z][y][x].color, .uv = {}},
        };
        uint32_t block_indices[] = {
          3, 2, 0, 1, 3, 0,
          5, 4, 6, 7, 5, 6,
          7, 6, 2, 3, 7, 2,
          1, 0, 4, 5, 1, 4,
          6, 4, 0, 2, 6, 0,
          1, 5, 7, 1, 7, 3,
        };

        for(size_t i=0; i<std::size(block_indices); ++i)
          vector_resize_push<uint32_t>(indices, size(vertices) + block_indices[i]);

        for(size_t i=0; i<std::size(block_vertices); ++i)
          vector_resize_push(vertices, block_vertices[i]);
      }

  printf("vertices size = %ld\n", size(vertices));
  printf("indices  size = %ld\n", size(indices));

  vulkan::mesh_layout_t mesh_layout = vulkan::mesh_layout_create_default();
  vulkan::mesh_t        mesh        = vulkan::mesh_create(context, allocator, mesh_layout, size(vertices), size(indices));
  vulkan::put(mesh_layout);

  vulkan::command_buffer_t command_buffer = vulkan::command_buffer_create(context);
  command_buffer_begin(command_buffer);

  const void     *_vertices[] = { data(vertices) };
  const uint32_t *_indices    = data(indices);
  vulkan::mesh_write(command_buffer, mesh, _vertices, _indices);

  command_buffer_end(command_buffer);
  command_buffer_submit(command_buffer);
  command_buffer_wait(command_buffer);
  put(command_buffer);

  destroy_vector(vertices);
  destroy_vector(indices);
  return mesh;
}

