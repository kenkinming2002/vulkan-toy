#include "mesh.hpp"

#include "tiny_obj_loader.h"

namespace vulkan
{
  // Layout
  struct MeshLayout
  {
    Ref ref;

    const MeshLayoutDescription *description;
    VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info;
  };

  void mesh_layout_free(ref_t ref)
  {
    mesh_layout_t mesh_layout = container_of(ref, MeshLayout, ref);
    delete[] mesh_layout->vertex_input_state_create_info.pVertexBindingDescriptions;
    delete[] mesh_layout->vertex_input_state_create_info.pVertexAttributeDescriptions;
    delete mesh_layout;
  }

  static void vertex_layout_description_visit(const VertexLayoutDescription *layout_description,
      auto binding_description_visitor,
      auto attribute_description_visitor)
  {
    for(uint32_t binding = 0; binding < layout_description->binding_count; ++binding)
    {
      const VertexBindingDescription *binding_description = &layout_description->bindings[binding];
      binding_description_visitor(binding, binding_description);
      for(uint32_t location = 0; location < binding_description->attribute_count; ++location)
      {
        const VertexAttributeDescription *attribute_description = &binding_description->attributes[location];
        attribute_description_visitor(binding, location, attribute_description);
      }
    }
  }

  static VkFormat to_vulkan_format(VertexAttributeDescription::Type type)
  {
    switch(type)
    {
    case VertexAttributeDescription::Type::FLOAT1: return VK_FORMAT_R32_SFLOAT;
    case VertexAttributeDescription::Type::FLOAT2: return VK_FORMAT_R32G32_SFLOAT;
    case VertexAttributeDescription::Type::FLOAT3: return VK_FORMAT_R32G32B32_SFLOAT;
    case VertexAttributeDescription::Type::FLOAT4: return VK_FORMAT_R32G32B32A32_SFLOAT;
    default:
      fprintf(stderr, "Unknown vertex attribute type\n");
      abort();
    }
  }

  mesh_layout_t mesh_layout_compile(const MeshLayoutDescription *mesh_layout_description)
  {
    mesh_layout_t mesh_layout = new MeshLayout;
    mesh_layout->ref.count = 1;
    mesh_layout->ref.free  = mesh_layout_free;

    mesh_layout->description = mesh_layout_description;

    // 2 loops, one for counting, one to allocate and fill
    uint32_t vertex_binding_description_count   = 0;
    uint32_t vertex_attribute_description_count = 0;

    vertex_layout_description_visit(&mesh_layout->description->vertex_layout,
      [&](uint32_t,           const VertexBindingDescription *)   { ++vertex_binding_description_count;   },
      [&](uint32_t, uint32_t, const VertexAttributeDescription *) { ++vertex_attribute_description_count; }
    );

    VkVertexInputBindingDescription   *vertex_binding_descriptions   = new VkVertexInputBindingDescription[vertex_binding_description_count];
    VkVertexInputAttributeDescription *vertex_attribute_descriptions = new VkVertexInputAttributeDescription[vertex_attribute_description_count];

    vertex_layout_description_visit(&mesh_layout->description->vertex_layout,
      [&, i=uint32_t(0)](uint32_t binding, const VertexBindingDescription *binding_description) mutable
      {
        vertex_binding_descriptions[i++] = VkVertexInputBindingDescription{
          .binding = binding,
          .stride    = static_cast<uint32_t>(binding_description->stride),
          .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
        };
      },
      [&, i=uint32_t(0)](uint32_t binding, uint32_t location, const VertexAttributeDescription *attribute_description) mutable
      {
        vertex_attribute_descriptions[i++] = VkVertexInputAttributeDescription{
          .location = location,
          .binding = binding,
          .format = to_vulkan_format(attribute_description->type),
          .offset = static_cast<uint32_t>(attribute_description->offset),
        };
      }
    );

    mesh_layout->vertex_input_state_create_info = {};
    mesh_layout->vertex_input_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    mesh_layout->vertex_input_state_create_info.vertexBindingDescriptionCount   = vertex_binding_description_count;
    mesh_layout->vertex_input_state_create_info.pVertexBindingDescriptions      = vertex_binding_descriptions;
    mesh_layout->vertex_input_state_create_info.vertexAttributeDescriptionCount = vertex_attribute_description_count;
    mesh_layout->vertex_input_state_create_info.pVertexAttributeDescriptions    = vertex_attribute_descriptions;


    return mesh_layout;
  }

  ref_t mesh_layout_as_ref(mesh_layout_t mesh_layout)
  {
    return &mesh_layout->ref;
  }

  static constexpr VertexAttributeDescription VERTEX_ATTRIBUTE_DESCRIPTIONS[] = {
    { .offset = offsetof(Vertex, pos),    .type = vulkan::VertexAttributeDescription::Type::FLOAT3 },
    { .offset = offsetof(Vertex, normal), .type = vulkan::VertexAttributeDescription::Type::FLOAT3 },
    { .offset = offsetof(Vertex, color),  .type = vulkan::VertexAttributeDescription::Type::FLOAT3 },
    { .offset = offsetof(Vertex, uv),     .type = vulkan::VertexAttributeDescription::Type::FLOAT2 },
  };

  static constexpr VertexBindingDescription VERTEX_BINDING_DESCRIPTIONS[] = {{
    .stride          = sizeof(Vertex),
    .attributes      = VERTEX_ATTRIBUTE_DESCRIPTIONS,
    .attribute_count = std::size(VERTEX_ATTRIBUTE_DESCRIPTIONS),
  }};

