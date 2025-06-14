#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "skybox/skybox.hpp"

Skybox::Skybox() {}

Skybox::~Skybox() {
    cleanup();
}

GLuint Skybox::getTextureID() const {
	return m_textureId;
}

int Skybox::getWidth() const {
	return m_width;
}

int Skybox::getHeight() const {
	return m_height;
}

bool Skybox::load(const std::string& filepath) {
	cleanup();

    float* imageData = stbi_loadf(filepath.c_str(), &m_width, &m_height, &m_channels, 0);

    if (!imageData) {
        std::cerr << "Error (Skybox): Failed to load HDR image: " << filepath << " - " << stbi_failure_reason() << std::endl;
        return false;
    }

    glGenTextures(1, &m_textureId);
    glBindTexture(GL_TEXTURE_2D, m_textureId);

    // Texture parameters for HDR
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Determine internal format based on channels (default 3)
    GLenum internalFormat = GL_RGB32F;
    GLenum format = GL_RGB;

    if (m_channels == 4) {
        internalFormat = GL_RGBA32F;
        format = GL_RGBA;
    }
    else if (m_channels == 2) {
        internalFormat = GL_RG32F;
        format = GL_RG;
    }
    else if (m_channels == 1) {
        internalFormat = GL_R32F;
        format = GL_RED;
    }

    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, m_width, m_height, 0, format, GL_FLOAT, imageData);
    glGenerateMipmap(GL_TEXTURE_2D);

    // Free CPU-side image data
    stbi_image_free(imageData);
    glBindTexture(GL_TEXTURE_2D, 0);

    return true;
}

void Skybox::cleanup() {
    if (m_textureId != 0) {
        glDeleteTextures(1, &m_textureId);
        m_textureId = 0;
        m_width = 0;
        m_height = 0;
        m_channels = 0;
    }
}