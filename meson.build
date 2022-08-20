project('vulkan', 'cpp',
  version : '0.1',
  default_options : ['warning_level=3',
                     'cpp_std=c++20'])

glfw_dep   = dependency('glfw3')
vulkan_dep = dependency('vulkan')
glm_dep    = dependency('glm')

srcs = [
  'src/core/command_buffer.cpp',
  'src/core/context.cpp',
  'src/core/fence.cpp',
  'src/core/semaphore.cpp',
  'src/descriptor_set.cpp',
  'src/frame.cpp',
  'src/framebuffer.cpp',
  'src/impl.cpp',
  'src/main.cpp',
  'src/pipeline.cpp',
  'src/render_pass.cpp',
  'src/render_target.cpp',
  'src/renderer.cpp',
  'src/resources/allocator.cpp',
  'src/resources/buffer.cpp',
  'src/resources/image.cpp',
  'src/resources/image_view.cpp',
  'src/resources/mesh.cpp',
  'src/resources/sampler.cpp',
  'src/resources/texture.cpp',
  'src/shader.cpp',
  'src/swapchain.cpp',
  'src/vk_check.cpp',
]

exe = executable('vulkan', srcs,
  dependencies : [glfw_dep, vulkan_dep, glm_dep],
  include_directories : 'src',
  install : true)

test('basic', exe)
