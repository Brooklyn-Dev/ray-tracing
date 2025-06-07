#include <iostream>

#include "camera\camera.hpp"
#include "renderer\renderer.hpp"
#include "utils\io.hpp"

// === CONSTANTS ===
const int WINDOW_WIDTH = 1200;
const int WINDOW_HEIGHT = 900;
const float MOUSE_SENSITIVITY = 0.3f;
const float CAMERA_SPEED = 5.0f;

// === GLOBALS ===
Camera g_camera;
double g_lastX = WINDOW_WIDTH / 2;
double g_lastY = WINDOW_HEIGHT / 2;
bool g_mouseHeld = false;
float g_deltaTime = 0.0f;
float g_lastFrame = 0.0f;

// === INPUT HANDLING ===
void processInput(GLFWwindow* window) {
    float velocity = CAMERA_SPEED * g_deltaTime;

    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        g_camera.position += g_camera.forward * velocity;
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
        g_camera.position -= g_camera.forward * velocity;
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
        g_camera.position -= g_camera.right * velocity;
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
        g_camera.position += g_camera.right * velocity;
}

void cursorPosCallback(GLFWwindow* window, double x, double y) {
    if (!g_mouseHeld) return;

    double xOffset = x - g_lastX;
    double yOffset = y - g_lastY;
    g_lastX = x;
    g_lastY = y;

    g_camera.yaw += xOffset * MOUSE_SENSITIVITY;
    g_camera.pitch -= yOffset * MOUSE_SENSITIVITY;

    if (g_camera.pitch > 89.0f) g_camera.pitch = 89.0f;
    if (g_camera.pitch < -89.0f) g_camera.pitch = -89.0f;

    g_camera.updateOrientation();
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            g_mouseHeld = true;
            glfwGetCursorPos(window, &g_lastX, &g_lastY);
        }
        else if (action == GLFW_RELEASE) {
            g_mouseHeld = false;
        }
    }
}

// === MAIN PROGRAM ===
int main() {
    // Init GLFW and OpenGL context
    if (!glfwInit()) {
        std::cerr << "Failed to initalise GLFW" << std::endl;
        return -1;
    }

    // Configure OpenGL context
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    // Create window
    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "ray-tracing", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window\n" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetCursorPosCallback(window, cursorPosCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);

    // Load OpenGL functions
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cerr << "Failed to initialise GLAD" << std::endl;
        return -1;
    }

    std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

    // Create renderer
    Renderer renderer(WINDOW_WIDTH, WINDOW_HEIGHT);

    double lastTime = glfwGetTime();
    int frames = 0;

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        // Calculate delta time
        float currentFrame = static_cast<float>(glfwGetTime());
        g_deltaTime = currentFrame - g_lastFrame;
        g_lastFrame = currentFrame;

        // Handle input
        processInput(window);

        // Render
        renderer.render(g_camera);

        frames++;
        double currentTime = glfwGetTime();
        if (currentTime - lastTime >= 1.0) { // Every second
            std::cout << "FPS: " << frames << std::endl;
            frames = 0;
            lastTime = currentTime;
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}