#version 410 core

// ============================================================================
// PROGRESSIVE ACCUMULATION SHADER
// Implements running average for noise reduction over multiple frames
// ============================================================================

in vec2 vUV;
out vec4 FragColor;

uniform sampler2D uNewSample;    // Current frame's path traced result
uniform sampler2D uAccumulated;  // Previous accumulated result
uniform int uFrame;              // Frame counter (0 = first frame)

void main()
{
    vec3 newColor = texture(uNewSample, vUV).rgb;
    vec3 accumulated = texture(uAccumulated, vUV).rgb;
    
    // Running average: (old * n + new) / (n + 1)
    // This converges to the true integral as n -> infinity
    if (uFrame == 0)
    {
        FragColor = vec4(newColor, 1.0);
    }
    else
    {
        float n = float(uFrame);
        vec3 result = (accumulated * n + newColor) / (n + 1.0);
        FragColor = vec4(result, 1.0);
    }
}

