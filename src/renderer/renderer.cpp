#include <fstream>
#include <iostream>
#include <sstream>

#include <glm/gtc/type_ptr.hpp>

#include "camera/camera.hpp"
#include "renderer/renderer.hpp"
#include "utils/io.hpp"
#include "utils/shader.hpp"

Renderer::Renderer(uint32_t width, uint32_t height)
	: m_width(width), m_height(height) {
	setupShaders();
	setupQuad();
    setupSpheres();

    createFramebuffer(width, height);
}

Renderer::~Renderer() {
	cleanup();
}

void Renderer::setupShaders() {
    m_shaderProgram = createShaderProgram("shaders/vertex.glsl", "shaders/fragment.glsl");
    if (m_shaderProgram == 0) {
        std::cerr << "Failed to create shader program" << std::endl;
    }

    // Cache uniform locations
    m_uLocResolution = glGetUniformLocation(m_shaderProgram, "uResolution");

    m_uLocCameraPos = glGetUniformLocation(m_shaderProgram, "uCameraPosition");
    m_uLocCameraForward = glGetUniformLocation(m_shaderProgram, "uCameraForward");
    m_uLocCameraRight = glGetUniformLocation(m_shaderProgram, "uCameraRight");
    m_uLocCameraUp = glGetUniformLocation(m_shaderProgram, "uCameraUp");

    m_uLocMaxBounces = glGetUniformLocation(m_shaderProgram, "uMaxBounces");
    m_uLocSamples = glGetUniformLocation(m_shaderProgram, "uSamples");

    m_uLocNumSpheres = glGetUniformLocation(m_shaderProgram, "uNumSpheres");
}

void Renderer::setupQuad() {
    // Fullscreen quad vertices
    float vertices[] = {
        // x, y, u, v
        -1.0f, -1.0f, 0.0f, 0.0f,  // Bottom-left
        1.0f, -1.0f, 1.0f, 0.0f,   // Bottom-right
        -1.0f, 1.0f, 0.0f, 1.0f,   // Top-left
        1.0f, 1.0f, 1.0f, 1.0f     // Top-right
    };

    glGenVertexArrays(1, &m_VAO);
    glGenBuffers(1, &m_VBO);

    glBindVertexArray(m_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Position attribute (location 0)
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // UV attribute (location 1)
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
}

void Renderer::setupSpheres() {
    m_spheres.clear();

    const glm::vec3 colours[] = {
        glm::vec3(1.0f, 0.0f, 0.0f),  // Red
        glm::vec3(0.0f, 1.0f, 0.0f),  // Green  
        glm::vec3(0.0f, 0.0f, 1.0f)   // Blue
    };

    const float startX = -2.0f;
    const float spacing = 2.0f;
    const float baseRadius = 0.3f;
    const float radiusIncrement = 0.2f;

    for (int i = 0; i < 3; ++i) {
        Sphere sphere;
        sphere.position = glm::vec3(startX + i * spacing, 0.0f, -3.0f);
        sphere.radius = baseRadius + i * radiusIncrement;
        sphere.material.colour = colours[i];
        sphere.material.emissionColour = glm::vec3(0.0f);
        sphere.material.emissionStrength = 0.0f;
        m_spheres.push_back(sphere);
    }

    // "Ground"
    Sphere sphereGround;
    sphereGround.position = glm::vec3(0.0f, -20.5f, -1.0f);
    sphereGround.radius = 20.0f;
    sphereGround.material.colour = glm::vec3(0.3f, 0.0f, 0.7f);
    sphereGround.material.emissionColour = glm::vec3(0.0f);
    sphereGround.material.emissionStrength = 0.0f;
    m_spheres.push_back(sphereGround);

    // Light
    Sphere sphereLight;
    sphereLight.position = glm::vec3(0.0f, 5.0f, -22.0f);
    sphereLight.radius = 10.0f;
    sphereLight.material.colour = glm::vec3(1.0f);
    sphereLight.material.emissionColour = glm::vec3(1.0f);
    sphereLight.material.emissionStrength = 10.0f;
    m_spheres.push_back(sphereLight);

    // Initialise sphere buffer
    if (m_sphereSSBO == 0)
        glGenBuffers(1, &m_sphereSSBO);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_sphereSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, m_spheres.size() * sizeof(Sphere), nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_sphereSSBO);  // binding = 0
}

void Renderer::uploadSpheres(const std::vector<Sphere>& spheres) {
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_sphereSSBO);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, spheres.size() * sizeof(Sphere), spheres.data());

    glUniform1i(m_uLocNumSpheres, (GLint)spheres.size());
}

void Renderer::createFramebuffer(uint32_t width, uint32_t height) {
    if (m_framebuffer != 0) {
        glDeleteFramebuffers(1, &m_framebuffer);
        glDeleteTextures(1, &m_colourTexture);
    }

    glGenFramebuffers(1, &m_framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);

    // Create colour texture
    glGenTextures(1, &m_colourTexture);
    glBindTexture(GL_TEXTURE_2D, m_colourTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Attach texture to framebuffer
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_colourTexture, 0);

    // Create and attach depth buffer
    GLuint depthBuffer;
    glGenRenderbuffers(1, &depthBuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, depthBuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depthBuffer);
    glDeleteRenderbuffers(1, &depthBuffer);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cerr << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::onResize(uint32_t width, uint32_t height) {
    if (width == 0 || height == 0 || (width == m_width && height == m_height))
        return;

    m_width = width;
    m_height = height;

    createFramebuffer(m_width, m_height);
}

void Renderer::setMaxBounces(uint32_t bounces) {
    m_maxBounces = bounces;
}

void Renderer::setSamples(uint32_t samples) {
    m_samples = samples;
}

GLuint Renderer::getColourTexture() const {
    return m_colourTexture;
}

void Renderer::render(const Camera& camera) {
    glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);
    glViewport(0, 0, m_width, m_height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (m_shaderProgram == 0) return;

    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(m_shaderProgram);

    glUniform2f(m_uLocResolution, (float)m_width, (float)m_height);

    // Upload spheres
    uploadSpheres(m_spheres);

    // Upload uniforms
    glUniform3fv(m_uLocCameraPos, 1, glm::value_ptr(camera.position));
    glUniform3fv(m_uLocCameraForward, 1, glm::value_ptr(camera.forward));
    glUniform3fv(m_uLocCameraRight, 1, glm::value_ptr(camera.right));
    glUniform3fv(m_uLocCameraUp, 1, glm::value_ptr(camera.up));

    glUniform1ui(m_uLocMaxBounces, m_maxBounces);
    glUniform1ui(m_uLocSamples, m_samples);

    // Draw fullscreen quad
    glBindVertexArray(m_VAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::cleanup() {
    if (m_framebuffer != 0) {
        glDeleteFramebuffers(1, &m_framebuffer);
        m_framebuffer = 0;
    }

    if (m_colourTexture != 0) {
        glDeleteTextures(1, &m_colourTexture);
        m_colourTexture = 0;
    }

    glDeleteVertexArrays(1, &m_VAO);
    glDeleteBuffers(1, &m_VBO);
    glDeleteProgram(m_shaderProgram);
}