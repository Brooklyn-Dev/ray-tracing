#include <fstream>
#include <iostream>
#include <sstream>

#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "camera/camera.hpp"
#include "renderer/renderer.hpp"
#include "scene/scene.hpp"
#include "utils/io.hpp"
#include "utils/shader.hpp"

Renderer::Renderer(uint32_t width, uint32_t height)
	: m_width(width), m_height(height) {
	setupShaders();
	setupQuad();

    createTexturesAndFBO(width, height);
    resetFrame();
}

Renderer::~Renderer() {
	cleanup();
}

void Renderer::setupShaders() {
    m_shaderProgram = createShaderProgram("shaders/vertex.glsl", "shaders/fragment.glsl");
    if (m_shaderProgram == 0)
        std::cerr << "Failed to create shader program" << std::endl;

    // Cache shader uniform locations
    glUseProgram(m_shaderProgram);
    m_uLocResolution = glGetUniformLocation(m_shaderProgram, "uResolution");
    m_uLocCameraPos = glGetUniformLocation(m_shaderProgram, "uCameraPosition");
    m_uLocCameraForward = glGetUniformLocation(m_shaderProgram, "uCameraForward");
    m_uLocCameraRight = glGetUniformLocation(m_shaderProgram, "uCameraRight");
    m_uLocCameraUp = glGetUniformLocation(m_shaderProgram, "uCameraUp");
    m_uLocGamma = glGetUniformLocation(m_shaderProgram, "uGamma");
    m_uLocMaxBounces = glGetUniformLocation(m_shaderProgram, "uMaxBounces");
    m_uLocSamplesPerPixel = glGetUniformLocation(m_shaderProgram, "uSamplesPerPixel");
    m_uLocFrame = glGetUniformLocation(m_shaderProgram, "uFrame");
    m_uLocSkyboxTexture = glGetUniformLocation(m_shaderProgram, "uSkyboxTexture");
    m_uLocHasSkybox = glGetUniformLocation(m_shaderProgram, "uHasSkybox");
    m_uLocSkyboxExposure = glGetUniformLocation(m_shaderProgram, "uSkyboxExposure");
    m_uLocSunDirection = glGetUniformLocation(m_shaderProgram, "uSunDirection");
    m_uLocSunColour = glGetUniformLocation(m_shaderProgram, "uSunColour");
    m_uLocSunIntensity = glGetUniformLocation(m_shaderProgram, "uSunIntensity");
    m_uLocSunFocus = glGetUniformLocation(m_shaderProgram, "uSunFocus");
    m_uLocNumSpheres = glGetUniformLocation(m_shaderProgram, "uNumSpheres");
    m_uLocNumPlanes = glGetUniformLocation(m_shaderProgram, "uNumPlanes");
    m_uLocNumQuads = glGetUniformLocation(m_shaderProgram, "uNumQuads");
    glUseProgram(0);
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

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Renderer::setupSpheres() {
    // Initialise sphere buffer
    if (m_sphereSSBO == 0)
        glGenBuffers(1, &m_sphereSSBO);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_sphereSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, m_spheres.size() * sizeof(Sphere), nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_sphereSSBO);  // binding = 0
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void Renderer::uploadSpheres(const std::vector<Sphere>& spheres) {
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_sphereSSBO);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, spheres.size() * sizeof(Sphere), spheres.data());
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    glUniform1i(m_uLocNumSpheres, (GLint)spheres.size());
}

void Renderer::setupPlanes() {
    // Initialise plane buffer
    if (m_planeSSBO == 0)
        glGenBuffers(1, &m_planeSSBO);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_planeSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, m_planes.size() * sizeof(Plane), nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_planeSSBO);  // binding = 1
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void Renderer::uploadPlanes(const std::vector<Plane>& planes) {
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_planeSSBO);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, planes.size() * sizeof(Plane), planes.data());
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    glUniform1i(m_uLocNumPlanes, (GLint)planes.size());
}

void Renderer::setupQuads() {
    // Initialise quad buffer
    if (m_quadSSBO == 0)
        glGenBuffers(1, &m_quadSSBO);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_quadSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, m_quads.size() * sizeof(Quad), nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, m_quadSSBO);  // binding = 2
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void Renderer::uploadQuads(const std::vector<Quad>& quads) {
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_quadSSBO);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, quads.size() * sizeof(Quad), quads.data());
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    glUniform1i(m_uLocNumQuads, (GLint)quads.size());
}

