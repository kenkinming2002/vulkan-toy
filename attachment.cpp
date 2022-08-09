#include "attachment.hpp"

#include "buffer.hpp"
#include "vk_check.hpp"

namespace vulkan
{
  AttachmentInfo swapchain_get_attachment_info(const Swapchain& swapchain)
  {
    AttachmentInfo info = {};
    info.type        = AttachmentType::COLOR;
    info.image_count = swapchain.image_count;
    info.extent      = swapchain.extent;
    info.format      = swapchain.surface_format.format;
    return info;
  }

  void init_attachment(const Context& context, allocator_t allocator, AttachmentCreateInfo create_info, Attachment& attachment)
  {
    attachment.source = create_info.source;

    // 1: Retrive info and images
    switch(attachment.source)
    {
    case AttachmentSource::SWAPCHAIN:
      {
        attachment.info      = swapchain_get_attachment_info(create_info.swapchain);
        attachment.images    = new VkImage[attachment.info.image_count];
        vkGetSwapchainImagesKHR(context.device, create_info.swapchain.handle, &attachment.info.image_count, attachment.images);
      }
      break;
    case AttachmentSource::MANAGED:
      {
        attachment.info               = create_info.attachment_info;
        attachment.images             = new VkImage[attachment.info.image_count];
        attachment.memory_allocations = new MemoryAllocation[attachment.info.image_count];

        for(uint32_t i=0; i<attachment.info.image_count; ++i)
        {
          Image2dCreateInfo create_info = {};
          switch(attachment.info.type)
          {
          case AttachmentType::COLOR:
            create_info.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
            break;
          case AttachmentType::DEPTH:
          case AttachmentType::STENCIL:
            create_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
            break;
          }
          create_info.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
          create_info.format     = attachment.info.format;
          create_info.width      = attachment.info.extent.width;
          create_info.height     = attachment.info.extent.height;
          attachment.images[i] = create_image2d(context, allocator, create_info, attachment.memory_allocations[i]);
        }
      }
      break;
    }

    // 2: Create image_views
    attachment.image_views = new VkImageView[attachment.info.image_count];
    for(uint32_t i=0; i<attachment.info.image_count; ++i)
    {
      VkImageViewCreateInfo create_info = {};
      create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
      create_info.image                           = attachment.images[i];
      create_info.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
      create_info.format                          = attachment.info.format;
      create_info.components.r                    = VK_COMPONENT_SWIZZLE_IDENTITY;
      create_info.components.g                    = VK_COMPONENT_SWIZZLE_IDENTITY;
      create_info.components.b                    = VK_COMPONENT_SWIZZLE_IDENTITY;
      create_info.components.a                    = VK_COMPONENT_SWIZZLE_IDENTITY;
      switch(attachment.info.type)
      {
      case AttachmentType::COLOR:
        create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        break;
      case AttachmentType::DEPTH:
        create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        break;
      case AttachmentType::STENCIL:
        create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_STENCIL_BIT;
        break;
      }
      create_info.subresourceRange.baseMipLevel   = 0;
      create_info.subresourceRange.levelCount     = 1;
      create_info.subresourceRange.baseArrayLayer = 0;
      create_info.subresourceRange.layerCount     = 1;

      VK_CHECK(vkCreateImageView(context.device, &create_info, nullptr, &attachment.image_views[i]));
    }
  }

  void deinit_attachment(const Context& context, allocator_t allocator, Attachment& attachment)
  {
    for(uint32_t i=0; i<attachment.info.image_count; ++i)
    {
      vkDestroyImageView(context.device, attachment.image_views[i], nullptr);
      attachment.image_views[i] = VK_NULL_HANDLE;
    }

    delete[] attachment.image_views;

    switch(attachment.source)
    {
    case AttachmentSource::SWAPCHAIN:
      delete[] attachment.images;
      break;
    case AttachmentSource::MANAGED:
      for(uint32_t i=0; i<attachment.info.image_count; ++i)
      {
        vkDestroyImage(context.device, attachment.images[i], nullptr);
        attachment.images[i] = VK_NULL_HANDLE;
        deallocate_memory(context, allocator, attachment.memory_allocations[i]);
      }

      delete[] attachment.images;
      delete[] attachment.memory_allocations;
      break;
    }

    attachment = {};
  }
}
