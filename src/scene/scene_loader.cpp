#include <fstream>
#include <iostream>

#include "scene/scene_loader.hpp"

#include "json.hpp"

using json = nlohmann::json;

namespace SceneLoader {
    glm::vec3 parseVec3(const json& j_vec) {
        return glm::vec3(j_vec[0].get<float>(), j_vec[1].get<float>(), j_vec[2].get<float>());
    }

    Material parseMaterial(const json& j_mat) {
        Material mat;
        mat.colour = parseVec3(j_mat.at("colour"));
        mat.smoothness = j_mat.at("smoothness").get<float>();
        mat.emissionColour = parseVec3(j_mat.at("emissionColour"));
        mat.emissionStrength = j_mat.at("emissionStrength").get<float>();
        mat.specularColour = parseVec3(j_mat.at("specularColour"));
        mat.flag = j_mat.at("flag").get<int>();
        mat.specularProbability = j_mat.at("specularProbability").get<float>();
        mat._pad0 = 0.0f;
        mat._pad1 = 0.0f;
        mat._pad2 = 0.0f;
        return mat;
    }

    bool loadScene(const std::string& filename, Scene& scene) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Error: Could not open scene file: " << filename << std::endl;
            return false;
        }

        json j;
        try {
            file >> j;
        } catch (const json::parse_error& e) {
            std::cerr << "Error parsing JSON in " << filename << ": " << e.what() << std::endl;
            return false;
        }

        scene.name = j.value("name", "Unknown Scene");

        // Clear existing scene
        scene.spheres.clear();
        scene.planes.clear();

        // Parse Tracing
        if (j.contains("tracing")) {
            const auto& j_tracing = j.at("tracing");
            scene.gamma = j_tracing.at("gamma");
            scene.maxBounces = j_tracing.at("maxBounces");
            scene.samplesPerPixel = j_tracing.at("samplesPerPixel");
        }

        // Parse Camera
        if (j.contains("camera")) {
            const auto& j_cam = j.at("camera");
            scene.camera.position = parseVec3(j_cam.at("position"));
            scene.camera.pitch = j_cam.at("pitch");
            scene.camera.yaw = j_cam.at("yaw");
            scene.camera.updateOrientation();
        }

        // Parse Environment
        if (j.contains("environment")) {
            const auto& j_env = j.at("environment");
            scene.skyboxPath = j_env.value("skyboxPath", "");
            scene.skyboxExposureEV = j_env.value("exposureEV", 0.0f);
            scene.sunPitch = j_env.value("sunPitch", -30.0f);
            scene.sunYaw = j_env.value("sunYaw", 50.0f);
            scene.sunColour = parseVec3(j_env.at("sunColour"));
            scene.sunIntensity = j_env.value("sunIntensity", 200.0f);
            scene.sunFocus = j_env.value("sunFocus", 500.0f);
        }

        // Parse Spheres
        if (j.contains("spheres") && j.at("spheres").is_array())
            for (const auto& j_sphere : j.at("spheres")) {
                Sphere s;
                s.position = parseVec3(j_sphere.at("position"));
                s.radius = j_sphere.at("radius").get<float>();
                s.material = parseMaterial(j_sphere.at("material"));
                scene.spheres.push_back(s);
            }

        // Parse Planes
        if (j.contains("planes") && j.at("planes").is_array())
            for (const auto& j_plane : j.at("planes")) {
                Plane p;
                p.position = parseVec3(j_plane.at("position"));
                p.normal = parseVec3(j_plane.at("normal"));
                p.material = parseMaterial(j_plane.at("material"));
                scene.planes.push_back(p);
            }

        std::cout << "Scene '" << j.value("name", "Unknown Scene") << "' loaded successfully." << std::endl;
        return true;
    }
}