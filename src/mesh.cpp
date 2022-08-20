#include "mesh.hpp"

#include "tiny_obj_loader.h"

namespace vulkan
{
  void mesh_init(const Context& context, Allocator& allocator, MeshCreateInfo create_info, Mesh& mesh)
  {
    mesh.vertex_count = create_info.vertex_count;
    mesh.index_count  = create_info.index_count;

    mesh.vertex_buffer = buffer_create(&context, &allocator, vulkan::BufferType::VERTEX_BUFFER, create_info.vertex_count * sizeof create_info.vertices[0]);
    mesh.index_buffer  = buffer_create(&context, &allocator, vulkan::BufferType::INDEX_BUFFER,  create_info.index_count  * sizeof create_info.indices[0]);

    command_buffer_t command_buffer = command_buffer_create(&context);

    command_buffer_begin(command_buffer);
    buffer_write(command_buffer, mesh.vertex_buffer, create_info.vertices, create_info.vertex_count * sizeof create_info.vertices[0]);
    buffer_write(command_buffer, mesh.index_buffer,  create_info.indices,  create_info.index_count  * sizeof create_info.indices[0]);
    command_buffer_end(command_buffer);

    Fence fence = {};
    init_fence(context, fence, false);
    command_buffer_submit(context, command_buffer, fence);
    fence_wait_and_reset(context, fence);
    deinit_fence(context, fence);

    command_buffer_put(command_buffer);
  }

  void mesh_deinit(const Context& context, Allocator& allocator, Mesh& model)
  {
    (void)context;
    (void)allocator;
    vulkan::buffer_put(model.vertex_buffer);
    vulkan::buffer_put(model.index_buffer);
  }

  void mesh_load(const Context& context, Allocator& allocator, const char *file_name, Mesh& mesh)
  {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;
    std::vector<Vertex>   vertices;
    std::vector<uint32_t> indices;

    if(!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, file_name))
    {
      fprintf(stderr, "Failed to load object file:%s", err.c_str());
      abort();
    }
    fprintf(stderr, "Warning:%s", warn.c_str());

    for(const auto& shape : shapes)
      for(const auto& index : shape.mesh.indices)
      {
        Vertex vertex = {};
        vertex.pos = {
          attrib.vertices[3 * index.vertex_index + 0],
          attrib.vertices[3 * index.vertex_index + 1],
          attrib.vertices[3 * index.vertex_index + 2]
        };
        vertex.normal = {
          attrib.normals[3 * index.normal_index + 0],
          attrib.normals[3 * index.normal_index + 1],
          attrib.normals[3 * index.normal_index + 2]
        };
        vertex.uv = {
          attrib.texcoords[2 * index.texcoord_index + 0],
          1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
        };
        vertex.color = {1.0f, 1.0f, 1.0f};
        vertices.push_back(vertex);
        indices.push_back(indices.size());
      }

    MeshCreateInfo create_info = {
      .vertices     = vertices.data(),
      .vertex_count = vertices.size(),
      .indices     = indices.data(),
      .index_count = indices.size(),
    };
    mesh_init(context, allocator, create_info, mesh);
  }

  void mesh_render_simple(VkCommandBuffer command_buffer, Mesh mesh)
  {
    VkDeviceSize offsets[] = {0};

    VkBuffer vertex_buffer_handle = buffer_get_handle(mesh.vertex_buffer);
    VkBuffer index_buffer_handle  = buffer_get_handle(mesh.index_buffer);

    vkCmdBindVertexBuffers(command_buffer, 0, 1, &vertex_buffer_handle, offsets);
    vkCmdBindIndexBuffer(command_buffer, index_buffer_handle, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(command_buffer, mesh.index_count, 1, 0, 0, 0);
  }
}
