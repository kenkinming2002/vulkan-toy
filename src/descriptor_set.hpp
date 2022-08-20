#pragma once

#include "core/context.hpp"
#include "input.hpp"
#include "resources/buffer.hpp"
#include "resources/image_view.hpp"
#include "resources/sampler.hpp"

namespace vulkan
{
  struct DescriptorPoolCreateInfo
  {
    DescriptorInput descriptor_input;
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

  void allocate_descriptor_set(const Context& context, DescriptorPool descriptor_pool, VkDescriptorSetLayout descriptor_set_layout, DescriptorSet& descriptor_set);

  struct UniformBufferDescriptor
  {
    buffer_t buffer;
    VkDeviceSize size;
  };

  struct CombinedImageSmaplerDescriptor
  {
    image_view_t image_view;
    sampler_t    sampler;
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
