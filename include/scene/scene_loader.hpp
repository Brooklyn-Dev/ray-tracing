#pragma once

#include <string>

#include "scene.hpp"

namespace SceneLoader {
    bool loadScene(const std::string& filename, Scene& scene);
}