#pragma once

#include <string>
#include <vector>

#include <glm/glm.hpp>

#include "../renderer/types.hpp"
#include "../camera/camera.hpp"

struct Scene {
    std::string name = "Default Scene";

    std::vector<Sphere> spheres;
    std::vector<Plane> planes;
    std::vector<Quad> quads;

    float gamma = 2.2f;
    int maxBounces = 2;
    int samplesPerPixel = 1;

    Camera camera;

    std::string skyboxPath = "";
    float skyboxExposureEV = 0.0f;

    float sunPitch = 50.0f;
    float sunYaw = -30.0f;
    glm::vec3 sunColour = glm::vec3(1.0f, 1.0f, 0.9f);
    float sunIntensity = 200;
    float sunFocus = 500;

    Scene();

    float getSkyboxExposure() const;
    glm::vec3 getSunDirection() const;
};