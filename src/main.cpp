#define IMGUI_ENABLE_DOCKING

#include <iostream>

#include <glad/glad.h> 
#include <GLFW/glfw3.h>

#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "camera\camera.hpp"
#include "renderer\renderer.hpp"

// === GLOBALS ===
// Window and Rendering State
uint32_t g_windowWidth = 1200;
uint32_t g_windowHeight = 900;
uint32_t g_viewportWidth = 0;
uint32_t g_viewportHeight = 0;
bool g_viewportFocused = false;
bool g_viewportHovered = false;
bool g_needsRender = true;

// Camera and Input
Camera g_camera;
double g_lastX = g_windowWidth / 2.0;
double g_lastY = g_windowHeight / 2.0;
bool g_mouseHeld = false;
float g_deltaTime = 0.0f;
float g_lastFrame = 0.0f;
float g_cameraSpeed = 5.0f;
float g_mouseSensitivity = 0.3f;

// Renderer Instance
Renderer* g_renderer = nullptr;
float g_lastRenderTime = 0.0f;

// ImGui State
bool g_firstFrame = true;

// === INPUT HANDLING ===
void processInput(GLFWwindow* window) {
    if (!g_viewportFocused) return;

    float velocity = g_cameraSpeed * g_deltaTime;
    bool cameraMoved = false;

    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
        g_camera.position += g_camera.forward * velocity;
        cameraMoved = true;
    }
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
        g_camera.position -= g_camera.forward * velocity;
        cameraMoved = true;
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
        g_camera.position -= g_camera.right * velocity;
        cameraMoved = true;
    }
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
        g_camera.position += g_camera.right * velocity;
        cameraMoved = true;
    }
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        g_camera.position += glm::vec3(0.0f, velocity, 0.0f);
        cameraMoved = true;
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
        g_camera.position -= glm::vec3(0.0f, velocity, 0.0f);
        cameraMoved = true;
    }

    if (cameraMoved) g_needsRender = true;
}

void cursorPosCallback(GLFWwindow* window, double x, double y) {
    (void)window;

    if (!g_viewportFocused || !g_mouseHeld) return;

    double xOffset = x - g_lastX;
    double yOffset = y - g_lastY;

    if (xOffset != 0 || yOffset != 0) {
        g_lastX = x;
        g_lastY = y;

        g_camera.yaw += static_cast<float>(xOffset * g_mouseSensitivity);
        g_camera.pitch -= static_cast<float>(yOffset * g_mouseSensitivity);

        if (g_camera.yaw > 360.0f) g_camera.yaw -= 360.0f;
        if (g_camera.yaw < -360.0f) g_camera.yaw += 360.0f;

        if (g_camera.pitch > 89.0f) g_camera.pitch = 89.0f;
        if (g_camera.pitch < -89.0f) g_camera.pitch = -89.0f;

        g_camera.updateOrientation();
        g_needsRender = true;
    }
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    (void)window;
    (void)mods;

    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            if (g_viewportHovered && g_viewportFocused) {
                g_mouseHeld = true;
                glfwGetCursorPos(window, &g_lastX, &g_lastY);
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            }
        }
        else if (action == GLFW_RELEASE) {
            g_mouseHeld = false;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }
}

// === RENDERER UTILITY ===
void performRender()
{
    if (!g_renderer) return;

    double renderStartTime = glfwGetTime();
    g_renderer->render(g_camera);
    g_lastRenderTime = (float)((glfwGetTime() - renderStartTime) * 1000.0);
    g_needsRender = false;
}

// == INITIALISATION FUNCTIONS ===
void initGLFW() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        exit(EXIT_FAILURE);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);
}

GLFWwindow* createGLFWWindow() {
    GLFWwindow* window = glfwCreateWindow(g_windowWidth, g_windowHeight, "Ray Tracer", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glfwMakeContextCurrent(window);
    return window;
}

void initGLAD() {
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialise GLAD" << std::endl;
        exit(EXIT_FAILURE);
    }

    std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
    glViewport(0, 0, g_windowWidth, g_windowHeight);
}

void setupImGui(GLFWwindow* window) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    ImGui::StyleColorsDark();

    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 430");
}

void setupRenderer(uint32_t initialWidth, uint32_t initialHeight) {
    static Renderer rendererInstance(initialWidth, initialHeight);
    g_renderer = &rendererInstance;
    g_renderer->onResize(initialWidth, initialHeight);
    performRender();
}

// === IMGUI UI DRAWING FUNCTIONS ===
void setupImGuiDockingLayout(ImGuiID dockspace_id, ImVec2 viewportSize) {
    if (g_firstFrame) {
        g_firstFrame = false;  // Run only once

        ImGui::DockBuilderRemoveNode(dockspace_id); // Clear any previous layout
        ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace | ImGuiDockNodeFlags_PassthruCentralNode); // Recreate empty node
        ImGui::DockBuilderSetNodeSize(dockspace_id, viewportSize);

        // Split dockspace: 75% viewport, 25% settings
        ImGuiID dock_id_right;
        ImGuiID dock_id_main = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Left, 0.75f, nullptr, &dock_id_right);

        // Dock windows into the created nodes
        ImGui::DockBuilderDockWindow("Viewport", dock_id_main);
        ImGui::DockBuilderDockWindow("Settings", dock_id_right);
        ImGui::DockBuilderFinish(dockspace_id);
    }
}

void renderImGuiMenuBar(GLFWwindow* window) {
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Exit", "Alt+F4"))
                glfwSetWindowShouldClose(window, true);

            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }
}

