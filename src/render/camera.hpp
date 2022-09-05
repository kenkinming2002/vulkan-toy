#pragma once

#include "transform.hpp"

namespace vulkan
{
  struct Camera
  {
    Transform transform;

    float yaw, pitch;
    float fov, aspect_ratio;
  };

  void camera_rotate(Camera& camera, float yaw, float pitch);
  void camera_translate(Camera& camera, glm::vec3 direction);

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
