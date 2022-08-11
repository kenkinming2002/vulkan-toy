#include "pipeline_layout.hpp"

#include "vk_check.hpp"
#include "utils.hpp"

namespace vulkan
{
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
    pipeline_layout_create_info.pSetLayouts            = &create_info.descriptor_set_layout.handle;
    pipeline_layout_create_info.pushConstantRangeCount = size(push_constant_ranges);
    pipeline_layout_create_info.pPushConstantRanges    = data(push_constant_ranges);
    VK_CHECK(vkCreatePipelineLayout(context.device, &pipeline_layout_create_info, nullptr, &pipeline_layout.handle));
  }

  void deinit_pipeline_layout(const Context& context, PipelineLayout& pipeline_layout)
  {
    vkDestroyPipelineLayout(context.device, pipeline_layout.handle, nullptr);
  }
}
