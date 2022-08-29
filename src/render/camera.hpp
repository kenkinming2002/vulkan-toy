#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

namespace vulkan
{
  struct Camera
  {
    float fov;
    float aspect_ratio;

    glm::vec3 position;
    float yaw, pitch;
  };

  glm::vec3 camera_forward(const Camera& camera);
  glm::vec3 camera_left(const Camera& camera);
  glm::vec3 camera_up(const Camera& camera);

  void camera_rotate(Camera& camera, float yaw, float pitch);
  void camera_translate(Camera& camera, glm::vec3 dir);

  // TODO: Ideally, we would also want to include the normal matrix but push
  //       constant can only send up to 128 bytes which equal 2 glm::mat4.
  //       Figure out a way around it perhaps via dynamic uniform buffer.
  struct CameraMatrices
  {
    glm::mat4 mvp;
    glm::mat4 model;
  };
  static_assert(sizeof(CameraMatrices) == 128);

  CameraMatrices camera_compute_matrices(const Camera& camera);
}
