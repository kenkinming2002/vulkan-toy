#include "camera.hpp"

#include <glm/ext/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>

namespace vulkan
{
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
