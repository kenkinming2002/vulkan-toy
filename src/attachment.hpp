#pragma once

#include "context.hpp"
#include "buffer.hpp"
#include "swapchain.hpp"

namespace vulkan
{
  enum class AttachmentSource { SWAPCHAIN, MANAGED };

  enum class AttachmentType   { COLOR, DEPTH, STENCIL };
  struct ManagedAttachmentCreatInfo
  {
    AttachmentType type;
    VkExtent2D     extent;
    VkFormat       format;
  };

  struct SwapchainAttachmentCreateInfo
  {
    Swapchain swapchain;
    uint32_t index;
  };

  struct AttachmentCreateInfo
  {
    AttachmentSource source;
    union
    {
      ManagedAttachmentCreatInfo    managed;
      SwapchainAttachmentCreateInfo swapchain;
    };
  };

  struct Attachment
  {
    AttachmentSource source;
    VkImage          image;
    VkImageView      image_view;
    MemoryAllocation memory_allocation;
  };

  void init_attachment(const Context& context, Allocator& allocator, AttachmentCreateInfo create_info, Attachment& attachment);
  void deinit_attachment(const Context& context, Allocator& allocator, Attachment& attachment);
}
