#include "attachment.hpp"

#include "buffer.hpp"
#include "vk_check.hpp"
#include <vulkan/vulkan_core.h>

namespace vulkan
{
  void init_attachment_managed(const Context& context, allocator_t allocator, ManagedAttachmentCreateInfo create_info, ManagedAttachment& attachment)
  {
    attachment = {};

    Image2dCreateInfo image_create_info = {};
    switch(create_info.type)
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
    image_create_info.format     = create_info.format;
    image_create_info.width      = create_info.extent.width;
    image_create_info.height     = create_info.extent.height;
    attachment.image = create_image2d(context, allocator, image_create_info, attachment.memory_allocation);

    VkImageViewCreateInfo image_view_create_info = {};
    image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_create_info.image                           = attachment.image;
    image_view_create_info.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
    image_view_create_info.format                          = create_info.format;
    image_view_create_info.components.r                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.g                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.b                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.a                    = VK_COMPONENT_SWIZZLE_IDENTITY;

    switch(create_info.type)
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

  void deinit_attachment_managed(const Context& context, allocator_t allocator, ManagedAttachment& attachment)
  {
    vkDestroyImageView(context.device, attachment.image_view, nullptr);
    vkDestroyImage(context.device, attachment.image, nullptr);
    deallocate_memory(context, allocator, attachment.memory_allocation);
    attachment = {};
  }

  void init_attachment_swapchain(const Context& context, SwapchainAttachmentCreateInfo create_info, SwapchainAttachment& attachment)
  {
    attachment = {};

    uint32_t image_count = create_info.swapchain.image_count;
    VkImage *images = new VkImage[image_count];
    VK_CHECK(vkGetSwapchainImagesKHR(context.device, create_info.swapchain.handle, &image_count, images));
    attachment.image = images[create_info.index];

    VkImageViewCreateInfo image_view_create_info = {};
    image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_create_info.image                           = attachment.image;
    image_view_create_info.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
    image_view_create_info.format                          = create_info.swapchain.surface_format.format;
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

  void deinit_attachment_swapchain(const Context& context, SwapchainAttachment& attachment)
  {
    vkDestroyImageView(context.device, attachment.image_view, nullptr);
    attachment = {};
  }
}
