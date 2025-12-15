# ============================================================================
# SCENE MANAGER TEST - CMake Build Configuration
# ============================================================================
#
# PURPOSE:
#   Builds a standalone test executable for the SceneManager class without
#   requiring OpenGL or any graphics dependencies. This enables testing the
#   OBJ/MTL parsing logic in isolation on any system with a C++ compiler.
#
# DESIGN DECISIONS:
#   1. MOCK OPENGL: Instead of linking against real OpenGL libraries, we
#      generate a mock_gl.h header with stub definitions. This allows the
#      SceneManager code to compile without modification while avoiding
#      any actual GPU operations during testing.
#
#   2. FETCHCONTENT FOR GLM: We use CMake's FetchContent to automatically
#      download GLM (OpenGL Mathematics library) instead of requiring it
#      to be pre-installed. This ensures reproducible builds.
#
#   3. ISOLATED BUILD: The test has its own build directory separate from
#      the main application to avoid contaminating the main build.
#
# USAGE:
#   Option 1 - Using test.sh (recommended):
#     ./test.sh           # Build and run tests
#     ./test.sh clean     # Clean build artifacts
#     ./test.sh build     # Build only
#
#   Option 2 - Manual CMake:
#     mkdir build && cd build
#     cmake .. -DCMAKE_BUILD_TYPE=Debug
#     cmake --build . --parallel
#     ./scene_manager_test
#
# REQUIREMENTS:
#   - CMake 3.14+ (for FetchContent)
#   - C++17 compatible compiler
#   - Network access (first build only, to fetch GLM)
#
# BUILD ARTIFACTS:
#   build/
#   ├── scene_manager_test      # Test executable
#   ├── mock_gl.h               # Generated mock OpenGL header
#   ├── test_assets/            # Copied test OBJ/MTL files
#   └── _deps/glm-*/            # Downloaded GLM library
#
# ============================================================================

cmake_minimum_required(VERSION 3.14)
project(SceneManagerTest VERSION 1.0 LANGUAGES CXX)

# C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# Compiler flags
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    add_compile_options(-Wall -Wextra -Wpedantic)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(-g -O0)
    endif()
endif()

# ============================================================================
# DEPENDENCY: GLM (OpenGL Mathematics)
# ============================================================================
# GLM provides the vector and matrix types (glm::vec3, glm::mat4, etc.) used
# throughout the SceneManager for 3D geometry representation.
#
# We use FetchContent to automatically download GLM during configuration.
# This avoids requiring users to manually install GLM and ensures we use
# a known, compatible version.
#
# GLM is header-only in most configurations, but version 1.0+ builds a
# small static library. We link against it regardless.
# ============================================================================
include(FetchContent)

FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.1
    GIT_SHALLOW TRUE  # Don't fetch full history, just the tag
)

FetchContent_MakeAvailable(glm)

# ============================================================================
# MOCK OPENGL HEADER
# ============================================================================
# The SceneManager uses OpenGL types (GLuint, GLint) and functions
# (glGenTextures, glBindTexture, etc.) for GPU upload. However, we only want
# to test the CPU-side parsing logic, not actual GPU operations.
#
# Solution: Generate a mock_gl.h header with:
#   - Type definitions (GLuint, GLint, GLfloat, etc.)
#   - Constant definitions (GL_TEXTURE_2D, GL_RGBA32F, etc.)
#   - Inline stub functions that do nothing
#
# The SceneManager.h conditionally includes this mock header when
# USE_MOCK_GL is defined (see target_compile_definitions below).
# ============================================================================

set(MOCK_GL_HEADER "${CMAKE_CURRENT_BINARY_DIR}/mock_gl.h")
file(WRITE ${MOCK_GL_HEADER} "
// Mock OpenGL types for testing without actual OpenGL
#pragma once
typedef unsigned int GLuint;
typedef int GLint;
typedef int GLsizei;
typedef unsigned int GLenum;
typedef float GLfloat;

// Mock OpenGL constants
#define GL_TEXTURE_2D 0x0DE1
#define GL_RGBA32F 0x8814
#define GL_RGBA 0x1908
#define GL_FLOAT 0x1406
#define GL_NEAREST 0x2600
#define GL_CLAMP_TO_EDGE 0x812F
#define GL_TEXTURE_MIN_FILTER 0x2801
#define GL_TEXTURE_MAG_FILTER 0x2800
#define GL_TEXTURE_WRAP_S 0x2802
#define GL_TEXTURE_WRAP_T 0x2803
#define GL_TEXTURE0 0x84C0
#define GL_TEXTURE2 0x84C2
#define GL_TEXTURE3 0x84C3
#define GL_TEXTURE4 0x84C4
#define GL_TEXTURE5 0x84C5

// Mock OpenGL functions (no-ops for testing)
inline void glGenTextures(GLsizei, GLuint*) {}
inline void glDeleteTextures(GLsizei, const GLuint*) {}
inline void glBindTexture(GLenum, GLuint) {}
inline void glTexImage2D(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const void*) {}
inline void glTexParameteri(GLenum, GLenum, GLint) {}
inline void glActiveTexture(GLenum) {}
inline void glUniform1i(GLint, GLint) {}
inline GLint glGetUniformLocation(GLuint, const char*) { return 0; }
")

# ============================================================================
# Source Files
# ============================================================================

# Create a modified SceneManager source that uses our mock GL header
set(SCENEMANAGER_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/../SceneManager.cpp")
set(SCENEMANAGER_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/../SceneManager.h")
set(FILEMANAGER_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/../FileManager.cpp")
set(FILEMANAGER_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/../FileManager.h")

# Test executable
add_executable(scene_manager_test
    SceneManagerTest.cpp
    ${SCENEMANAGER_SOURCE}
    ${FILEMANAGER_SOURCE}
)

# Include directories
target_include_directories(scene_manager_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}
)

# Link GLM
target_link_libraries(scene_manager_test PRIVATE glm::glm)

# Define macro to use mock GL instead of real glad
target_compile_definitions(scene_manager_test PRIVATE
    USE_MOCK_GL=1
)

# Copy test assets to build directory
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/test_assets
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# ============================================================================
# Custom Target for Running Tests
# ============================================================================

add_custom_target(run_tests
    COMMAND scene_manager_test
    DEPENDS scene_manager_test
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Running SceneManager tests..."
)

