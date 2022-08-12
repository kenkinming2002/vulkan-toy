project('vulkan', 'cpp',
  version : '0.1',
  default_options : ['warning_level=3',
                     'cpp_std=c++20'])

glfw_dep   = dependency('glfw3')
vulkan_dep = dependency('vulkan')
glm_dep    = dependency('glm')

srcs = [
  'src/allocator.cpp',
  'src/buffer.cpp',
  'src/command_buffer.cpp',
  'src/context.cpp',
  'src/descriptor_set.cpp',
  'src/descriptor_set_layout.cpp',
  'src/fence.cpp',
  'src/framebuffer.cpp',
  'src/image.cpp',
  'src/image_view.cpp',
  'src/impl.cpp',
  'src/main.cpp',
  'src/pipeline.cpp',
  'src/pipeline_layout.cpp',
  'src/render_context.cpp',
  'src/render_pass.cpp',
  'src/sampler.cpp',
  'src/shader.cpp',
  'src/swapchain.cpp',
  'src/vk_check.cpp',
  'src/vulkan.cpp',
]

exe = executable('vulkan', srcs, dependencies : [glfw_dep, vulkan_dep, glm_dep],
  install : true)

test('basic', exe)
