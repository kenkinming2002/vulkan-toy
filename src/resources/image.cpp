#include "image.hpp"

#include "allocator.hpp"
#include "buffer.hpp"
#include "command_buffer.hpp"
#include "vk_check.hpp"

#include <assert.h>

namespace vulkan
{
  static VkImageUsageFlags get_vulkan_image_usage(ImageType type)
  {
    switch(type)
    {
    case ImageType::TEXTURE:            return VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    case ImageType::COLOR_ATTACHMENT:   return VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    case ImageType::DEPTH_ATTACHMENT:   return VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    case ImageType::STENCIL_ATTACHMENT: return VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    default: assert(false && "Unreachable");
    }
  }

  void init_image(const Context& context, Allocator& allocator, ImageCreateInfo create_info, Image& image)
  {
    image = {};

    VkImageCreateInfo image_create_info = {};
    image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.flags         = 0;
    image_create_info.imageType     = VK_IMAGE_TYPE_2D;
    image_create_info.format        = create_info.format;
    image_create_info.extent.width  = create_info.extent.width;
    image_create_info.extent.height = create_info.extent.height;
    image_create_info.extent.depth  = 1;
    image_create_info.mipLevels     = 1;
    image_create_info.arrayLayers   = 1;
    image_create_info.samples       = VK_SAMPLE_COUNT_1_BIT;
    image_create_info.tiling        = VK_IMAGE_TILING_OPTIMAL;
    image_create_info.usage         = get_vulkan_image_usage(create_info.type);
    image_create_info.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
    image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VK_CHECK(vkCreateImage(context.device, &image_create_info, nullptr, &image.handle));

    VkMemoryRequirements memory_requirements = {};
    vkGetImageMemoryRequirements(context.device, image.handle, &memory_requirements);

    MemoryAllocationInfo allocation_info = {};
    allocation_info.type_bits  = memory_requirements.memoryTypeBits;
    allocation_info.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    allocation_info.size       = memory_requirements.size;
    allocate_memory(context, allocator, allocation_info, image.allocation);
    VK_CHECK(vkBindImageMemory(context.device, image.handle, image.allocation.memory, 0));
  }

  void deinit_image(const Context& context, Allocator& allocator, Image& image)
  {
    vkDestroyImage(context.device, image.handle, nullptr);
    deallocate_memory(context, allocator, image.allocation);
  }

  void write_image(const Context& context, Allocator& allocator, Image image, const void *data, size_t width, size_t height, size_t size)
  {
    // Bytes per pixel should be integer
    assert(size / (width * height) * (width * height) == size);


    // TODO: What if we want to copy non color image
    CommandBuffer command_buffer = {};
    init_command_buffer(context, command_buffer);

    command_buffer_begin(command_buffer);

    // NOTE: This should not actually record any command in the command buffer
    // TODO: Make this fact explicit
    buffer_t staging_buffer = buffer_create(&context, &allocator, BufferType::STAGING_BUFFER, size);
    buffer_write(command_buffer.handle, staging_buffer, data, size);

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
      barrier.image                           = image.handle;
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
      vkCmdCopyBufferToImage(command_buffer.handle, buffer_get_handle(staging_buffer), image.handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &buffer_image_copy);
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
      barrier.image                           = image.handle;
      barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
      barrier.subresourceRange.baseMipLevel   = 0;
      barrier.subresourceRange.levelCount     = 1;
      barrier.subresourceRange.baseArrayLayer = 0;
      barrier.subresourceRange.layerCount     = 1;
      vkCmdPipelineBarrier(command_buffer.handle, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
    }

    command_buffer_end(command_buffer);

    Fence fence = {};
    init_fence(context, fence, false);
    command_buffer_submit(context, command_buffer, fence);
    fence_wait_and_reset(context, fence);
    deinit_fence(context, fence);

    deinit_command_buffer(context, command_buffer);

    buffer_put(staging_buffer);
  }
}
