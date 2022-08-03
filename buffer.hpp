#pragma once

#include "vulkan.hpp"
#include "context.hpp"
#include "command_buffer.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define VK_CHECK(expr) do { if(expr != VK_SUCCESS) { fprintf(stderr, "Vulkan pooped itself:%s\n", #expr); } } while(0)

namespace vulkan
{
  struct Allocator
  {
    VkPhysicalDeviceMemoryProperties memory_properties;
  };

  inline Allocator create_allocator(const Context& context)
  {
    Allocator allocator = {};
    vkGetPhysicalDeviceMemoryProperties(context.physical_device, &allocator.memory_properties);

    // DEBUG:
    for(uint32_t i = 0; i<allocator.memory_properties.memoryTypeCount; ++i)
    {
      printf("Memory type %d\n", i);
      printf(" - heap_index     = %d\n",     allocator.memory_properties.memoryTypes[i].heapIndex);
      printf(" - property_flags = 0x%08x\n", allocator.memory_properties.memoryTypes[i].propertyFlags);
    }

    for(uint32_t i = 0; i<allocator.memory_properties.memoryHeapCount; ++i)
    {
      printf("Memory heap %d\n", i);
      printf(" - size  = %fGB\n", (float)allocator.memory_properties.memoryHeaps[i].size / (1024 * 1024 * 1024));
      printf(" - flags = 0x%08x\n", allocator.memory_properties.memoryHeaps[i].flags);
    }

    return allocator;
  }

  inline void destroy_allocator(const Allocator& allocator)
  {
    // Nothing to do
    (void)allocator;
  }

  inline uint32_t select_memory_type(const Allocator& allocator, uint32_t type_filter, VkMemoryPropertyFlags& memory_properties)
  {
    for (uint32_t i = 0; i < allocator.memory_properties.memoryTypeCount; i++)
    {
      if(!(type_filter & (1 << i)))
        continue;

      if((memory_properties & allocator.memory_properties.memoryTypes[i].propertyFlags) != memory_properties)
        continue;

      return i;
    }

    fprintf(stderr, "No memory type suitable");
    abort();
  }

  struct BufferAllocation
  {
    VkBuffer buffer;

    VkDeviceMemory memory;
    VkDeviceSize size;
    VkMemoryPropertyFlags memory_properties;
  };

  // Ideally, we would want the buffer memory to be device local
  //
  inline BufferAllocation allocate_buffer(const vulkan::Context& context, Allocator& allocator,
      VkDeviceSize size, VkBufferUsageFlags buffer_usage, VkMemoryPropertyFlags memory_properties)
  {
    BufferAllocation allocation = {};

    VkBufferCreateInfo buffer_create_info = {};
    buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_create_info.size        = size;
    buffer_create_info.usage       = buffer_usage;
    buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VK_CHECK(vkCreateBuffer(context.device, &buffer_create_info, nullptr, &allocation.buffer));

    // Even if we do not request host visible memory, the selected memory type may still be host visible
    // because that is the only memory type which is the case for integrated GPU.
    VkMemoryRequirements buffer_memory_requirement = {};
    vkGetBufferMemoryRequirements(context.device, allocation.buffer, &buffer_memory_requirement);
    uint32_t memory_type_index = select_memory_type(allocator, buffer_memory_requirement.memoryTypeBits, memory_properties);

    allocation.size              = size;
    allocation.memory_properties = allocator.memory_properties.memoryTypes[memory_type_index].propertyFlags;

    VkMemoryAllocateInfo allocate_info = {};
    allocate_info.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocate_info.memoryTypeIndex = memory_type_index;
    allocate_info.allocationSize  = buffer_memory_requirement.size;
    VK_CHECK(vkAllocateMemory(context.device, &allocate_info, nullptr, &allocation.memory));
    VK_CHECK(vkBindBufferMemory(context.device, allocation.buffer, allocation.memory, 0));

    return allocation;
  }

  inline void deallocate_buffer(const Context& context, const Allocator& allocator, BufferAllocation allocation)
  {
    (void)allocator;
    vkDestroyBuffer(context.device, allocation.buffer, nullptr);
    vkFreeMemory(context.device, allocation.memory, nullptr);
  }

  inline void write_buffer(const Context& context, Allocator& allocator, BufferAllocation& allocation, const void *data)
  {
    if(allocation.memory_properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
    {
      void *buffer_data;
      VK_CHECK(vkMapMemory(context.device, allocation.memory, 0, allocation.size, 0, &buffer_data));
      memcpy(buffer_data, data, allocation.size);
      vkUnmapMemory(context.device, allocation.memory);
      return;
    }

    auto staging_allocation = allocate_buffer(context, allocator, allocation.size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    {
      // Write to staging buffer which is HOST_VISIBLE
      write_buffer(context, allocator, staging_allocation, data);

      // Copy from staging_allocation to allocation
      auto command_buffer = create_command_buffer(context, false);

      vulkan::command_buffer_begin(command_buffer);

      VkBufferCopy buffer_copy = {};
      buffer_copy.srcOffset = 0;
      buffer_copy.dstOffset = 0;
      buffer_copy.size      = allocation.size;
      vkCmdCopyBuffer(command_buffer.handle, staging_allocation.buffer, allocation.buffer, 1, &buffer_copy);

      vulkan::command_buffer_end(command_buffer);
      vulkan::command_buffer_submit(context, command_buffer);
      vulkan::command_buffer_wait(context, command_buffer);

      destroy_command_buffer(context, command_buffer);
    }
    deallocate_buffer(context, allocator, staging_allocation);
  }
}
