#include "image_view.hpp"

#include "vk_check.hpp"

namespace vulkan
{
  struct ImageView
  {
    Ref ref;

    context_t context;

    VkFormat format;
    size_t width, height;
    size_t mip_levels;

    VkImageView handle;
  };
  REF_DEFINE(ImageView, image_view_t, ref);

  static void image_view_free(ref_t ref)
  {
    image_view_t image_view = container_of(ref, ImageView, ref);

    VkDevice device = context_get_device_handle(image_view->context);
    vkDestroyImageView(device, image_view->handle, nullptr);

    put(image_view->context);

    delete image_view;
  }

  image_view_t image_view_create(context_t context, ImageViewType type, image_t image)
  {
    image_view_t image_view = new ImageView {};
    image_view->ref.count = 1;
    image_view->ref.free  = &image_view_free;

    get(context);
    image_view->context = context;

    image_view->format     = image->format;
    image_view->width      = image->width;
    image_view->height     = image->height;
    image_view->mip_levels = image->mip_levels;

    VkDevice device = context_get_device_handle(image_view->context);

    VkImageViewCreateInfo image_view_create_info = {};
    image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_create_info.image                           = image_get_handle(image);
    image_view_create_info.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
    image_view_create_info.format                          = image_view->format;
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
    image_view_create_info.subresourceRange.levelCount     = image_view->mip_levels;
    image_view_create_info.subresourceRange.baseArrayLayer = 0;
    image_view_create_info.subresourceRange.layerCount     = 1;
    VK_CHECK(vkCreateImageView(device, &image_view_create_info, nullptr, &image_view->handle));

    return image_view;
  }

  image_view_t image_view_create(context_t context, ImageViewType type, VkFormat format, VkImage image)
  {
    image_view_t image_view = new ImageView {};
    image_view->ref.count = 1;
    image_view->ref.free  = &image_view_free;

    get(context);
    image_view->context = context;

    VkDevice device = context_get_device_handle(image_view->context);

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
    VK_CHECK(vkCreateImageView(device, &image_view_create_info, nullptr, &image_view->handle));

    return image_view;
  }

  VkImageView image_view_get_handle(image_view_t image_view)
  {
    return image_view->handle;
  }
}
