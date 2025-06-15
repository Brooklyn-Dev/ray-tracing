#include "scene/scene.hpp"

Scene::Scene() : camera() { }

float Scene::getSkyboxExposure() const {
    return powf(2.0f, skyboxExposureEV);
}

glm::vec3 Scene::getSunDirection() const {
    float pitchRad = glm::radians(sunPitch);
    float yawRad = glm::radians(sunYaw);

    glm::vec3 dir;
    dir.x = cos(pitchRad) * sin(yawRad);
    dir.y = sin(pitchRad);
    dir.z = cos(pitchRad) * cos(yawRad);

    return glm::normalize(dir);
}