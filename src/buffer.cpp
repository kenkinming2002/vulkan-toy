#include "buffer.hpp"

#include "command_buffer.hpp"
#include "fence.hpp"
#include "allocator.hpp"
#include "vk_check.hpp"

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

  void init_buffer(const Context& context, Allocator& allocator, BufferCreateInfo create_info, Buffer& buffer)
  {
    VkBufferCreateInfo buffer_create_info = {};
    buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_create_info.size        = create_info.size;
    buffer_create_info.usage       = get_vulkan_buffer_usage(create_info.type);
    buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VK_CHECK(vkCreateBuffer(context.device, &buffer_create_info, nullptr, &buffer.handle));

    VkMemoryRequirements memory_requirements = {};
    vkGetBufferMemoryRequirements(context.device, buffer.handle, &memory_requirements);

    MemoryAllocationInfo allocation_info = {};
    allocation_info.type_bits  = memory_requirements.memoryTypeBits;
    allocation_info.properties = get_vulkan_buffer_memory_properties(create_info.type);
    allocation_info.size       = memory_requirements.size;
    allocate_memory(context, allocator, allocation_info, buffer.allocation);
    VK_CHECK(vkBindBufferMemory(context.device, buffer.handle, buffer.allocation.memory, 0));
  }

  void deinit_buffer(const Context& context, Allocator& allocator, Buffer& buffer)
  {
    deallocate_memory(context, allocator, buffer.allocation);
    vkDestroyBuffer(context.device, buffer.handle, nullptr);
    buffer = {};
  }

  void write_buffer(const Context& context, Allocator& allocator, Buffer& buffer, const void *data, size_t size)
  {
    VkMemoryPropertyFlags memory_properties = allocator.memory_properties.memoryTypes[buffer.allocation.type_index].propertyFlags;
    if(memory_properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
    {
      // Nice, the memory is host visible so we can directly map it
      void *buffer_data;
      VK_CHECK(vkMapMemory(context.device, buffer.allocation.memory, 0, size, 0, &buffer_data));
      memcpy(buffer_data, data, size);
      vkUnmapMemory(context.device, buffer.allocation.memory);
      return;
    }

    Buffer staging_buffer = {};

    BufferCreateInfo buffer_create_info = {};
    buffer_create_info.type = BufferType::STAGING_BUFFER;
    buffer_create_info.size = size;
    init_buffer(context, allocator, buffer_create_info, staging_buffer);
    {
      write_buffer(context, allocator, staging_buffer, data, size);

      CommandBuffer command_buffer = {};
      init_command_buffer(context, command_buffer);

      command_buffer_begin(command_buffer);
      {
        VkBufferCopy buffer_copy = {};
        buffer_copy.srcOffset = 0;
        buffer_copy.dstOffset = 0;
        buffer_copy.size      = size;
        vkCmdCopyBuffer(command_buffer.handle, staging_buffer.handle, buffer.handle, 1, &buffer_copy);
      }
      command_buffer_end(command_buffer);

      Fence fence = {};
      init_fence(context, fence, false);
      command_buffer_submit(context, command_buffer, fence);
      fence_wait_and_reset(context, fence);
      deinit_fence(context, fence);

      deinit_command_buffer(context, command_buffer);
    }
    deinit_buffer(context, allocator, staging_buffer);
  }
}
