#include "buffer.hpp"
#include "command_buffer.hpp"
#include "fence.hpp"
#include "vk_check.hpp"

#include <assert.h>
#include <vulkan/vulkan_core.h>

namespace vulkan
{
  VkBuffer create_buffer(const Context& context, Allocator& allocator, BufferCreateInfo info, MemoryAllocation& allocation)
  {
    VkBuffer buffer = VK_NULL_HANDLE;

    VkBufferCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    create_info.size        = info.size;
    create_info.usage       = info.usage;
    create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VK_CHECK(vkCreateBuffer(context.device, &create_info, nullptr, &buffer));

    VkMemoryRequirements memory_requirements = {};
    vkGetBufferMemoryRequirements(context.device, buffer, &memory_requirements);

    MemoryAllocationInfo allocation_info = {};
    allocation_info.type_bits = memory_requirements.memoryTypeBits;
    allocation_info.properties = info.properties;
    allocation_info.size       = memory_requirements.size;
    allocate_memory(context, allocator, allocation_info, allocation);
    VK_CHECK(vkBindBufferMemory(context.device, buffer, allocation.memory, 0));
    return buffer;
  }

  VkImage create_image2d(const Context& context, Allocator& allocator, Image2dCreateInfo info, MemoryAllocation& allocation)
  {
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
    VK_CHECK(vkCreateImage(context.device, &create_info, nullptr, &image));

    VkMemoryRequirements memory_requirements = {};
    vkGetImageMemoryRequirements(context.device, image, &memory_requirements);

    MemoryAllocationInfo allocation_info = {};
    allocation_info.type_bits  = memory_requirements.memoryTypeBits;
    allocation_info.properties = info.properties;
    allocation_info.size       = memory_requirements.size;
    allocate_memory(context, allocator, allocation_info, allocation);
    VK_CHECK(vkBindImageMemory(context.device, image, allocation.memory, 0));
    return image;
  }

  void write_buffer(const Context& context, Allocator& allocator, VkBuffer buffer, MemoryAllocation allocation, const void *data)
  {
    VkMemoryPropertyFlags properties = allocator.memory_properties.memoryTypes[allocation.type_index].propertyFlags;
    if(properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
    {
      void *buffer_data;
      VK_CHECK(vkMapMemory(context.device, allocation.memory, 0, allocation.size, 0, &buffer_data));
      memcpy(buffer_data, data, allocation.size);
      vkUnmapMemory(context.device, allocation.memory);
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
      CommandBuffer command_buffer = {};
      Fence fence = {};

      init_command_buffer(context, command_buffer);
      init_fence(context, fence, false);

      command_buffer_begin(command_buffer);

      VkBufferCopy buffer_copy = {};
      buffer_copy.srcOffset = 0;
      buffer_copy.dstOffset = 0;
      buffer_copy.size      = allocation.size;
      vkCmdCopyBuffer(command_buffer.handle, staging_buffer, buffer, 1, &buffer_copy);

      command_buffer_end(command_buffer);

      command_buffer_submit(context, command_buffer, fence);
      fence_wait_and_reset(context, fence);

      deinit_fence(context, fence);
      deinit_command_buffer(context, command_buffer);
    }
    vkDestroyBuffer(context.device, staging_buffer, nullptr);
    deallocate_memory(context, allocator, staging_allocation);
  }

  void write_image2d(const Context& context, Allocator& allocator, VkImage image, size_t width, size_t height, MemoryAllocation allocation, const void *data)
  {
    (void)allocation;

    BufferCreateInfo info = {};
    info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    info.properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    info.size       = width * height * 4; // Magic

    MemoryAllocation staging_allocation = {};
    VkBuffer staging_buffer = create_buffer(context, allocator, info, staging_allocation);
    write_buffer(context, allocator, staging_buffer, staging_allocation, data);
    {
      CommandBuffer command_buffer = {};
      Fence fence = {};

      init_command_buffer(context, command_buffer);
      init_fence(context, fence, false);

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
        barrier.image                           = image;
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
        buffer_image_copy.imageExtent                     = { static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1};
        vkCmdCopyBufferToImage(command_buffer.handle, staging_buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &buffer_image_copy);
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
        vkCmdPipelineBarrier(command_buffer.handle, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
      }

      command_buffer_end(command_buffer);

      command_buffer_submit(context, command_buffer, fence);
      fence_wait_and_reset(context, fence);

      deinit_fence(context, fence);
      deinit_command_buffer(context, command_buffer);
    }
    vkDestroyBuffer(context.device, staging_buffer, nullptr);
    deallocate_memory(context, allocator, staging_allocation);
  }
}
