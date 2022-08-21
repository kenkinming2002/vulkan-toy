#include "image.hpp"

#include "allocator.hpp"
#include "buffer.hpp"
#include "vk_check.hpp"

#include <algorithm>

#include <assert.h>

namespace vulkan
{
  static VkImageUsageFlags get_vulkan_image_usage(ImageType type)
  {
    switch(type)
    {
    case ImageType::TEXTURE:            return VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    case ImageType::COLOR_ATTACHMENT:   return VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    case ImageType::DEPTH_ATTACHMENT:   return VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    case ImageType::STENCIL_ATTACHMENT: return VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    default: assert(false && "Unreachable");
    }
  }

  struct Image
  {
    Ref ref;

    context_t context;
    Allocator     *allocator;

    size_t width, height;
    size_t mip_levels;

    VkImage          handle;
    MemoryAllocation allocation;
  };

  static void image_free(ref_t ref)
  {
    image_t image = container_of(ref, Image, ref);

    VkDevice device = context_get_device_handle(image->context);

    vkDestroyImage(device, image->handle, nullptr);
    deallocate_memory(image->context, *image->allocator, image->allocation);

    context_put(image->context);

    delete image;
  }

  image_t image_create(context_t context, Allocator *allocator, ImageType type, VkFormat format, size_t width, size_t height, size_t mip_levels)
  {
    image_t image = new Image {};
    image->ref.count = 1;
    image->ref.free  = &image_free;

    context_get(context);
    image->context   = context;
    image->allocator = allocator;

    VkDevice device = context_get_device_handle(image->context);

    image->width      = width;
    image->height     = height;
    image->mip_levels = mip_levels;

    VkImageCreateInfo image_create_info = {};
    image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.flags         = 0;
    image_create_info.imageType     = VK_IMAGE_TYPE_2D;
    image_create_info.format        = format;
    image_create_info.extent.width  = image->width;
    image_create_info.extent.height = image->height;
    image_create_info.extent.depth  = 1;
    image_create_info.mipLevels     = image->mip_levels;
    image_create_info.arrayLayers   = 1;
    image_create_info.samples       = VK_SAMPLE_COUNT_1_BIT;
    image_create_info.tiling        = VK_IMAGE_TILING_OPTIMAL;
    image_create_info.usage         = get_vulkan_image_usage(type);
    image_create_info.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
    image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VK_CHECK(vkCreateImage(device, &image_create_info, nullptr, &image->handle));

    VkMemoryRequirements memory_requirements = {};
    vkGetImageMemoryRequirements(device, image->handle, &memory_requirements);

    MemoryAllocationInfo allocation_info = {};
    allocation_info.type_bits  = memory_requirements.memoryTypeBits;
    allocation_info.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    allocation_info.size       = memory_requirements.size;
    allocate_memory(image->context, *image->allocator, allocation_info, image->allocation);
    VK_CHECK(vkBindImageMemory(device, image->handle, image->allocation.memory, 0));

    return image;
  }

  ref_t image_as_ref(image_t image) { return &image->ref; }

  VkImage image_get_handle(image_t image)
  {
    return image->handle;
  }

  void image_write(command_buffer_t command_buffer, image_t image, const void *data, size_t width, size_t height, size_t size)
  {
    // Bytes per pixel should be integer
    assert(size / (width * height) * (width * height) == size);

    buffer_t staging_buffer = buffer_create(image->context, image->allocator, BufferType::STAGING_BUFFER, size);
    buffer_write(command_buffer, staging_buffer, data, size);

    VkCommandBuffer handle = command_buffer_get_handle(command_buffer);
    command_buffer_use(command_buffer, buffer_as_ref(staging_buffer));
    command_buffer_use(command_buffer, image_as_ref(image));

    // TODO: What if we want to copy non color image
    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    barrier.image                           = image->handle;
    barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount     = 1;

    // Barrier
    barrier.subresourceRange.baseMipLevel   = 0;
    barrier.subresourceRange.levelCount     = image->mip_levels;
    barrier.srcAccessMask                   = 0;
    barrier.dstAccessMask                   = VK_ACCESS_MEMORY_WRITE_BIT;
    barrier.oldLayout                       = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout                       = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    vkCmdPipelineBarrier(handle, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    // Fill mip level 0
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
    vkCmdCopyBufferToImage(handle, buffer_get_handle(staging_buffer), image_get_handle(image), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &buffer_image_copy);

    VkImageBlit image_blit = {};
    image_blit.srcOffsets[0] = { 0, 0, 0 };
    image_blit.dstOffsets[0] = { 0, 0, 0 };
    image_blit.srcSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    image_blit.srcSubresource.baseArrayLayer = 0;
    image_blit.srcSubresource.layerCount     = 1;
    image_blit.dstSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    image_blit.dstSubresource.baseArrayLayer = 0;
    image_blit.dstSubresource.layerCount     = 1;

    int32_t prev_mip_width  = image->width;
    int32_t prev_mip_height = image->height;

    for(size_t mip_level=1; mip_level<image->mip_levels; ++mip_level)
    {
      int32_t mip_width  = prev_mip_width  > 1 ? prev_mip_width  >>= 1 : 1;
      int32_t mip_height = prev_mip_height > 1 ? prev_mip_height >>= 1 : 1;

      barrier.subresourceRange.baseMipLevel = mip_level - 1;
      barrier.subresourceRange.levelCount   = 1;
      barrier.srcAccessMask                 = VK_ACCESS_MEMORY_WRITE_BIT;
      barrier.dstAccessMask                 = VK_ACCESS_MEMORY_READ_BIT;
      barrier.oldLayout                     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
      barrier.newLayout                     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
      vkCmdPipelineBarrier(handle, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

      image_blit.srcOffsets[1]                 = { prev_mip_width, prev_mip_height, 1 };
      image_blit.srcSubresource.mipLevel       = mip_level - 1;
      image_blit.dstOffsets[1]                 = { mip_width, mip_height, 1 };
      image_blit.dstSubresource.mipLevel       = mip_level;
      vkCmdBlitImage(handle,
          image_get_handle(image), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
          image_get_handle(image), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
          1, &image_blit,
          VK_FILTER_LINEAR);

      prev_mip_width  = mip_width;
      prev_mip_height = mip_height;
    }

    barrier.subresourceRange.baseMipLevel = image->mip_levels - 1;
    barrier.subresourceRange.levelCount   = 1;
    barrier.srcAccessMask                 = VK_ACCESS_MEMORY_WRITE_BIT;
    barrier.dstAccessMask                 = VK_ACCESS_MEMORY_READ_BIT;
    barrier.oldLayout                     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout                     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    vkCmdPipelineBarrier(handle, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    // Barrier
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount   = image->mip_levels;
    barrier.srcAccessMask                 = VK_ACCESS_MEMORY_WRITE_BIT;
    barrier.dstAccessMask                 = VK_ACCESS_MEMORY_READ_BIT;
    barrier.oldLayout                     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.newLayout                     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    vkCmdPipelineBarrier(handle, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    buffer_put(staging_buffer);
  }
}