void renderImGuiSettingsWindow(ImGuiIO& io) {
    ImGui::Begin("Settings");

    // Performance / Debug
    ImGui::Text("Last render: %.3fms", g_lastRenderTime);
    ImGui::Text("Application FPS: %.1f", io.Framerate);
    ImGui::Text("Viewport Size: %dx%d", g_viewportWidth, g_viewportHeight);
    ImGui::Separator();

    // Rendering Parameters
    if (ImGui::CollapsingHeader("Renderer Settings")) {
        ImGui::PushItemWidth(-1); // Sliders fill available width

        ImGui::Text("Samples Per Pixel:");
        static int samples = 1; // Persistent slider state
        if (ImGui::SliderInt("##Samples Per Pixel", &samples, 1, 512)) {
            g_renderer->setSamples(samples);
            g_needsRender = true;
        }

        ImGui::Text("Max Bounces:");
        static int maxBounces = 2; // Persistent slider state
        if (ImGui::SliderInt("##Max Bounces", &maxBounces, 1, 32)) {
            g_renderer->setMaxBounces(maxBounces);
            g_needsRender = true;
        }

        ImGui::PopItemWidth();
    }
    ImGui::Separator();

    // Camera Controls
    if (ImGui::CollapsingHeader("Camera Controls")) {
        // Camera Position
        ImGui::Text("Position:");
        ImGui::Indent();
        ImGui::Text("X: %.2f  Y: %.2f  Z: %.2f", g_camera.position.x, g_camera.position.y, g_camera.position.z);
        ImGui::Unindent();

        // Camera Orientation
        ImGui::Text("Orientation:");
        ImGui::Indent();
        ImGui::Text("Yaw: %.1f  Pitch: %.1f", g_camera.yaw, g_camera.pitch);
        ImGui::Unindent();

        ImGui::PushItemWidth(-1); // Sliders fill available width

        ImGui::Text("Camera Speed:");
        ImGui::SliderFloat("##Camera Speed", &g_cameraSpeed, 0.1f, 10.0f, "%.1f");

        ImGui::Text("Mouse Sensitivity:");
        if (ImGui::SliderFloat("##Mouse Sensitivity", &g_mouseSensitivity, 0.1f, 1.0f, "%.1f"))

        ImGui::PopItemWidth();
    }
    ImGui::Separator();

    ImGui::End();
}

void renderImGuiViewportWindow() {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f)); // No padding for viewport
    ImGui::Begin("Viewport");

    ImVec2 viewportSize = ImGui::GetContentRegionAvail(); // Get available space

    // Resize renderer if viewport dimensions change
    if (g_viewportWidth != static_cast<uint32_t>(viewportSize.x) || g_viewportHeight != static_cast<uint32_t>(viewportSize.y)) {
        g_viewportWidth = static_cast<uint32_t>(viewportSize.x);
        g_viewportHeight = static_cast<uint32_t>(viewportSize.y);

        if (g_renderer)
            g_renderer->onResize(g_viewportWidth, g_viewportHeight);

        g_needsRender = true;
    }

    // Render the scene if needed and dimensions are valid
    if (g_needsRender && g_viewportWidth > 0 && g_viewportHeight > 0)
        performRender();

    // Display the rendered image texture
    if (g_renderer && g_renderer->getColourTexture() && g_viewportWidth > 0 && g_viewportHeight > 0)
        ImGui::Image(static_cast<ImTextureID>(g_renderer->getColourTexture()), viewportSize, ImVec2(0, 1), ImVec2(1, 0));

    g_viewportFocused = ImGui::IsWindowFocused();
    g_viewportHovered = ImGui::IsWindowHovered();

    ImGui::End();
    ImGui::PopStyleVar();
}

// === CLEANUP ===
void cleanup(GLFWwindow* window) {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
}

// === MAIN PROGRAM ===
int main() {
    initGLFW();

    GLFWwindow* window = createGLFWWindow();
    glfwSetCursorPosCallback(window, cursorPosCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);

    initGLAD();

    setupImGui(window);

    setupRenderer(g_windowWidth, g_windowHeight);

    // Main app loop
    while (!glfwWindowShouldClose(window)) {
        // Calculate delta time
        float currentFrame = static_cast<float>(glfwGetTime());
        g_deltaTime = currentFrame - g_lastFrame;
        g_lastFrame = currentFrame;

        glfwPollEvents();
        processInput(window);

        // ImGui Frame Start
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Configure main DockSpace window
        static bool opt_fullscreen = true;
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
        if (opt_fullscreen) {
            const ImGuiViewport* viewport = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(viewport->WorkPos);
            ImGui::SetNextWindowSize(viewport->WorkSize);
            ImGui::SetNextWindowViewport(viewport->ID);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
            window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
            window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
        }

        ImGui::SetNextWindowBgAlpha(0.0f);  // Transparent BG for the main dockspace window
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("DockSpace", nullptr, window_flags);
        ImGui::PopStyleVar();

        if (opt_fullscreen)
            ImGui::PopStyleVar(2);

        // Submit the DockSpace
        ImGuiIO& io = ImGui::GetIO();

        ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
        if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) {
            ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

            setupImGuiDockingLayout(dockspace_id, ImGui::GetMainViewport()->Size);
        }

        renderImGuiMenuBar(window);  // Render menu bar
        ImGui::End();  // End DockSpace window

        // Render individual UI windows
        renderImGuiSettingsWindow(io);
        renderImGuiViewportWindow();

        // Final ImGui render and swap buffers
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Handle ImGui viewports
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            GLFWwindow* backup_current_context = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup_current_context);
        }

        glfwSwapBuffers(window);
    }

    // Cleanup
    cleanup(window);
    return EXIT_SUCCESS;
}