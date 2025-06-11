#pragma once

#include <glm/glm.hpp>

struct alignas(16) Material {
	glm::vec3 colour;
	float _pad0;

	glm::vec3 emissionColour;
	float emissionStrength;
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