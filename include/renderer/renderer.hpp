#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "types.hpp"

struct Camera;

class Renderer {
private:
	GLuint m_shaderProgram = 0;
	GLuint m_VAO = 0;
	GLuint m_VBO = 0;

	int m_width;
	int m_height;

	GLint m_uLocResolution;

	GLint m_uLocCameraPos;
	GLint m_uLocCameraForward;
	GLint m_uLocCameraRight;
	GLint m_uLocCameraUp;

	std::vector<Sphere> m_spheres;
	GLuint m_sphereSSBO = 0;
	GLint m_uLocNumSpheres;

	void setupShaders();
	void setupQuad();
	void setupSpheres();

	void uploadSpheres(const std::vector<Sphere>& spheres);

	void cleanup();

public:
	Renderer(int width, int height);
	~Renderer();

	void render(const Camera& camera);
};