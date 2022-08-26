#include "sampler.hpp"

#include "vk_check.hpp"

namespace vulkan
{
  struct Sampler
  {
    Ref ref;

    context_t context;

    VkSampler handle;
  };
  REF_DEFINE(Sampler, sampler_t, ref);

  static void sampler_free(ref_t ref)
  {
    sampler_t sampler = container_of(ref, Sampler, ref);

    VkDevice device = context_get_device_handle(sampler->context);
    vkDestroySampler(device, sampler->handle, nullptr);

    put(sampler->context);
    delete sampler;
  }

  sampler_t sampler_create_simple(context_t context)
  {
    sampler_t sampler = new Sampler {};
    sampler->ref.count = 1;
    sampler->ref.free  = sampler_free;

    get(context);
    sampler->context = context;

    VkPhysicalDevice physical_device = context_get_physical_device(sampler->context);
    VkDevice         device          = context_get_device_handle(sampler->context);

    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(physical_device, &properties);

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
    sampler_create_info.maxLod                  = VK_LOD_CLAMP_NONE;
    VK_CHECK(vkCreateSampler(device, &sampler_create_info, nullptr, &sampler->handle));

    return sampler;
  }

  VkSampler sampler_get_handle(sampler_t sampler)
  {
    return sampler->handle;
  }
}
