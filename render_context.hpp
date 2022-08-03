#pragma once

#include "context.hpp"
#include "vulkan.hpp"

#include <glm/glm.hpp>

#include <optional>
#include <algorithm>
#include <fstream>
#include <limits>

#include <stdio.h>
#include <assert.h>
#include <vulkan/vulkan_core.h>

namespace vulkan
{
  // Describe layout of vertex attribute in a single buffer
  struct VertexAttributeDescription
  {
    enum class Type
    {
      FLOAT1, FLOAT2, FLOAT3, FLOAT4 // Which maniac who not use float as vertex input anyway?
    };

    size_t offset;
    Type type;
  };

  // You need multiple binding description if you use multiple buffer
  struct VertexBindingDescription
  {
    size_t stride;
    std::vector<VertexAttributeDescription> attribute_descriptions;
  };

  struct PipelineCreateInfo
  {
    VkShaderModule vert_shader_module;
    VkShaderModule frag_shader_module;
    std::vector<VertexBindingDescription> vertex_binding_descriptions;
  };

  struct RenderContext
  {
    VkSwapchainKHR swapchain;
    VkFormat       format;
    VkExtent2D     extent; // If this change we have to recreate the swapchain but fortunately not the render pass

    // TODO: How to support multiple render pass
    VkRenderPass          render_pass;
    VkPipelineLayout      pipeline_layout;
    VkPipeline            pipeline;
    VkDescriptorSetLayout descriptor_set_layout;

    std::vector<VkImage>       images;
    std::vector<VkImageView>   image_views;
    std::vector<VkFramebuffer> framebuffers;
  };

  struct RenderContextCreateInfo
  {
    PipelineCreateInfo pipeline;
  };

  RenderContext create_render_context(const Context& context, RenderContextCreateInfo create_info);
  void destroy_render_context(const Context& context, RenderContext& render_context);
}

