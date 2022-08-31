#include "core/command_buffer.hpp"
#include "core/context.hpp"
#include "render/camera.hpp"
#include "render/render_target.hpp"
#include "render/renderer.hpp"
#include "render/shader.hpp"
#include "resources/buffer.hpp"
#include "resources/material.hpp"
#include "resources/mesh.hpp"
#include "resources/sampler.hpp"

#include "chunk.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <chrono>

#include <utility>
#include <limits>
#include <fstream>
#include <algorithm>
#include <optional>
#include <vector>
#include <iostream>

#include <assert.h>
#include <string.h>
#include <stdlib.h>

// Constant sections
static constexpr const char *APPLICATION_NAME = "Vulkan";
static constexpr const char *ENGINE_NAME      = "Engine";
static constexpr const char *WINDOW_NAME      = "Vulkan";
static constexpr const unsigned WINDOW_WIDTH  = 1080;
static constexpr const unsigned WINDOW_HEIGHT = 720;

static constexpr const char *VERTEX_SHADER_FILE_NAME   = "shaders/vert.spv";
static constexpr const char *FRAGMENT_SHADER_FILE_NAME = "shaders/frag.spv";

struct Application
{
  Chunk *chunk;

  vulkan::mesh_layout_t     mesh_layout;
  vulkan::material_layout_t material_layout;

  vulkan::context_t   context;
  vulkan::allocator_t allocator;

  vulkan::mesh_t     mesh;
  vulkan::material_t material;

  vulkan::mesh_t  chunk_mesh;

  vulkan::render_target_t render_target;
  vulkan::renderer_t      renderer;

  vulkan::Camera camera;

  bool first_frame = true;

  double prev_x;
  double prev_y;
};

void application_on_render_target_invalidate(Application& application)
{
  VkDevice device = vulkan::context_get_device_handle(application.context);
  vkDeviceWaitIdle(device);

  vulkan::render_target_destroy(application.render_target);
  application.render_target = vulkan::render_target_create(application.context, application.allocator);

  unsigned width, height;
  vulkan::render_target_get_extent(application.render_target, width, height);

  application.camera.fov          = glm::radians(45.0f);
  application.camera.aspect_ratio = (float)width / (float)height;

  vulkan::renderer_destroy(application.renderer);
  application.renderer = vulkan::renderer_create(application.context,
      application.render_target,
      application.mesh_layout,
      application.material_layout,
      VERTEX_SHADER_FILE_NAME,
      FRAGMENT_SHADER_FILE_NAME);
}

void application_init(Application& application)
{
  application.context = vulkan::context_create(APPLICATION_NAME, ENGINE_NAME, WINDOW_NAME, WINDOW_WIDTH, WINDOW_HEIGHT);
  application.allocator = vulkan::allocator_create(application.context);

  vulkan::command_buffer_t command_buffer = command_buffer_create(application.context);
  command_buffer_begin(command_buffer);
  {
    application.mesh_layout     = vulkan::mesh_layout_create_default();
    application.material_layout = vulkan::material_layout_create_default(application.context);

    application.mesh     = vulkan::mesh_load   (command_buffer, application.context, application.allocator, "viking_room.obj");
    application.material = vulkan::material_load(command_buffer, application.context, application.allocator, "viking_room.png");

    application.chunk      = chunk_generate_random();
    application.chunk_mesh = chunk_generate_mesh(command_buffer, application.context, application.allocator, *application.chunk);

    command_buffer_end(command_buffer);
    command_buffer_submit(command_buffer);
    command_buffer_wait(command_buffer);
  }
  put(command_buffer);

  application.render_target = vulkan::render_target_create(application.context, application.allocator);

  unsigned width, height;
  vulkan::render_target_get_extent(application.render_target, width, height);

  application.camera.fov          = glm::radians(45.0f);
  application.camera.aspect_ratio = (float)width / (float)height;
  application.camera.position     = glm::vec3(2.0f, 2.0f, 0.5f);
  application.camera.yaw          = 0.0f;
  application.camera.pitch        = 0.0f;

  application.renderer = vulkan::renderer_create(application.context,
      application.render_target,
      application.mesh_layout,
      application.material_layout,
      VERTEX_SHADER_FILE_NAME,
      FRAGMENT_SHADER_FILE_NAME);

  GLFWwindow *window = vulkan::context_get_glfw_window(application.context);
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void application_deinit(Application& application)
{
  delete application.chunk;

  VkDevice device = vulkan::context_get_device_handle(application.context);
  vkDeviceWaitIdle(device);

  vulkan::renderer_destroy(application.renderer);
  vulkan::render_target_destroy(application.render_target);

  vulkan::put(application.mesh);
  vulkan::put(application.chunk_mesh);
  vulkan::put(application.material);

  vulkan::put(application.mesh_layout);
  vulkan::put(application.material_layout);

  vulkan::put(application.allocator);
  vulkan::put(application.context);
}

static constexpr float MOUSE_SENSITIVITY = 1 / 500.0f;
static constexpr float MOVEMENT_SPEED    = 0.01f;

void application_update(Application& application)
{
  vulkan::context_handle_events(application.context);

  GLFWwindow *window = vulkan::context_get_glfw_window(application.context);

  double x, y;
  glfwGetCursorPos(window, &x, &y);
  if(!application.first_frame)
  {
    const double dx = (x - application.prev_x) * MOUSE_SENSITIVITY;
    const double dy = (y - application.prev_y) * MOUSE_SENSITIVITY;
    vulkan::camera_rotate(application.camera, dx, dy);
  }
  application.first_frame = false;
  application.prev_x = x;
  application.prev_y = y;

  glm::vec3 direction = glm::vec3(0.0f, 0.0f, 0.0f);
  if(glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) direction += glm::vec3( 1.0f,  0.0f, 0.0f);
  if(glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) direction += glm::vec3(-1.0f,  0.0f, 0.0f);
  if(glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) direction += glm::vec3( 0.0f,  1.0f, 0.0f);
  if(glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) direction += glm::vec3( 0.0f, -1.0f, 0.0f);
  vulkan::camera_translate(application.camera, MOVEMENT_SPEED * direction);

}

bool application_render(Application& application)
{
  // Acquire frame
  const vulkan::Frame *frame = vulkan::render_target_begin_frame(application.render_target);
  if(!frame)
    return false;

  // Rendering
  vulkan::renderer_begin_render(application.renderer, frame);
  {
    unsigned width, height;
    vulkan::render_target_get_extent(application.render_target, width, height);
    vulkan::renderer_set_viewport_and_scissor(application.renderer, {width, height});
    vulkan::renderer_use_camera(application.renderer, application.camera);
    vulkan::renderer_draw(application.renderer, application.material, application.mesh);
    //vulkan::renderer_draw(application.renderer, application.material, application.chunk_mesh);
  }
  vulkan::renderer_end_render(application.renderer);

  // Present frame
  return vulkan::render_target_end_frame(application.render_target, frame);
}

void application_run(Application& application)
{
  while(!vulkan::context_should_destroy(application.context))
  {
    application_update(application);
    if(!application_render(application))
      application_on_render_target_invalidate(application);
  }
}

int main()
{
  glfwInit();

  Application application = {};
  application_init(application);
  application_run(application);
  application_deinit(application);

  glfwTerminate();
}
