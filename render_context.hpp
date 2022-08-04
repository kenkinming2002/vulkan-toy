#pragma once

#include "context.hpp"
#include "vulkan.hpp"
#include "buffer.hpp"

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

  struct RenderContextCreateInfo
  {
    VkShaderModule vert_shader_module;
    VkShaderModule frag_shader_module;
    std::vector<VertexBindingDescription> vertex_binding_descriptions;
  };

  struct RenderContext
  {
    uint32_t                      image_count;
    VkExtent2D                    extent;
    VkSurfaceTransformFlagBitsKHR surface_transform;

    VkSurfaceFormatKHR       surface_format;
    VkPresentModeKHR         present_mode;

    VkSwapchainKHR swapchain;

    // TODO: How to support multiple render pass
    VkRenderPass          render_pass;
    VkPipelineLayout      pipeline_layout;
    VkPipeline            pipeline;
    VkDescriptorSetLayout descriptor_set_layout;

    VkImage       *images;
    VkImageView   *image_views;

    ImageAllocation *depth_image_allocations;
    VkImageView     *depth_image_views;

    VkFramebuffer *framebuffers;
  };

  RenderContext create_render_context(const Context& context, Allocator& allocator, RenderContextCreateInfo create_info);
  void destroy_render_context(const Context& context, Allocator& allocator, RenderContext& render_context);
}

