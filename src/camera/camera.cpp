#include "camera/camera.hpp"

Camera::Camera()
	: position(0.0f, 2.0f, 0.0f),
	  yaw(-90.0f),
	  pitch(0.0f)
{
	updateOrientation();
}

void Camera::updateOrientation() {
    float yawRad = glm::radians(yaw);
    float pitchRad = glm::radians(pitch);

    forward.x = cos(pitchRad) * cos(yawRad);
    forward.y = sin(pitchRad);
    forward.z = cos(pitchRad) * sin(yawRad);
    forward = glm::normalize(forward);

    right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));
    up = glm::normalize(glm::cross(right, forward));
}