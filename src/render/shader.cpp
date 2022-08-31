#include "shader.hpp"

#include "vk_check.hpp"
#include "utils.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <type_traits>


template<typename T>
static T *libc_check(const char* expr_str, T *ptr)
{
  if(!ptr)
  {
    perror(expr_str);
    abort();
  }
  return ptr;
}

template<typename T>
static T libc_check(const char* expr_str, T value) requires(std::is_integral_v<T> && std::is_signed_v<T>)
{
  if(value < 0)
  {
    perror(expr_str);
    abort();
  }
  return value;
}

#define LIBC_CHECK(expr) libc_check(#expr, expr)

static inline dynarray<char> read_file(const char *file_name)
{
  FILE *file = LIBC_CHECK(fopen(file_name, "rb"));
  const long begin = ftell(file);
  LIBC_CHECK(fseek(file, 0, SEEK_END));
  const long end = ftell(file);
  LIBC_CHECK(fseek(file, 0, SEEK_SET));

  dynarray<char> content = create_dynarray<char>(end-begin);
  if(fread(data(content), size(content), 1, file) !=  1)
  {
    fprintf(stderr, "Failed to read file");
    abort();
  }
  return content;
}

namespace vulkan
{
  struct Shader
  {
    Ref ref;
    context_t context;
    VkShaderModule handle;
  };
  REF_DEFINE(Shader, shader_t, ref);

  static void shader_free(ref_t ref)
  {
    shader_t shader = container_of(ref, Shader, ref);

    VkDevice device = context_get_device_handle(shader->context);
    vkDestroyShaderModule(device, shader->handle, nullptr);
    put(shader->context);

    delete shader;
  }

  shader_t shader_load(context_t context, const char *file_name)
  {
    shader_t shader = new Shader;
    shader->ref.count = 1;
    shader->ref.free  = shader_free;

    get(context);
    shader->context = context;

    dynarray<char> code = read_file(file_name);

    VkDevice device = context_get_device_handle(shader->context);

    VkShaderModuleCreateInfo shader_module_create_info = {};
    shader_module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shader_module_create_info.codeSize = size(code);
    shader_module_create_info.pCode    = reinterpret_cast<const uint32_t*>(data(code));
    VK_CHECK(vkCreateShaderModule(device, &shader_module_create_info, nullptr, &shader->handle));

    destroy_dynarray(code);

    return shader;
  }

  VkShaderModule shader_get_handle(shader_t shader)
  {
    return shader->handle;
  }
}
