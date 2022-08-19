#include "image_view.hpp"

#include "vk_check.hpp"

namespace vulkan
{
  void init_image_view(const Context& context, ImageViewCreateInfo create_info, ImageView& image_view)
  {
    VkImageViewCreateInfo image_view_create_info = {};
    image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_create_info.image                           = image_get_handle(create_info.image);
    image_view_create_info.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
    image_view_create_info.format                          = create_info.format;
    image_view_create_info.components.r                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.g                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.b                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.a                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    switch(create_info.type)
    {
    case ImageViewType::COLOR:   image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; break;
    case ImageViewType::DEPTH:   image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT; break;
    case ImageViewType::STENCIL: image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_STENCIL_BIT; break;
    }
    image_view_create_info.subresourceRange.baseMipLevel   = 0;
    image_view_create_info.subresourceRange.levelCount     = 1;
    image_view_create_info.subresourceRange.baseArrayLayer = 0;
    image_view_create_info.subresourceRange.layerCount     = 1;
    VK_CHECK(vkCreateImageView(context.device, &image_view_create_info, nullptr, &image_view.handle));
  }

  void deinit_image_view(const Context& context, ImageView& image_view)
  {
    vkDestroyImageView(context.device, image_view.handle, nullptr);
    image_view.handle = VK_NULL_HANDLE;
    image_view = {};
  }
}
