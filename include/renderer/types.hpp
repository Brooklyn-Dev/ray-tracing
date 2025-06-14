#pragma once

#include <glm/glm.hpp>

enum MaterialFlags {
	FLAG_NONE = 0,
	FLAG_CHECKERBOARD = 1
};

struct alignas(16) Material {
	glm::vec3 colour;  // Diffuse colour
	float smoothness;  // Controls glossiness

	glm::vec3 emissionColour;  // Colour of emitted light
	float emissionStrength;  // Intensity of emitted light

	glm::vec3 specularColour;
	int flag;
};

struct alignas(16) Sphere {
	glm::vec3 position;
	float radius;

	Material material;
};

struct alignas(16) Plane {
	glm::vec3 position;
	float _pad0;

	glm::vec3 normal;
	float _pad1;

	Material material;
};