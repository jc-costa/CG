#version 410 core

// ============================================================================
// DISPLAY SHADER - Tonemapping and Color Grading
// Implements ACES Filmic tonemapping for cinematic output
// ============================================================================

in vec2 vUV;
out vec4 FragColor;

uniform sampler2D uTexture;
uniform float uExposure;
uniform float uGamma;
uniform int uTonemapper;  // 0 = None, 1 = Reinhard, 2 = ACES, 3 = Uncharted2

// ----------------------------------------------------------------------------
// TONEMAPPING OPERATORS
// ----------------------------------------------------------------------------

// Simple Reinhard tonemapping
vec3 tonemapReinhard(vec3 color)
{
    return color / (color + vec3(1.0));
}

// Reinhard with luminance preservation
vec3 tonemapReinhardLuminance(vec3 color)
{
    float luma = dot(color, vec3(0.2126, 0.7152, 0.0722));
    float toneMappedLuma = luma / (1.0 + luma);
    return color * (toneMappedLuma / max(luma, 0.0001));
}

// ACES Filmic tonemapping (industry standard)
// Attempt to match the look of the Academy Color Encoding System
vec3 tonemapACES(vec3 color)
{
    // sRGB => XYZ => D65_2_D60 => AP1 => RRT_SAT
    const mat3 ACESInputMat = mat3(
        0.59719, 0.07600, 0.02840,
        0.35458, 0.90834, 0.13383,
        0.04823, 0.01566, 0.83777
    );
    
    // ODT_SAT => XYZ => D60_2_D65 => sRGB
    const mat3 ACESOutputMat = mat3(
         1.60475, -0.10208, -0.00327,
        -0.53108,  1.10813, -0.07276,
        -0.07367, -0.00605,  1.07602
    );
    
    color = ACESInputMat * color;
    
    // Apply RRT and ODT
    vec3 a = color * (color + 0.0245786) - 0.000090537;
    vec3 b = color * (0.983729 * color + 0.4329510) + 0.238081;
    color = a / b;
    
    color = ACESOutputMat * color;
    
    return clamp(color, 0.0, 1.0);
}

// Uncharted 2 tonemapping (filmic look)
vec3 Uncharted2Tonemap(vec3 x)
{
    float A = 0.15;  // Shoulder strength
    float B = 0.50;  // Linear strength
    float C = 0.10;  // Linear angle
    float D = 0.20;  // Toe strength
    float E = 0.02;  // Toe numerator
    float F = 0.30;  // Toe denominator
    
    return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}

vec3 tonemapUncharted2(vec3 color)
{
    float W = 11.2;  // Linear white point value
    float exposureBias = 2.0;
    
    vec3 curr = Uncharted2Tonemap(exposureBias * color);
    vec3 whiteScale = 1.0 / Uncharted2Tonemap(vec3(W));
    
    return curr * whiteScale;
}

// ----------------------------------------------------------------------------
// POST-PROCESSING
// ----------------------------------------------------------------------------

// Film grain (subtle)
float filmGrain(vec2 uv, float time)
{
    return fract(sin(dot(uv + fract(time * 0.1), vec2(12.9898, 78.233))) * 43758.5453);
}

// Vignette effect
float vignette(vec2 uv, float intensity, float roundness)
{
    vec2 centered = uv * 2.0 - 1.0;
    float dist = length(centered * vec2(1.0, roundness));
    return smoothstep(1.4, 0.5, dist * intensity);
}

// ----------------------------------------------------------------------------
// MAIN
// ----------------------------------------------------------------------------
void main()
{
    vec3 color = texture(uTexture, vUV).rgb;
    
    // Apply exposure
    float exposure = uExposure > 0.0 ? uExposure : 1.0;
    color *= exposure;
    
    // Apply tonemapping
    int tonemap = uTonemapper >= 0 ? uTonemapper : 2;
    
    if (tonemap == 1)
    {
        color = tonemapReinhard(color);
    }
    else if (tonemap == 2)
    {
        color = tonemapACES(color);
    }
    else if (tonemap == 3)
    {
        color = tonemapUncharted2(color);
    }
    
    // Subtle vignette
    color *= vignette(vUV, 0.4, 0.8);
    
    // Gamma correction
    float gamma = uGamma > 0.0 ? uGamma : 2.2;
    color = pow(color, vec3(1.0 / gamma));
    
    // Clamp to valid range
    color = clamp(color, 0.0, 1.0);
    
    FragColor = vec4(color, 1.0);
}

