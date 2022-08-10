#pragma once

#include "context.hpp"
#include "buffer.hpp"
#include "swapchain.hpp"

namespace vulkan
{
  enum class AttachmentType   { COLOR, DEPTH, STENCIL };
  struct ManagedAttachmentCreateInfo
  {
    AttachmentType type;
    VkExtent2D     extent;
    VkFormat       format;
  };

  struct ManagedAttachment
  {
    VkImage          image;
    VkImageView      image_view;

    MemoryAllocation memory_allocation;
  };

  void init_attachment_managed(const Context& context, Allocator& allocator, ManagedAttachmentCreateInfo create_info, ManagedAttachment& attachment);
  void deinit_attachment_managed(const Context& context, Allocator& allocator, ManagedAttachment& attachment);

  struct SwapchainAttachmentCreateInfo
  {
    Swapchain swapchain;
    uint32_t index;
  };

  struct SwapchainAttachment
  {
    VkImage     image;
    VkImageView image_view;
  };

  void init_attachment_swapchain(const Context& context, SwapchainAttachmentCreateInfo create_info, SwapchainAttachment& attachment);
  void deinit_attachment_swapchain(const Context& context, SwapchainAttachment& attachment);

  struct Attachment
  {
    VkImage     image;
    VkImageView image_view;
  };

  inline Attachment to_attachment(const ManagedAttachment& attachment)
  {
    return Attachment{
      .image      = attachment.image,
      .image_view = attachment.image_view,
    };
  }

  inline Attachment to_attachment(const SwapchainAttachment& attachment)
  {
    return Attachment{
      .image      = attachment.image,
      .image_view = attachment.image_view,
    };
  }
}