void Renderer::createTexturesAndFBO(uint32_t width, uint32_t height) {
    // Cleanup existing resources
    if (m_fbo != 0) glDeleteFramebuffers(1, &m_fbo);
    if (m_accumulatedImage != 0) glDeleteTextures(1, &m_accumulatedImage);
    if (m_displayTexture != 0) glDeleteTextures(1, &m_displayTexture);

    // Create the single FBO
    glGenFramebuffers(1, &m_fbo);

    // Create accumulated image (GL_RGBA32F for precision)
    glGenTextures(1, &m_accumulatedImage);
    glBindTexture(GL_TEXTURE_2D, m_accumulatedImage);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Create display texture (GL_RGBA8 for standard display), target for FBO
    glGenTextures(1, &m_displayTexture);
    glBindTexture(GL_TEXTURE_2D, m_displayTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Renderer::onResize(uint32_t width, uint32_t height) {
    if (width == 0 || height == 0 || (width == m_width && height == m_height))
        return;

    m_width = width;
    m_height = height;

    createTexturesAndFBO(m_width, m_height);
    resetFrame();
}

void Renderer::setGamma(float gamma) {
    if (m_gamma != gamma) {
        m_gamma = gamma;
        resetFrame();
    }
}

void Renderer::setMaxBounces(uint32_t bounces) {
    if (m_maxBounces != bounces) {
        m_maxBounces = bounces;
        resetFrame();
    }
}

void Renderer::setSamplesPerPixel(uint32_t samples) {
    if (m_samplesPerPixel != samples) {
        m_samplesPerPixel = samples;
        resetFrame();
    }
}

void Renderer::setSkybox(const std::string& filepath) {
    if (filepath == "") {
        m_skybox.cleanup();
        resetFrame();
        return;
    }

    m_skybox.load(filepath);
    resetFrame();
}

void Renderer::setSkyboxExposure(float exposure) {
    if (m_skyboxExposure != exposure) {
        m_skyboxExposure = exposure;
        resetFrame();
    }
}

void Renderer::setSunDirection(glm::vec3 direction) {
    if (m_sunDirection != direction) {
        m_sunDirection = direction;
        resetFrame();
    }
}

void Renderer::setSunColour(glm::vec3 colour) {
    if (m_sunColour != colour) {
        m_sunColour = colour;
        resetFrame();
    }
}

void Renderer::setSunIntensity(float intensity) {
    if (m_sunIntensity != intensity) {
        m_sunIntensity = intensity;
        resetFrame();
    }
}

void Renderer::setSunFocus(float focus) {
    if (m_sunFocus != focus) {
        m_sunFocus = focus;
        resetFrame();
    }
}

GLuint Renderer::getDisplayTexture() const {
    return m_displayTexture;
}

uint32_t Renderer::getFrame() const {
    return m_frame;
}

void Renderer::render(const Camera& camera) {
    // Reset accumulation if camera moved
    static glm::vec3 lastCamPos = camera.position;
    static glm::vec3 lastCamForward = camera.forward;
    static glm::vec3 lastCamRight = camera.right;
    static glm::vec3 lastCamUp = camera.up;

    if (lastCamPos != camera.position || lastCamForward != camera.forward ||
        lastCamRight != camera.right || lastCamUp != camera.up) {
        resetFrame();
        lastCamPos = camera.position;
        lastCamForward = camera.forward;
        lastCamRight = camera.right;
        lastCamUp = camera.up;
    }

    // Single Pass: Ray Trace & Accumulate
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_displayTexture, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "ERROR::FRAMEBUFFER:: Combined Pass FBO is not complete!" << std::endl;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return;
    }

    glViewport(0, 0, m_width, m_height);

    glUseProgram(m_shaderProgram);

    // Upload shader uniforms
    glUniform2f(m_uLocResolution, (float)m_width, (float)m_height);
    uploadSpheres(m_spheres);
    uploadPlanes(m_planes);
    uploadQuads(m_quads);
    glUniform3fv(m_uLocCameraPos, 1, glm::value_ptr(camera.position));
    glUniform3fv(m_uLocCameraForward, 1, glm::value_ptr(camera.forward));
    glUniform3fv(m_uLocCameraRight, 1, glm::value_ptr(camera.right));
    glUniform3fv(m_uLocCameraUp, 1, glm::value_ptr(camera.up));
    glUniform3fv(m_uLocSunDirection, 1, glm::value_ptr(m_sunDirection));
    glUniform3fv(m_uLocSunColour, 1, glm::value_ptr(m_sunColour));
    glUniform1f(m_uLocSunIntensity, m_sunIntensity);
    glUniform1f(m_uLocSunFocus, m_sunFocus);
    glUniform1f(m_uLocGamma, m_gamma);
    glUniform1ui(m_uLocMaxBounces, m_maxBounces);
    glUniform1ui(m_uLocSamplesPerPixel, m_samplesPerPixel);
    glUniform1ui(m_uLocFrame, m_frame);

    // Skybox
    if (m_skybox.getTextureID() != 0) {
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, m_skybox.getTextureID());
        glUniform1i(m_uLocSkyboxTexture, 1);
        glUniform1i(m_uLocHasSkybox, 1);
        glUniform1f(m_uLocSkyboxExposure, m_skyboxExposure);
    }
    else {
        glUniform1i(m_uLocHasSkybox, 0);
    }

    // Bind accumulated image
    glBindImageTexture(0, m_accumulatedImage, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

    // Draw fullscreen quad
    glBindVertexArray(m_VAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
    glUseProgram(0);

    // Unbind image texture
    glBindImageTexture(0, 0, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

    // Unbind FBO
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    m_frame++;
}

void Renderer::loadScene(const Scene& scene) {
    m_spheres = scene.spheres;
    setupSpheres();
    m_planes = scene.planes;
    setupPlanes();
    m_quads = scene.quads;
    setupQuads();

    setGamma(scene.gamma);
    setMaxBounces(scene.maxBounces);
    setSamplesPerPixel(scene.samplesPerPixel);

    setSkybox(scene.skyboxPath);
    setSkyboxExposure(scene.getSkyboxExposure());

    setSunDirection(scene.getSunDirection());
    setSunColour(scene.sunColour);
    setSunIntensity(scene.sunIntensity);
    setSunFocus(scene.sunFocus);

    resetFrame();
}

void Renderer::saveRenderedImage(const std::string& filepath, int textureWidth, int textureHeight) {
    if (m_displayTexture == 0 || textureWidth <= 0 || textureHeight <= 0) {
        std::cerr << "Error: Invalid texture ID or dimensions for saving image." << std::endl;
        return;
    }

    GLuint fbo;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_displayTexture, 0);

    size_t bufferSize = static_cast<size_t>(textureWidth) * textureHeight * 4;
    std::vector<unsigned char> pixels(bufferSize);

    glReadPixels(0, 0, textureWidth, textureHeight, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &fbo);

    std::vector<unsigned char> flippedPixels(bufferSize);
    int rowSize = textureWidth * 4;

    for (int y = 0; y < textureHeight; ++y) {
        memcpy(&flippedPixels[y * rowSize], &pixels[(textureHeight - 1 - y) * rowSize], rowSize);
    }

    int result = stbi_write_png(filepath.c_str(), textureWidth, textureHeight, 4, flippedPixels.data(), rowSize);
    if (result)
        std::cout << "Image saved successfully to " << filepath << std::endl;
    else
        std::cerr << "Error: Failed to save image to " << filepath << std::endl;
}

void Renderer::resetFrame() {
    m_frame = 1;
    // Clear accumulation texture
    glBindTexture(GL_TEXTURE_2D, m_accumulatedImage);
    glClearTexImage(m_accumulatedImage, 0, GL_RGBA, GL_FLOAT, nullptr);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Renderer::cleanup() {
    if (m_fbo != 0) {
        glDeleteFramebuffers(1, &m_fbo);
        m_fbo = 0;
    }
    if (m_accumulatedImage != 0) {
        glDeleteTextures(1, &m_accumulatedImage);
        m_accumulatedImage = 0;
    }
    if (m_displayTexture != 0) {
        glDeleteTextures(1, &m_displayTexture);
        m_displayTexture = 0;
    }
    if (m_shaderProgram != 0) {
        glDeleteProgram(m_shaderProgram);
        m_shaderProgram = 0;
    }
    if (m_VAO != 0) {
        glDeleteVertexArrays(1, &m_VAO);
        m_VAO = 0;
    }
    if (m_VBO != 0) {
        glDeleteBuffers(1, &m_VBO);
        m_VBO = 0;
    }
    if (m_sphereSSBO != 0) {
        glDeleteBuffers(1, &m_sphereSSBO);
        m_sphereSSBO = 0;
    }
    if (m_skybox.getTextureID() != 0) {
        m_skybox.cleanup();
    }
}