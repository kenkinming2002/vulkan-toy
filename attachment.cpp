#include "attachment.hpp"

#include "buffer.hpp"
#include "vk_check.hpp"
#include <vulkan/vulkan_core.h>

namespace vulkan
{
  void init_attachment(const Context& context, Allocator& allocator, AttachmentCreateInfo create_info, Attachment& attachment)
  {
    attachment = {};
    switch(create_info.source)
    {
    case AttachmentSource::MANAGED:
      {
        Image2dCreateInfo image_create_info = {};
        switch(create_info.managed.type)
        {
        case AttachmentType::COLOR:
          image_create_info.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
          break;
        case AttachmentType::DEPTH:
        case AttachmentType::STENCIL:
          image_create_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
          break;
        }
        image_create_info.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        image_create_info.format     = create_info.managed.format;
        image_create_info.width      = create_info.managed.extent.width;
        image_create_info.height     = create_info.managed.extent.height;
        attachment.image = create_image2d(context, allocator, image_create_info, attachment.memory_allocation);

        VkImageViewCreateInfo image_view_create_info = {};
        image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        image_view_create_info.image                           = attachment.image;
        image_view_create_info.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
        image_view_create_info.format                          = create_info.managed.format;
        image_view_create_info.components.r                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        image_view_create_info.components.g                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        image_view_create_info.components.b                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        image_view_create_info.components.a                    = VK_COMPONENT_SWIZZLE_IDENTITY;

        switch(create_info.managed.type)
        {
          case AttachmentType::COLOR:   image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; break;
          case AttachmentType::DEPTH:   image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT; break;
          case AttachmentType::STENCIL: image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_STENCIL_BIT; break;
        }

        image_view_create_info.subresourceRange.baseMipLevel   = 0;
        image_view_create_info.subresourceRange.levelCount     = 1;
        image_view_create_info.subresourceRange.baseArrayLayer = 0;
        image_view_create_info.subresourceRange.layerCount     = 1;
        VK_CHECK(vkCreateImageView(context.device, &image_view_create_info, nullptr, &attachment.image_view));
      }
      break;
    case AttachmentSource::SWAPCHAIN:
      {
        uint32_t image_count = create_info.swapchain.swapchain.image_count;
        VkImage *images = new VkImage[image_count];
        VK_CHECK(vkGetSwapchainImagesKHR(context.device, create_info.swapchain.swapchain.handle, &image_count, images));
        attachment.image = images[create_info.swapchain.index];

        VkImageViewCreateInfo image_view_create_info = {};
        image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        image_view_create_info.image                           = attachment.image;
        image_view_create_info.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
        image_view_create_info.format                          = create_info.swapchain.swapchain.surface_format.format;
        image_view_create_info.components.r                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        image_view_create_info.components.g                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        image_view_create_info.components.b                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        image_view_create_info.components.a                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        image_view_create_info.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        image_view_create_info.subresourceRange.baseMipLevel   = 0;
        image_view_create_info.subresourceRange.levelCount     = 1;
        image_view_create_info.subresourceRange.baseArrayLayer = 0;
        image_view_create_info.subresourceRange.layerCount     = 1;
        VK_CHECK(vkCreateImageView(context.device, &image_view_create_info, nullptr, &attachment.image_view));
      }
      break;
    }
  }

  void deinit_attachment(const Context& context, Allocator& allocator, Attachment& attachment)
  {
    vkDestroyImageView(context.device, attachment.image_view, nullptr);
    switch(attachment.source)
    {
    case AttachmentSource::MANAGED:
      vkDestroyImage(context.device, attachment.image, nullptr);
      deallocate_memory(context, allocator, attachment.memory_allocation);
      break;
    case AttachmentSource::SWAPCHAIN:
      break;
    }
    attachment = {};
  }
}
