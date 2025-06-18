cmake_minimum_required (VERSION 3.16)
project (ray-tracing VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build configuration
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler-specific options
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -pedantic)
endif()

# Create GLAD library
add_library(glad STATIC external/glad/src/glad.c)
target_include_directories(glad PUBLIC external/glad/include)

# Configure GLFW
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(external/glfw)

# Configure GLM
add_library(glm INTERFACE)
target_include_directories(glm INTERFACE external/glm)

# Find OpenGL
find_package(OpenGL REQUIRED)

# Configure ImGui
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/external/imgui)

add_library(imgui STATIC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

target_include_directories(imgui PUBLIC
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)

target_link_libraries(imgui PUBLIC
    glfw
    OpenGL::GL
)

# Add ImGuiFileDialog
set(IMGUIFILEDIALOG_DIR ${CMAKE_SOURCE_DIR}/external/imguifiledialog)

add_library(ImGuiFileDialog STATIC
    ${IMGUIFILEDIALOG_DIR}/ImGuiFileDialog.cpp
)

target_include_directories(ImGuiFileDialog PUBLIC
    ${IMGUIFILEDIALOG_DIR}
    ${IMGUI_DIR}
)

target_link_libraries(ImGuiFileDialog PUBLIC
    imgui
)

# Create main executable
file(GLOB_RECURSE SOURCES src/*.cpp)
add_executable(${PROJECT_NAME} ${APP_TYPE} ${SOURCES})

if (WIN32 AND MSVC)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE "$<$<CONFIG:Release>:TRUE>$<$<CONFIG:Debug>:FALSE>"
    )
    target_link_options(${PROJECT_NAME} PRIVATE
        "$<$<CONFIG:Release>:/ENTRY:mainCRTStartup>"
    )
endif()

# Link libraries
target_link_libraries(${PROJECT_NAME} 
    PRIVATE 
        glad
        glfw
        glm
        OpenGL::GL
        imgui
        ImGuiFileDialog
)

# Include directories
target_include_directories(${PROJECT_NAME} 
    PRIVATE 
        ${CMAKE_SOURCE_DIR}/include
        external/glad/include
        external/glfw/include
        external/glm
        external/imgui
        external/imgui/backends
        external/stb
        external/imfiledialog
        external/json
)

# Set output directory
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Copy shaders to output directory
add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/shaders
            $<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders
)

# Copy skyboxes to output directory
add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/skyboxes
            $<TARGET_FILE_DIR:${PROJECT_NAME}>/skyboxes
)

# Copy scenes to output directory
add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/scenes
            $<TARGET_FILE_DIR:${PROJECT_NAME}>/scenes
)

# Create exports directory
add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
            $<TARGET_FILE_DIR:${PROJECT_NAME}>/exports
)