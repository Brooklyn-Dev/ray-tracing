#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "types.hpp"

struct Camera;

class Renderer {
private:
	GLuint m_framebuffer = 0;
	GLuint m_colourTexture = 0;

	GLuint m_shaderProgram = 0;
	GLuint m_VAO = 0;
	GLuint m_VBO = 0;

	uint32_t m_width;
	uint32_t m_height;

	uint32_t m_maxBounces = 2;
	uint32_t m_samples = 1;

	GLint m_uLocResolution;

	GLint m_uLocCameraPos;
	GLint m_uLocCameraForward;
	GLint m_uLocCameraRight;
	GLint m_uLocCameraUp;

	GLint m_uLocMaxBounces;
	GLint m_uLocSamples;

	std::vector<Sphere> m_spheres;
	GLuint m_sphereSSBO = 0;
	GLint m_uLocNumSpheres;

	void setupShaders();
	void setupQuad();
	void setupSpheres();

	void uploadSpheres(const std::vector<Sphere>& spheres);

	void createFramebuffer(uint32_t width, uint32_t height);

	void cleanup();

public:
	Renderer(uint32_t width, uint32_t height);
	~Renderer();

	GLuint getColourTexture() const;

	void onResize(uint32_t width, uint32_t height);

	void setMaxBounces(uint32_t bounces);
	void setSamples(uint32_t samples);

	void render(const Camera& camera);
};