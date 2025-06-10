#pragma once

#include <string>
#include <glad/glad.h>
#include <glm/glm.hpp>

class Skybox {
public:
	Skybox();
	~Skybox();

	bool load(const std::string& filepath);
	void cleanup();

	GLuint getTextureID() const;
	int getWidth() const;
	int getHeight() const;

private:
	GLuint m_textureId = 0;
	int m_width = 0;
	int m_height = 0; 
	int m_channels = 0;
};