  static constexpr VertexLayoutDescription VERTEX_LAYOUT_DESCRIPTION = {
    .bindings      = VERTEX_BINDING_DESCRIPTIONS,
    .binding_count = std::size(VERTEX_BINDING_DESCRIPTIONS),
  };

  static constexpr MeshLayoutDescription MESH_LAYOUT_DESCRIPTION = {
    .vertex_layout = VERTEX_LAYOUT_DESCRIPTION,
  };

  mesh_layout_t mesh_layout_create_default()
  {
    return mesh_layout_compile(&MESH_LAYOUT_DESCRIPTION);
  }


  // Mesh
  struct Mesh
  {
    Ref ref;

    context_t   context;
    allocator_t allocator;

    mesh_layout_t layout;

    size_t vertex_count;
    size_t index_count;

    buffer_t *vertex_buffers;
    buffer_t index_buffer;
  };

  static void mesh_free(ref_t ref)
  {
    mesh_t mesh = container_of(ref, Mesh, ref);

    const size_t vertex_buffer_count = mesh->layout->description->vertex_layout.binding_count;
    printf("vertex buffer count = %zu\n", vertex_buffer_count);
    for(size_t i=0; i<vertex_buffer_count; ++i)
      buffer_put(mesh->vertex_buffers[i]);

    delete[] mesh->vertex_buffers;

    buffer_put(mesh->index_buffer);

    mesh_layout_put(mesh->layout);
    allocator_put(mesh->allocator);
    context_put(mesh->context);

    delete mesh;
  }

  mesh_t mesh_create(context_t context, allocator_t allocator, mesh_layout_t mesh_layout, size_t vertex_count, size_t index_count)
  {
    mesh_t mesh = new Mesh {};
    mesh->ref.count = 1;
    mesh->ref.free  = &mesh_free;

    context_get(context);
    mesh->context = context;

    allocator_get(allocator);
    mesh->allocator = allocator;

    mesh_layout_get(mesh_layout);
    mesh->layout = mesh_layout;

    mesh->vertex_count = vertex_count;
    mesh->index_count  = index_count;

    // Vertex buffers
    const size_t vertex_buffer_count = mesh->layout->description->vertex_layout.binding_count;
    mesh->vertex_buffers = new buffer_t[vertex_buffer_count];
    for(size_t i=0; i<vertex_buffer_count; ++i)
    {
      const size_t vertex_buffer_stride = mesh->layout->description->vertex_layout.bindings[i].stride;
      mesh->vertex_buffers[i] = buffer_create(mesh->context, mesh->allocator, BufferType::VERTEX_BUFFER, vertex_buffer_stride * mesh->vertex_count);
    }

    // Index buffers
    // TODO: Allow specifying index type
    mesh->index_buffer = buffer_create(mesh->context, mesh->allocator, BufferType::INDEX_BUFFER,  sizeof(uint32_t) * mesh->index_count);

    return mesh;
  }

  ref_t mesh_as_ref(mesh_t mesh)
  {
    return &mesh->ref;
  }

  void mesh_write(command_buffer_t command_buffer, mesh_t mesh, const void **vertices, const uint32_t *indices)
  {
    // Vertex buffers
    const size_t vertex_buffer_count = mesh->layout->description->vertex_layout.binding_count;
    for(size_t i=0; i<vertex_buffer_count; ++i)
    {
      const size_t vertex_buffer_stride = mesh->layout->description->vertex_layout.bindings[i].stride;
      buffer_write(command_buffer, mesh->vertex_buffers[i], vertices[i], vertex_buffer_stride * mesh->vertex_count);
    }

    // Index buffers
    buffer_write(command_buffer, mesh->index_buffer, indices, sizeof(uint32_t) * mesh->index_count);
  }

  mesh_t mesh_load(command_buffer_t command_buffer, context_t context, allocator_t allocator, const char *file_name)
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

    mesh_layout_t mesh_layout = mesh_layout_create_default();
    mesh_t        mesh        = mesh_create(context, allocator, mesh_layout, vertices.size(), indices.size());
    mesh_layout_put(mesh_layout);

    const uint32_t *_indices    = indices.data();
    const void     *_vertices[] = { vertices.data() };
    mesh_write(command_buffer, mesh, _vertices, _indices);
    return mesh;
  }

  void mesh_render_simple(command_buffer_t command_buffer, mesh_t mesh)
  {
    VkCommandBuffer handle = command_buffer_get_handle(command_buffer);

    // Vertex buffers
    const size_t vertex_buffer_count = mesh->layout->description->vertex_layout.binding_count;
    VkDeviceSize *offsets        = new VkDeviceSize[vertex_buffer_count];
    VkBuffer     *vertex_buffers = new VkBuffer[vertex_buffer_count];
    for(size_t i=0; i<vertex_buffer_count; ++i)
    {
      command_buffer_use(command_buffer, buffer_as_ref(mesh->vertex_buffers[i]));

      offsets[i] = 0;
      vertex_buffers[i] = buffer_get_handle(mesh->vertex_buffers[i]);
    }
    vkCmdBindVertexBuffers(handle, 0, 1, vertex_buffers, offsets);

    delete[] offsets;
    delete[] vertex_buffers;

    // Index buffers
    command_buffer_use(command_buffer, buffer_as_ref(mesh->index_buffer));
    vkCmdBindIndexBuffer(handle, buffer_get_handle(mesh->index_buffer), 0, VK_INDEX_TYPE_UINT32);

    vkCmdDrawIndexed(handle, mesh->index_count, 1, 0, 0, 0);
  }
}
