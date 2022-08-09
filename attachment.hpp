#pragma once

#include "context.hpp"
#include "buffer.hpp"
#include "swapchain.hpp"
#include "utils.hpp"

namespace vulkan
{
  enum class AttachmentType   { COLOR, DEPTH, STENCIL };
  struct AttachmentInfo
  {
    AttachmentType type;
    uint32_t       image_count;
    VkExtent2D     extent;
    VkFormat       format;
  };
  AttachmentInfo swapchain_get_attachment_info(const Swapchain& swapchain);

  enum class AttachmentSource { SWAPCHAIN, MANAGED };
  struct AttachmentCreateInfo
  {
    AttachmentSource source;
    union
    {
      Swapchain swapchain;
      AttachmentInfo attachment_info;
    };
  };

  struct Attachment
  {
    AttachmentSource source;
    MemoryAllocation *memory_allocations;

    AttachmentInfo info;

    VkImage     *images;
    VkImageView *image_views;
  };

  void init_attachment(const Context& context, allocator_t allocator, AttachmentCreateInfo create_info, Attachment& attachment);
  void deinit_attachment(const Context& context, allocator_t allocator, Attachment& attachment);
}
