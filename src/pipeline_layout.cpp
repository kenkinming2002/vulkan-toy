#include "pipeline_layout.hpp"

#include "vk_check.hpp"
#include "utils.hpp"

namespace vulkan
{
  static VkDescriptorType to_vulkan_descriptor_type(DescriptorType type)
  {
    switch(type)
    {
    case DescriptorType::UNIFORM_BUFFER: return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    case DescriptorType::SAMPLER:        return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    default: assert(false && "Unreachable");
    }
  }

  static VkShaderStageFlags to_vulkan_stage_flags(ShaderStage stage)
  {
    switch(stage)
    {
    case ShaderStage::VERTEX:   return VK_SHADER_STAGE_VERTEX_BIT;
    case ShaderStage::FRAGMENT: return VK_SHADER_STAGE_FRAGMENT_BIT;
    default: assert(false && "Unreachable");
    }
  }

  void init_pipeline_layout(const Context& context, PipelineLayoutCreateInfo create_info, PipelineLayout& pipeline_layout)
  {
    dynarray<VkDescriptorSetLayoutBinding> layout_bindings = create_dynarray<VkDescriptorSetLayoutBinding>(create_info.descriptor_count);
    for(uint32_t i=0; i<create_info.descriptor_count; ++i)
    {
      VkDescriptorSetLayoutBinding layout_binding = {};
      layout_binding.binding            = i;
      layout_binding.descriptorType     = to_vulkan_descriptor_type(create_info.descriptors[i].type);
      layout_binding.descriptorCount    = 1;
      layout_binding.stageFlags         = to_vulkan_stage_flags(create_info.descriptors[i].stage);
      layout_binding.pImmutableSamplers = nullptr;
      layout_bindings[i] = layout_binding;
    }

    VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info = {};
    descriptor_set_layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptor_set_layout_create_info.bindingCount = size(layout_bindings);
    descriptor_set_layout_create_info.pBindings    = data(layout_bindings);
    VK_CHECK(vkCreateDescriptorSetLayout(context.device, &descriptor_set_layout_create_info, nullptr, &pipeline_layout.descriptor_set_layout));

    destroy_dynarray(layout_bindings);

    dynarray<VkPushConstantRange> push_constant_ranges = create_dynarray<VkPushConstantRange>(create_info.push_constant_count);
    for(uint32_t i=0; i<create_info.push_constant_count; ++i)
    {
      VkPushConstantRange push_constant_range = {};
      push_constant_range.stageFlags = to_vulkan_stage_flags(create_info.push_constants[i].stage);
      push_constant_range.offset     = create_info.push_constants[i].offset;
      push_constant_range.size       = create_info.push_constants[i].size;
      push_constant_ranges[i] = push_constant_range;
    }

    VkPipelineLayoutCreateInfo pipeline_layout_create_info = {};
    pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_create_info.setLayoutCount         = 1;
    pipeline_layout_create_info.pSetLayouts            = &pipeline_layout.descriptor_set_layout;
    pipeline_layout_create_info.pushConstantRangeCount = size(push_constant_ranges);
    pipeline_layout_create_info.pPushConstantRanges    = data(push_constant_ranges);
    VK_CHECK(vkCreatePipelineLayout(context.device, &pipeline_layout_create_info, nullptr, &pipeline_layout.pipeline_layout));
  }

  void deinit_pipeline_layout(const Context& context, PipelineLayout& pipeline_layout)
  {
    vkDestroyPipelineLayout(context.device, pipeline_layout.pipeline_layout, nullptr);
    vkDestroyDescriptorSetLayout(context.device, pipeline_layout.descriptor_set_layout, nullptr);
  }
}
