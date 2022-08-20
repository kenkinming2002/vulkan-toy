#include "sampler.hpp"

#include "vk_check.hpp"

namespace vulkan
{
  struct Sampler
  {
    Ref ref;

    const Context *context;

    VkSampler handle;
  };

  static void sampler_free(ref_t ref)
  {
    sampler_t sampler = container_of(ref, Sampler, ref);
    vkDestroySampler(sampler->context->device, sampler->handle, nullptr);
    delete sampler;
  }

  sampler_t sampler_create_simple(const Context *context, size_t mip_levels)
  {
    sampler_t sampler = new Sampler {};
    sampler->ref.count = 1;
    sampler->ref.free  = sampler_free;

    sampler->context = context;

    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(sampler->context->physical_device, &properties);

    VkSamplerCreateInfo sampler_create_info{};
    sampler_create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_create_info.magFilter               = VK_FILTER_LINEAR;
    sampler_create_info.minFilter               = VK_FILTER_LINEAR;
    sampler_create_info.addressModeU            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_create_info.addressModeV            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_create_info.addressModeW            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_create_info.anisotropyEnable        = VK_TRUE;
    sampler_create_info.maxAnisotropy           = properties.limits.maxSamplerAnisotropy;
    sampler_create_info.borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    sampler_create_info.unnormalizedCoordinates = VK_FALSE;
    sampler_create_info.compareEnable           = VK_FALSE;
    sampler_create_info.compareOp               = VK_COMPARE_OP_ALWAYS;
    sampler_create_info.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler_create_info.mipLodBias              = 0.0f;
    sampler_create_info.minLod                  = 0.0f;
    sampler_create_info.maxLod                  = mip_levels;
    VK_CHECK(vkCreateSampler(sampler->context->device, &sampler_create_info, nullptr, &sampler->handle));

    return sampler;
  }

  ref_t sampler_as_ref(sampler_t sampler)
  {
    return &sampler->ref;
  }

  VkSampler sampler_get_handle(sampler_t sampler)
  {
    return sampler->handle;
  }
}
