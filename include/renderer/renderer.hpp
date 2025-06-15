#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "skybox/skybox.hpp"
#include "types.hpp"

struct Camera;
struct Scene;

class Renderer {
private:
	GLuint m_fbo = 0;

	// Textures
	GLuint m_accumulatedImage = 0;
	GLuint m_displayTexture = 0;

	// Shader and Quad
	GLuint m_shaderProgram = 0;
	GLuint m_VAO = 0;
	GLuint m_VBO = 0;

	uint32_t m_width;
	uint32_t m_height;

	float m_gamma = 2.2;
	uint32_t m_maxBounces = 2;
	uint32_t m_samplesPerPixel = 1;
	uint32_t m_frame = 1;

	// Uniform locations for shader program
	GLint m_uLocResolution;
	GLint m_uLocCameraPos;
	GLint m_uLocCameraForward;
	GLint m_uLocCameraRight;
	GLint m_uLocCameraUp;
	GLint m_uLocGamma;
	GLint m_uLocMaxBounces;
	GLint m_uLocSamplesPerPixel;
	GLint m_uLocFrame;
	GLint m_uLocSkyboxTexture;
	GLint m_uLocHasSkybox;
	GLint m_uLocSkyboxExposure;
	GLint m_uLocSunDirection;
	GLint m_uLocSunColour;
	GLint m_uLocSunIntensity;
	GLint m_uLocSunFocus;

	Skybox m_skybox;
	float m_skyboxExposure = 1.0f;

	glm::vec3 m_sunDirection;
	glm::vec3 m_sunColour;
	float m_sunIntensity;
	float m_sunFocus;

	std::vector<Sphere> m_spheres;
	GLuint m_sphereSSBO = 0;
	GLint m_uLocNumSpheres;

	std::vector<Plane> m_planes;
	GLuint m_planeSSBO = 0;
	GLint m_uLocNumPlanes;

	void setupShaders();
	void setupQuad();

	void setupSpheres();
	void setupPlanes();

	void createTexturesAndFBO(uint32_t width, uint32_t height);

	void resetFrame();

	void cleanup();

public:
	Renderer(uint32_t width, uint32_t height);
	~Renderer();

	void uploadSpheres(const std::vector<Sphere>& spheres);
	void uploadPlanes(const std::vector<Plane>& planes);

	GLuint getDisplayTexture() const;
	uint32_t getFrame() const;

	void setGamma(float gamma);
	void setMaxBounces(uint32_t bounces);
	void setSamplesPerPixel(uint32_t samples);
	void setSkybox(const std::string& filepath);
	void setSkyboxExposure(float exposure);
	void setSunDirection(glm::vec3 direction);
	void setSunColour(glm::vec3 colour);
	void setSunIntensity(float intensity);
	void setSunFocus(float focus);

	void loadScene(const Scene& scene);

	void onResize(uint32_t width, uint32_t height);
	void render(const Camera& camera);
};