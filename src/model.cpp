#include "model.hpp"

#include "tiny_obj_loader.h"

namespace vulkan
{
  void init_model(const Context& context, Allocator& allocator, ModelCreateInfo create_info, Model& model)
  {
    model.vertex_count = create_info.vertex_count;
    model.index_count  = create_info.index_count;

    vulkan::BufferCreateInfo buffer_create_info = {};

    buffer_create_info.type = vulkan::BufferType::VERTEX_BUFFER;
    buffer_create_info.size = create_info.vertex_count * sizeof create_info.vertices[0];
    vulkan::init_buffer(context, allocator, buffer_create_info, model.vertex_buffer);
    vulkan::write_buffer(context, allocator, model.vertex_buffer, create_info.vertices, create_info.vertex_count * sizeof create_info.vertices[0]);

    buffer_create_info.type = vulkan::BufferType::INDEX_BUFFER;
    buffer_create_info.size = create_info.index_count * sizeof create_info.indices[0];
    vulkan::init_buffer(context, allocator, buffer_create_info, model.index_buffer);
    vulkan::write_buffer(context, allocator, model.index_buffer, create_info.indices, create_info.index_count * sizeof create_info.indices[0]);
  }

  void deinit_model(const Context& context, Allocator& allocator, Model& model)
  {
    vulkan::deinit_buffer(context, allocator, model.vertex_buffer);
    vulkan::deinit_buffer(context, allocator, model.index_buffer);
  }

  void load_model(const Context& context, Allocator& allocator, const char *file_name, Model& model)
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
        vertex.uv = {
          attrib.texcoords[2 * index.texcoord_index + 0],
          1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
        };
        vertex.color = {1.0f, 1.0f, 1.0f};
        vertices.push_back(vertex);
        indices.push_back(indices.size());
      }

    ModelCreateInfo create_info = {
      .vertices     = vertices.data(),
      .vertex_count = vertices.size(),
      .indices     = indices.data(),
      .index_count = indices.size(),
    };
    init_model(context, allocator, create_info, model);
  }

  void command_model_render_simple(CommandBuffer command_buffer, Model model)
  {
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(command_buffer.handle, 0, 1, &model.vertex_buffer.handle, offsets);
    vkCmdBindIndexBuffer(command_buffer.handle, model.index_buffer.handle, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(command_buffer.handle, model.index_count, 1, 0, 0, 0);
  }
}
