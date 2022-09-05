#include "camera.hpp"

#include <algorithm>

namespace vulkan
{
  void camera_rotate(Camera& camera, float yaw, float pitch)
  {
    camera.yaw   += yaw;
    camera.pitch += pitch;

    camera.pitch = std::clamp(camera.pitch, glm::radians(-90.0f), glm::radians(90.0f));
    transform_set_euler_angle(camera.transform, camera.yaw, camera.pitch, 0.0f);
  }

  void camera_translate(Camera& camera, glm::vec3 direction)
  {
    transform_translate_local(camera.transform, direction);
  }

  CameraMatrices camera_compute_matrices(const Camera& camera)
  {
    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 view  = glm::inverse(transform_as_mat4(camera.transform));

    glm::mat4 perspective = glm::perspective(camera.fov, camera.aspect_ratio, 0.1f, 10.0f);
    perspective[1][1] *= -1;

    // Rotate because glm::perspective expect z-axis to be pointing out of
    // screen but z-axis point upward in our world coordinate
    glm::mat4 correction  = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));

    glm::mat4 proj =  perspective * correction;

    return CameraMatrices{
      .mvp = proj * view * model,
      .model = model,
    };
  }
}
