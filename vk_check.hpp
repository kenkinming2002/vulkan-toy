#pragma once

#include "vulkan/vulkan_core.h"

void vk_check(const char *expr_str, VkResult result);
#define VK_CHECK(expr) do { vk_check(#expr, expr); } while(0);
