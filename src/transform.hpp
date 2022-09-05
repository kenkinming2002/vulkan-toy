#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

namespace vulkan
{
  struct Transform
  {
    glm::vec3 position;
    glm::quat rotation;
  };

  static constexpr glm::vec3 WORLD_RIGHT   = glm::vec3(1.0f, 0.0f, 0.0f);
  static constexpr glm::vec3 WORLD_FORWARD = glm::vec3(0.0f, 1.0f, 0.0f);
  static constexpr glm::vec3 WORLD_UP      = glm::vec3(0.0f, 0.0f, 1.0f);

  glm::vec3 transform_forward(const Transform& transform);
  glm::vec3 transform_left(const Transform& transform);
  glm::vec3 transform_up(const Transform& transform);

  void transform_translate_local(Transform& transform, glm::vec3 direction);
  void transform_set_euler_angle(Transform& transform, float yaw, float pitch, float roll);

  glm::mat4 transform_as_mat4(const Transform& transform);
}
