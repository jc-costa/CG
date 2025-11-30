#version 410 core

// Fullscreen triangle - no vertex buffer needed
// Generates a triangle that covers the entire screen

out vec2 vUV;

void main()
{
    // Generate fullscreen triangle vertices from gl_VertexID
    // This is more efficient than a quad (3 vertices vs 4)
    vUV = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
    gl_Position = vec4(vUV * 2.0 - 1.0, 0.0, 1.0);
}

