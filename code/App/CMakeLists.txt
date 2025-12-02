# ============================================================================
# Cinematic Path Tracer - Build Configuration
# ============================================================================

set(SOURCES
    Source/Main.cpp
    Source/Shader.h
    Source/Shader.cpp
    Source/Renderer.h
    Source/Renderer.cpp
    Source/Math/Vec3.h
    Source/Math/Vec3.cpp
    Source/Math/Ray.h
    Source/Math/Ray.cpp
    Source/Math/Utils.h
    Source/Math/Utils.cpp
    Source/Math/MonteCarlo.h
    Source/Math/MonteCarlo.cpp
    Source/Quadric/Quadric.h
    Source/Quadric/Quadric.cpp
    Source/Quadric/QuadricExample.cpp
    vendor/stb/stb_image.h
    vendor/stb/stb_image.cpp
)

# Add the executable target
add_executable(App ${SOURCES})

target_link_libraries(App glfw)
target_link_libraries(App glad)
target_link_libraries(App glm)

target_include_directories(App PRIVATE vendor/stb)
target_include_directories(App PRIVATE Source)

# Copy all shaders to build directory
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/Shaders/
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Shaders/
)