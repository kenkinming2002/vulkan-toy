#include "buffer.hpp"
#include "command_buffer.hpp"

#include <assert.h>
#include <vulkan/vulkan_core.h>

namespace vulkan
{
  struct Allocator
  {
    VkPhysicalDeviceMemoryProperties memory_properties;
  };

  allocator_t create_allocator(context_t context)
  {
    VkPhysicalDevice physical_device = context_get_physical_device(context);

    allocator_t allocator = new Allocator{};
    vkGetPhysicalDeviceMemoryProperties(physical_device, &allocator->memory_properties);
    return allocator;
  }

  void destroy_allocator(context_t context, allocator_t allocator)
  {
    (void)context;
    delete allocator;
  }

  template<typename T>
  static inline bool is_bits_set(T value, T flags)
  {
    return (value & flags) == flags;
  }

  MemoryAllocation allocate_memory(context_t context, allocator_t allocator, MemoryAllocationInfo info)
  {
    VkDevice device = context_get_device(context);
    for(uint32_t i = 0; i < allocator->memory_properties.memoryTypeCount; i++)
    {
      if(!is_bits_set<uint32_t>(info.type_bits, 1 << i))
        continue;

      VkMemoryType memory_type = allocator->memory_properties.memoryTypes[i];
      if(!is_bits_set(memory_type.propertyFlags, info.properties))
        continue;

      MemoryAllocation allocation = {};
      allocation.size = info.size;

      VkMemoryAllocateInfo allocate_info = {};
      allocate_info.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
      allocate_info.memoryTypeIndex = i;
      allocate_info.allocationSize  = info.size;
      VK_CHECK(vkAllocateMemory(device, &allocate_info, nullptr, &allocation.memory));

      allocation.type_index = i;
      allocation.size       = info.size;
      return allocation;
    }

    fprintf(stderr, "No memory type while allocating device memory");
    abort();
  }

  void deallocate_memory(context_t context, allocator_t allocator, MemoryAllocation allocation)
  {
    VkDevice device = context_get_device(context);

    (void)allocator;
    vkFreeMemory(device, allocation.memory, nullptr);
  }

  VkBuffer create_buffer(context_t context, allocator_t allocator, BufferCreateInfo info, MemoryAllocation& allocation)
  {
    VkDevice device = context_get_device(context);

    VkBuffer buffer = VK_NULL_HANDLE;

    VkBufferCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    create_info.size        = info.size;
    create_info.usage       = info.usage;
    create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VK_CHECK(vkCreateBuffer(device, &create_info, nullptr, &buffer));

    VkMemoryRequirements memory_requirements = {};
    vkGetBufferMemoryRequirements(device, buffer, &memory_requirements);

    MemoryAllocationInfo allocation_info = {};
    allocation_info.type_bits = memory_requirements.memoryTypeBits;
    allocation_info.properties = info.properties;
    allocation_info.size       = memory_requirements.size;
    allocation = allocate_memory(context, allocator, allocation_info);
    VK_CHECK(vkBindBufferMemory(device, buffer, allocation.memory, 0));
    return buffer;
  }

  VkImage create_image2d(context_t context, allocator_t allocator, Image2dCreateInfo info, MemoryAllocation& allocation)
  {
    VkDevice device = context_get_device(context);

    VkImage image = VK_NULL_HANDLE;

    VkImageCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    create_info.flags         = 0;
    create_info.imageType     = VK_IMAGE_TYPE_2D;
    create_info.format        = info.format;
    create_info.extent.width  = info.width;
    create_info.extent.height = info.height;
    create_info.extent.depth  = 1;
    create_info.mipLevels     = 1;
    create_info.arrayLayers   = 1;
    create_info.samples       = VK_SAMPLE_COUNT_1_BIT;
    create_info.tiling        = VK_IMAGE_TILING_OPTIMAL;
    create_info.usage         = info.usage;
    create_info.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
    create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VK_CHECK(vkCreateImage(device, &create_info, nullptr, &image));

    VkMemoryRequirements memory_requirements = {};
    vkGetImageMemoryRequirements(device, image, &memory_requirements);

    MemoryAllocationInfo allocation_info = {};
    allocation_info.type_bits  = memory_requirements.memoryTypeBits;
    allocation_info.properties = info.properties;
    allocation_info.size       = memory_requirements.size;
    allocation = allocate_memory(context, allocator, allocation_info);
    VK_CHECK(vkBindImageMemory(device, image, allocation.memory, 0));
    return image;
  }

