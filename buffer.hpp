#pragma once

#include "vulkan.hpp"
#include "context.hpp"

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
      printf(" - heap_index     = %d\n", allocator.memory_properties.memoryTypes[i].heapIndex);
      printf(" - property_flags = %d\n", allocator.memory_properties.memoryTypes[i].propertyFlags);
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

  inline uint32_t select_memory_type(const Allocator& allocator, uint32_t type_filter, VkMemoryPropertyFlags memory_properties)
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
  };

  // Ideally, we would want the buffer memory to be device local

  inline BufferAllocation allocate_buffer(const vulkan::Context& context, Allocator& allocator,
      VkDeviceSize size, VkBufferUsageFlags buffer_usage,
      VkMemoryPropertyFlags memory_properties)
  {
    BufferAllocation allocation = {};

    VkBufferCreateInfo buffer_create_info = {};
    buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_create_info.size        = size;
    buffer_create_info.usage       = buffer_usage;
    buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VK_CHECK(vkCreateBuffer(context.device, &buffer_create_info, nullptr, &allocation.buffer));

    VkMemoryRequirements buffer_memory_requirement = {};
    vkGetBufferMemoryRequirements(context.device, allocation.buffer, &buffer_memory_requirement);
    uint32_t memory_type_index = select_memory_type(allocator, buffer_memory_requirement.memoryTypeBits, memory_properties);

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

  // Write to buffer, buffer must be created with VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
  inline void write_buffer(const Context& context, BufferAllocation& allocation, const void *data, size_t size)
  {
    void *buffer_data;
    VK_CHECK(vkMapMemory(context.device, allocation.memory, 0, size, 0, &buffer_data));
    memcpy(buffer_data, data, size);
    vkUnmapMemory(context.device, allocation.memory);
  }
}
