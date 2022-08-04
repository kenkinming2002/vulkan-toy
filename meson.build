project('vulkan', 'cpp',
  version : '0.1',
  default_options : ['warning_level=3',
                     'cpp_std=c++20'])

glfw_dep   = dependency('glfw3')
vulkan_dep = dependency('vulkan')
glm_dep    = dependency('glm')

srcs = ['vulkan.cpp', 'main.cpp', 'render_context.cpp', 'command_buffer.cpp', 'buffer.cpp']
exe = executable('vulkan', srcs, dependencies : [glfw_dep, vulkan_dep, glm_dep],
  install : true)

test('basic', exe)
