#include "transform.hpp"

namespace vulkan
{
  glm::vec3 transform_right(const Transform& camera)   { return glm::rotate(camera.rotation, WORLD_RIGHT); }
  glm::vec3 transform_forward(const Transform& camera) { return glm::rotate(camera.rotation, WORLD_FORWARD); }
  glm::vec3 transform_up(const Transform& camera)      { return glm::rotate(camera.rotation, WORLD_UP); }

  void transform_translate_local(Transform& transform, glm::vec3 direction)
  {
    transform.position +=
      direction.x * transform_right(transform) +
      direction.y * transform_forward(transform) +
      direction.z * transform_up(transform);
  }

  void transform_set_euler_angle(Transform& transform, float yaw, float pitch, float roll)
  {
    transform.rotation =
      glm::angleAxis(yaw,   WORLD_UP) *
      glm::angleAxis(pitch, WORLD_RIGHT) *
      glm::angleAxis(roll,  WORLD_FORWARD);
  }

  glm::mat4 transform_as_mat4(const Transform& transform)
  {
    return glm::translate(glm::mat4(1.0f), transform.position) * glm::mat4_cast(transform.rotation);
  }
}
