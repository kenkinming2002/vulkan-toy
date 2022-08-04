#include "buffer.hpp"

#include "malloc.h"

namespace vulkan
{
  Allocator create_allocator(const Context& context)
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

  void destroy_allocator(Allocator allocator)
  {
    // Nothing to do
    (void)allocator;
  }

  struct MemoryAllocation
  {
    VkDeviceMemory        memory;
    VkDeviceSize          size;
    VkMemoryPropertyFlags memory_properties;
  };

  template<typename T>
  static inline bool is_bits_set(T value, T flags)
  {
    return (value & flags) == flags;
  }

  static inline MemoryAllocation allocate_device_memory(const Context& context, Allocator& allocator, VkMemoryRequirements memory_requirements, VkMemoryPropertyFlags memory_properties)
  {
    for (uint32_t i = 0; i < allocator.memory_properties.memoryTypeCount; i++)
    {
      VkMemoryType memory_type = allocator.memory_properties.memoryTypes[i];
      if(is_bits_set(memory_requirements.memoryTypeBits, uint32_t(1 << i)) && is_bits_set(memory_type.propertyFlags, memory_properties))
      {
        MemoryAllocation allocation = {};

        VkMemoryAllocateInfo allocate_info = {};
        allocate_info.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocate_info.memoryTypeIndex = i;
        allocate_info.allocationSize  = memory_requirements.size;
        VK_CHECK(vkAllocateMemory(context.device, &allocate_info, nullptr, &allocation.memory));
        allocation.size              = memory_requirements.size;
        allocation.memory_properties = memory_type.propertyFlags;

        return allocation;
      }
    }

    fprintf(stderr, "No memory type while allocating device memory");
    abort();
  }

  BufferAllocation allocate_buffer(
      const vulkan::Context& context,
      Allocator& allocator,
      VkDeviceSize size,
      VkBufferUsageFlags buffer_usage,
      VkMemoryPropertyFlags memory_properties)
  {
    BufferAllocation allocation = {};

    VkBufferCreateInfo buffer_create_info = {};
    buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_create_info.size        = size;
    buffer_create_info.usage       = buffer_usage;
    buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VK_CHECK(vkCreateBuffer(context.device, &buffer_create_info, nullptr, &allocation.buffer));

    VkMemoryRequirements buffer_memory_requirements = {};
    vkGetBufferMemoryRequirements(context.device, allocation.buffer, &buffer_memory_requirements);

    MemoryAllocation memory_allocation = allocate_device_memory(context, allocator, buffer_memory_requirements, memory_properties);
    allocation.memory            = memory_allocation.memory;
    allocation.size              = memory_allocation.size;
    allocation.memory_properties = memory_allocation.memory_properties;
    VK_CHECK(vkBindBufferMemory(context.device, allocation.buffer, memory_allocation.memory, 0));
    return allocation;
  }

  void deallocate_buffer(
      const Context& context,
      Allocator& allocator,
      BufferAllocation allocation)
  {
    (void)allocator;
    vkDestroyBuffer(context.device, allocation.buffer, nullptr);
    vkFreeMemory(context.device, allocation.memory, nullptr);
  }

  ImageAllocation allocate_image2d(
      const vulkan::Context& context,
      Allocator& allocator,
      VkFormat format,
      uint32_t width, uint32_t height,
      VkImageUsageFlags image_usage,
      VkMemoryPropertyFlags memory_properties)
  {
    ImageAllocation allocation = {};

    VkImageCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    create_info.flags         = 0;
    create_info.imageType     = VK_IMAGE_TYPE_2D;
    create_info.format        = format;
    create_info.extent.width  = width;
    create_info.extent.height = height;
    create_info.extent.depth  = 1;
    create_info.mipLevels     = 1;
    create_info.arrayLayers   = 1;
    create_info.samples       = VK_SAMPLE_COUNT_1_BIT;
    create_info.tiling        = VK_IMAGE_TILING_OPTIMAL;
    create_info.usage         = image_usage;
    create_info.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
    create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VK_CHECK(vkCreateImage(context.device, &create_info, nullptr, &allocation.image));

    VkMemoryRequirements memory_requirements = {};
    vkGetImageMemoryRequirements(context.device, allocation.image, &memory_requirements);

    MemoryAllocation memory_allocation = allocate_device_memory(context, allocator, memory_requirements, memory_properties);
    allocation.width             = width;
    allocation.height            = height;
    allocation.memory            = memory_allocation.memory;
    allocation.size              = memory_allocation.size;
    allocation.memory_properties = memory_allocation.memory_properties;
    VK_CHECK(vkBindImageMemory(context.device, allocation.image, memory_allocation.memory, 0));
    return allocation;
  }

  void deallocate_image2d(
      const Context& context,
      Allocator& allocator,
      ImageAllocation allocation)
  {
    (void)allocator;
    vkDestroyImage(context.device, allocation.image, nullptr);
    vkFreeMemory(context.device, allocation.memory, nullptr);
  }

  void write_buffer(const Context& context, Allocator& allocator, BufferAllocation& allocation, const void *data)
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

  void write_image2d(const Context& context, Allocator& allocator, ImageAllocation& allocation, const void *data)
  {
    // For image, we must use a staging buffer always
    auto staging_allocation = allocate_buffer(context, allocator, allocation.width * allocation.height * 4, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    {
      write_buffer(context, allocator, staging_allocation, data);

      auto command_buffer = create_command_buffer(context, false);
      command_buffer_begin(command_buffer);

      // Barrier
      {
        VkImageMemoryBarrier barrier = {};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        barrier.srcAccessMask                   = 0;
        barrier.dstAccessMask                   = VK_ACCESS_MEMORY_WRITE_BIT;
        barrier.oldLayout                       = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout                       = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.image                           = allocation.image;
        barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel   = 0;
        barrier.subresourceRange.levelCount     = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount     = 1;
        vkCmdPipelineBarrier(command_buffer.handle, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
      }

      // Copy
      {
        VkBufferImageCopy buffer_image_copy = {};
        buffer_image_copy.bufferOffset                    = 0;
        buffer_image_copy.bufferRowLength                 = 0;
        buffer_image_copy.bufferImageHeight               = 0;
        buffer_image_copy.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        buffer_image_copy.imageSubresource.mipLevel       = 0;
        buffer_image_copy.imageSubresource.baseArrayLayer = 0;
        buffer_image_copy.imageSubresource.layerCount     = 1;
        buffer_image_copy.imageOffset                     = {0, 0, 0};
        buffer_image_copy.imageExtent                     = {allocation.width, allocation.height, 1};
        vkCmdCopyBufferToImage(command_buffer.handle, staging_allocation.buffer, allocation.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &buffer_image_copy);
      }

      // Barrier
      {
        VkImageMemoryBarrier barrier = {};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        barrier.srcAccessMask                   = VK_ACCESS_MEMORY_WRITE_BIT;
        barrier.dstAccessMask                   = VK_ACCESS_MEMORY_READ_BIT;
        barrier.oldLayout                       = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout                       = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.image                           = allocation.image;
        barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel   = 0;
        barrier.subresourceRange.levelCount     = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount     = 1;
        vkCmdPipelineBarrier(command_buffer.handle, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
      }

      command_buffer_end(command_buffer);
      command_buffer_submit(context, command_buffer);
      command_buffer_wait(context, command_buffer);

      destroy_command_buffer(context, command_buffer);
    }
    deallocate_buffer(context, allocator, staging_allocation);
  }
}