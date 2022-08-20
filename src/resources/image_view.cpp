#include "image_view.hpp"

#include "vk_check.hpp"

namespace vulkan
{
  struct ImageView
  {
    Ref ref;

    const Context *context;

    VkImageView handle;
  };

  static void image_view_free(ref_t ref)
  {
    image_view_t image_view = container_of(ref, ImageView, ref);
    vkDestroyImageView(image_view->context->device, image_view->handle, nullptr);
    delete image_view;
  }

  image_view_t image_view_create(const Context *context, ImageViewType type, VkFormat format, size_t mip_levels, image_t image)
  {
    image_view_t image_view = new ImageView {};
    image_view->ref.count = 1;
    image_view->ref.free  = &image_view_free;

    image_view->context = context;

    VkImageViewCreateInfo image_view_create_info = {};
    image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_create_info.image                           = image_get_handle(image);
    image_view_create_info.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
    image_view_create_info.format                          = format;
    image_view_create_info.components.r                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.g                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.b                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.a                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    switch(type)
    {
    case ImageViewType::COLOR:   image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; break;
    case ImageViewType::DEPTH:   image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT; break;
    case ImageViewType::STENCIL: image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_STENCIL_BIT; break;
    }
    image_view_create_info.subresourceRange.baseMipLevel   = 0;
    image_view_create_info.subresourceRange.levelCount     = mip_levels;
    image_view_create_info.subresourceRange.baseArrayLayer = 0;
    image_view_create_info.subresourceRange.layerCount     = 1;
    VK_CHECK(vkCreateImageView(image_view->context->device, &image_view_create_info, nullptr, &image_view->handle));

    return image_view;
  }

  image_view_t image_view_create(const Context *context, ImageViewType type, VkFormat format, VkImage image)
  {
    image_view_t image_view = new ImageView {};
    image_view->ref.count = 1;
    image_view->ref.free  = &image_view_free;

    image_view->context = context;

    VkImageViewCreateInfo image_view_create_info = {};
    image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_create_info.image                           = image;
    image_view_create_info.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
    image_view_create_info.format                          = format;
    image_view_create_info.components.r                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.g                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.b                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.a                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    switch(type)
    {
    case ImageViewType::COLOR:   image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; break;
    case ImageViewType::DEPTH:   image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT; break;
    case ImageViewType::STENCIL: image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_STENCIL_BIT; break;
    }
    image_view_create_info.subresourceRange.baseMipLevel   = 0;
    image_view_create_info.subresourceRange.levelCount     = 1;
    image_view_create_info.subresourceRange.baseArrayLayer = 0;
    image_view_create_info.subresourceRange.layerCount     = 1;
    VK_CHECK(vkCreateImageView(image_view->context->device, &image_view_create_info, nullptr, &image_view->handle));

    return image_view;
  }

  ref_t image_view_as_ref(image_view_t image_view)
  {
    return &image_view->ref;
  }

  VkImageView image_view_get_handle(image_view_t image_view)
  {
    return image_view->handle;
  }
}
