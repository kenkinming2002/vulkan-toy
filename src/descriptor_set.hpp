#pragma once

#include "buffer.hpp"
#include "context.hpp"
#include "descriptor_set_layout.hpp"
#include "image_view.hpp"
#include "sampler.hpp"

namespace vulkan
{
  struct DescriptorPoolCreateInfo
  {
    const DescriptorBinding *descriptors;
    uint32_t              descriptor_count;

    uint32_t count;
  };

  struct DescriptorPool
  {
    VkDescriptorPool handle;
  };

  void init_descriptor_pool(const Context& context, DescriptorPoolCreateInfo create_info, DescriptorPool& descriptor_pool);
  void deinit_descriptor_pool(const Context& context, DescriptorPool& descriptor_pool);

  struct DescriptorSet
  {
    VkDescriptorSet handle;
  };

  void allocate_descriptor_set(const Context& context, DescriptorPool descriptor_pool, DescriptorSetLayout descriptor_set_layout, DescriptorSet& descriptor_set);

  struct UniformBufferDescriptor
  {
    Buffer buffer;
    VkDeviceSize size;
  };

  struct CombinedImageSmaplerDescriptor
  {
    ImageView image_view;
    Sampler sampler;
  };

  struct Descriptor
  {
    DescriptorType type;
    union
    {
      UniformBufferDescriptor        uniform_buffer;
      CombinedImageSmaplerDescriptor combined_image_sampler;
    };
  };

  struct DescriptorSetWriteInfo
  {
    const Descriptor *descriptors;
    uint32_t          descriptor_count;
  };

  void write_descriptor_set(const Context& context, DescriptorSet descriptor_set, DescriptorSetWriteInfo write_info);
}
