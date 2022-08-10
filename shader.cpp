#include "shader.hpp"

#include "vk_check.hpp"
#include "utils.hpp"

#include <stdio.h>

#define LIBC_CHECK(expr) do { if(expr == -1) { perror(#expr); abort(); } } while(0)

namespace vulkan
{
  static inline dynarray<char> read_file(const char *file_name)
  {
    FILE *file = fopen(file_name, "rb");

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

  void load_shader(const Context& context, ShaderCreateInfo create_info, Shader& shader)
  {
    dynarray<char> code = read_file(create_info.file_name);

    VkShaderModuleCreateInfo shader_module_create_info = {};
    shader_module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shader_module_create_info.codeSize = size(code);
    shader_module_create_info.pCode    = reinterpret_cast<const uint32_t*>(data(code));
    VK_CHECK(vkCreateShaderModule(context.device, &shader_module_create_info, nullptr, &shader.module));

    destroy_dynarray(code);
  }

  void deinit_shader(const Context& context, Shader& shader)
  {
    vkDestroyShaderModule(context.device, shader.module, nullptr);
    shader = {};
  }
}
