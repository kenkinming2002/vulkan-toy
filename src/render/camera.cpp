#include "camera.hpp"

#include <glm/ext/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>

namespace vulkan
{
  glm::vec3 camera_forward(const Camera& camera)
  {
    return glm::vec3(
      cos(camera.pitch) * cos(camera.yaw),
      cos(camera.pitch) * sin(camera.yaw),
      sin(camera.pitch)
    );
  }

  glm::vec3 camera_left(const Camera& camera)
  {
    return glm::vec3(
      -sin(camera.yaw),
      cos(camera.yaw),
      0.0f
    );
  }

  glm::vec3 camera_up(const Camera& camera)
  {
    return glm::vec3(
      -sin(camera.pitch) * cos(camera.yaw),
      -sin(camera.pitch) * sin(camera.yaw),
      cos(camera.pitch)
    );
  }

  void camera_rotate(Camera& camera, float yaw, float pitch)
  {
    camera.yaw   += yaw;
    camera.pitch += pitch;
  }

  void camera_translate(Camera& camera, glm::vec3 dir)
  {
    const glm::vec3 forward = camera_forward(camera);
    const glm::vec3 left    = camera_left(camera);
    const glm::vec3 up      = camera_up(camera);

    camera.position +=
      dir.x * forward +
      dir.y * left +
      dir.z * up;
  }

  CameraMatrices camera_compute_matrices(const Camera& camera)
  {
    glm::mat4 model = glm::mat4(1.0f);

    glm::vec3 direction;
    direction.x = cos(camera.pitch) * cos(camera.yaw);
    direction.y = cos(camera.pitch) * sin(camera.yaw);
    direction.z = sin(camera.pitch);
    glm::mat4 view  = glm::lookAt(camera.position, camera.position + direction, glm::vec3(0.0f, 0.0f, 1.0f));

    glm::mat4 proj  = glm::perspective(camera.fov, camera.aspect_ratio, 0.1f, 10.0f);
    proj[1][1] *= -1;

    return CameraMatrices{
      .mvp = proj * view * model,
      .model = model,
    };
  }
}
