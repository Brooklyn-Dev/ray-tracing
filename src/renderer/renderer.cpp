#include <fstream>
#include <iostream>
#include <sstream>

#include <glm/gtc/type_ptr.hpp>

#include "camera/camera.hpp"
#include "renderer/renderer.hpp"
#include "utils/io.hpp"
#include "utils/shader.hpp"

Renderer::Renderer(int width, int height)
	: m_width(width), m_height(height) {
	setupShaders();
	setupQuad();
    setupSpheres();
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
    sphereGround.position = glm::vec3(0.0f, -20.5f, 0.0f);
    sphereGround.radius = 20.0f;
    sphereGround.material.colour = glm::vec3(1.0f);
    sphereGround.material.emissionColour = glm::vec3(0.0f);
    sphereGround.material.emissionStrength = 0.0f;
    m_spheres.push_back(sphereGround);

    // Light
    Sphere sphereLight;
    sphereLight.position = glm::vec3(0.0f, 4.0f, -25.0f);
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

void Renderer::render(const Camera& camera) {
    if (m_shaderProgram == 0) return;

    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(m_shaderProgram);

    glUniform2f(m_uLocResolution, (float)m_width, (float)m_height);

    // Upload spheres
    uploadSpheres(m_spheres);

    // Upload camera uniforms
    glUniform3fv(m_uLocCameraPos, 1, glm::value_ptr(camera.position));
    glUniform3fv(m_uLocCameraForward, 1, glm::value_ptr(camera.forward));
    glUniform3fv(m_uLocCameraRight, 1, glm::value_ptr(camera.right));
    glUniform3fv(m_uLocCameraUp, 1, glm::value_ptr(camera.up));

    // Draw fullscreen quad
    glBindVertexArray(m_VAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void Renderer::cleanup() {
    glDeleteVertexArrays(1, &m_VAO);
    glDeleteBuffers(1, &m_VBO);
    glDeleteProgram(m_shaderProgram);
}