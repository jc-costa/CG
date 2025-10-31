#version 410 core

in vec2 aVertexPosition;

out vec2 vTexCoord;

void main()
{
    gl_Position = vec4(aVertexPosition, 0.0, 1.0);
    vTexCoord = aVertexPosition * 0.5 + 0.5;
}