  void write_buffer(context_t context, allocator_t allocator, VkBuffer buffer, MemoryAllocation allocation, const void *data)
  {
    VkDevice device = context_get_device(context);

    VkMemoryPropertyFlags properties = allocator->memory_properties.memoryTypes[allocation.type_index].propertyFlags;
    if(properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
    {
      void *buffer_data;
      VK_CHECK(vkMapMemory(device, allocation.memory, 0, allocation.size, 0, &buffer_data));
      memcpy(buffer_data, data, allocation.size);
      vkUnmapMemory(device, allocation.memory);
      return;
    }

    // Staging buffer
    BufferCreateInfo info = {};
    info.usage      = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    info.properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    info.size       = allocation.size;

    MemoryAllocation staging_allocation;
    VkBuffer staging_buffer = create_buffer(context, allocator, info, staging_allocation);
    {
      command_buffer_t command_buffer = create_command_buffer(context, false);
      VkCommandBuffer command_buffer_handle = command_buffer_get_handle(command_buffer);

      vulkan::command_buffer_begin(command_buffer);

      VkBufferCopy buffer_copy = {};
      buffer_copy.srcOffset = 0;
      buffer_copy.dstOffset = 0;
      buffer_copy.size      = allocation.size;
      vkCmdCopyBuffer(command_buffer_handle, staging_buffer, buffer, 1, &buffer_copy);

      vulkan::command_buffer_end(command_buffer);
      vulkan::command_buffer_submit(context, command_buffer);
      vulkan::command_buffer_wait(context, command_buffer);

      destroy_command_buffer(context, command_buffer);
    }
    vkDestroyBuffer(device, staging_buffer, nullptr);
    deallocate_memory(context, allocator, staging_allocation);
  }

  void write_image2d(context_t context, allocator_t allocator, VkImage image, size_t width, size_t height, MemoryAllocation allocation, const void *data)
  {
    VkDevice device = context_get_device(context);

    (void)allocation;

    BufferCreateInfo info = {};
    info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    info.properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    info.size       = width * height * 4; // Magic

    MemoryAllocation staging_allocation = {};
    VkBuffer staging_buffer = create_buffer(context, allocator, info, staging_allocation);
    write_buffer(context, allocator, staging_buffer, staging_allocation, data);
    {
      command_buffer_t command_buffer = create_command_buffer(context, false);
      VkCommandBuffer command_buffer_handle = command_buffer_get_handle(command_buffer);

      vulkan::command_buffer_begin(command_buffer);

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
        barrier.image                           = image;
        barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel   = 0;
        barrier.subresourceRange.levelCount     = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount     = 1;
        vkCmdPipelineBarrier(command_buffer_handle, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
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
        buffer_image_copy.imageExtent                     = { static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1};
        vkCmdCopyBufferToImage(command_buffer_handle, staging_buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &buffer_image_copy);
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
        barrier.image                           = image;
        barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel   = 0;
        barrier.subresourceRange.levelCount     = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount     = 1;
        vkCmdPipelineBarrier(command_buffer_handle, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
      }

      vulkan::command_buffer_end(command_buffer);
      vulkan::command_buffer_submit(context, command_buffer);
      vulkan::command_buffer_wait(context, command_buffer);

      destroy_command_buffer(context, command_buffer);
    }
    vkDestroyBuffer(device, staging_buffer, nullptr);
    deallocate_memory(context, allocator, staging_allocation);
  }
}
