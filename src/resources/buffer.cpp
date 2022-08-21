#include "buffer.hpp"

#include "allocator.hpp"
#include "vk_check.hpp"

#include <string.h>
#include <assert.h>

namespace vulkan
{
  static VkBufferUsageFlags get_vulkan_buffer_usage(BufferType type)
  {
    switch(type)
    {
    case BufferType::STAGING_BUFFER: return VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    case BufferType::VERTEX_BUFFER:  return VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    case BufferType::INDEX_BUFFER:   return VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    case BufferType::UNIFORM_BUFFER: return VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    default: assert(false && "Unreachable");
    }
  }

  static VkMemoryPropertyFlags get_vulkan_buffer_memory_properties(BufferType type)
  {
    switch(type)
    {
    case BufferType::STAGING_BUFFER: return VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    case BufferType::VERTEX_BUFFER:  return VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    case BufferType::INDEX_BUFFER:   return VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    case BufferType::UNIFORM_BUFFER: return VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    default: assert(false && "Unreachable");
    }
  }

  struct Buffer
  {
    Ref ref;

    context_t   context;
    allocator_t allocator;

    VkBuffer        handle;
    device_memory_t device_memory;
  };

  static void buffer_free(ref_t ref)
  {
    buffer_t buffer = container_of(ref, Buffer, ref);

    VkDevice device = context_get_device_handle(buffer->context);

    vkDestroyBuffer(device, buffer->handle, nullptr);
    device_memory_put(buffer->device_memory);

    context_put(buffer->context);
    allocator_put(buffer->allocator);

    delete buffer;
  }

  buffer_t buffer_create(context_t context, allocator_t allocator, BufferType type, size_t size)
  {
    buffer_t buffer = new Buffer {};
    buffer->ref.count = 1;
    buffer->ref.free  = &buffer_free;

    context_get(context);
    buffer->context = context;
    allocator_get(allocator);
    buffer->allocator = allocator;

    VkDevice device = context_get_device_handle(buffer->context);

    VkBufferCreateInfo buffer_create_info = {};
    buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_create_info.size        = size;
    buffer_create_info.usage       = get_vulkan_buffer_usage(type);
    buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VK_CHECK(vkCreateBuffer(device, &buffer_create_info, nullptr, &buffer->handle));

    VkMemoryRequirements memory_requirements = {};
    vkGetBufferMemoryRequirements(device, buffer->handle, &memory_requirements);

    buffer->device_memory = device_memory_allocate(buffer->allocator,
        memory_requirements.memoryTypeBits,
        get_vulkan_buffer_memory_properties(type),
        memory_requirements.size);

    VK_CHECK(vkBindBufferMemory(device, buffer->handle, device_memory_get_handle(buffer->device_memory), 0));

    return buffer;
  }

  ref_t buffer_as_ref(buffer_t buffer) { return &buffer->ref; }

  VkBuffer buffer_get_handle(buffer_t buffer)
  {
    return buffer->handle;
  }

  void buffer_copy(command_buffer_t command_buffer, buffer_t src, buffer_t dst, size_t size)
  {
    VkCommandBuffer handle = command_buffer_get_handle(command_buffer);
    command_buffer_use(command_buffer, buffer_as_ref(src));
    command_buffer_use(command_buffer, buffer_as_ref(dst));

    VkBufferCopy buffer_copy = {};
    buffer_copy.srcOffset = 0;
    buffer_copy.dstOffset = 0;
    buffer_copy.size      = size;
    vkCmdCopyBuffer(handle, src->handle, dst->handle, 1, &buffer_copy);
  }

  void buffer_write(command_buffer_t command_buffer, buffer_t buffer, const void *data, size_t size)
  {
    if(device_memory_mappable(buffer->device_memory))
    {
      void *buffer_data = device_memory_map(buffer->device_memory);
      memcpy(buffer_data, data, size);
      device_memory_unmap(buffer->device_memory);
    }
    else
    {
      buffer_t staging_buffer = buffer_create(buffer->context, buffer->allocator, BufferType::STAGING_BUFFER, size);
      buffer_write(command_buffer, staging_buffer, data, size);

      VkCommandBuffer handle = command_buffer_get_handle(command_buffer);
      command_buffer_use(command_buffer, buffer_as_ref(staging_buffer));
      command_buffer_use(command_buffer, buffer_as_ref(buffer));

      VkBufferCopy buffer_copy = {};
      buffer_copy.srcOffset = 0;
      buffer_copy.dstOffset = 0;
      buffer_copy.size      = size;
      vkCmdCopyBuffer(handle, buffer_get_handle(staging_buffer), buffer_get_handle(buffer), 1, &buffer_copy);

      buffer_put(staging_buffer);
    }
  }
}
