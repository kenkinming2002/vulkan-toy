#include "mesh.hpp"

#include "tiny_obj_loader.h"

namespace vulkan
{
  struct Mesh
  {
    Ref ref;

    context_t context;
    Allocator     *allocator;

    size_t vertex_count;
    size_t index_count;

    buffer_t vertex_buffer;
    buffer_t index_buffer;
  };

  static void mesh_free(ref_t ref)
  {
    mesh_t mesh = container_of(ref, Mesh, ref);
    buffer_put(mesh->vertex_buffer);
    buffer_put(mesh->index_buffer);
    context_put(mesh->context);
    delete mesh;
  }

  mesh_t mesh_create(context_t context, Allocator *allocator, size_t vertex_count, size_t index_count)
  {
    mesh_t mesh = new Mesh {};
    mesh->ref.count = 1;
    mesh->ref.free  = &mesh_free;

    context_get(context);
    mesh->context   = context;

    mesh->allocator = allocator;

    mesh->vertex_count = vertex_count;
    mesh->index_count  = index_count;

    mesh->vertex_buffer = buffer_create(mesh->context, mesh->allocator, BufferType::VERTEX_BUFFER, sizeof(Vertex)   * mesh->vertex_count);
    mesh->index_buffer  = buffer_create(mesh->context, mesh->allocator, BufferType::INDEX_BUFFER,  sizeof(uint32_t) * mesh->index_count);

    return mesh;
  }

  ref_t mesh_as_ref(mesh_t mesh)
  {
    return &mesh->ref;
  }

  void mesh_write(command_buffer_t command_buffer, mesh_t mesh, const Vertex *vertices, const uint32_t *indices)
  {
    buffer_write(command_buffer, mesh->vertex_buffer, vertices, sizeof(Vertex)   * mesh->vertex_count);
    buffer_write(command_buffer, mesh->index_buffer,  indices,  sizeof(uint32_t) * mesh->index_count);
  }

  mesh_t mesh_load(command_buffer_t command_buffer, context_t context, Allocator *allocator, const char *file_name)
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

    mesh_t mesh = mesh_create(context, allocator, vertices.size(), indices.size());
    mesh_write(command_buffer, mesh, vertices.data(), indices.data());
    return mesh;
  }

  void mesh_render_simple(command_buffer_t command_buffer, mesh_t mesh)
  {
    VkDeviceSize offsets[] = {0};

    VkCommandBuffer handle = command_buffer_get_handle(command_buffer);
    command_buffer_use(command_buffer, buffer_as_ref(mesh->vertex_buffer));
    command_buffer_use(command_buffer, buffer_as_ref(mesh->index_buffer));
    VkBuffer vertex_buffer_handle = buffer_get_handle(mesh->vertex_buffer); // TODO: Consider putting this inside the buffer module
    VkBuffer index_buffer_handle  = buffer_get_handle(mesh->index_buffer);  // TODO: Consider putting this inside the buffer module

    vkCmdBindVertexBuffers(handle, 0, 1, &vertex_buffer_handle, offsets);
    vkCmdBindIndexBuffer(handle, index_buffer_handle, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(handle, mesh->index_count, 1, 0, 0, 0);
  }
}
