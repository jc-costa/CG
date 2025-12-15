// ============================================================================
// PATH TRACER WEB - Real-time Path Tracing with WebGL
// Computer Graphics 2025.2 - Jefferson Costa & Karla Falcão
// ============================================================================

// Global state
let gl;
let canvas;
let pathTraceProgram;
let accumulateProgram;
let displayProgram;
let quadVAO;

let pathTraceFBO;
let accumFBOs = [null, null];
let pathTraceTexture;
let accumTextures = [null, null];

let frameIndex = 0;
let samples = 0;
let lastTime = 0;
let fps = 0;
let resetAccumulation = true;

// Camera state
let camera = {
    position: [0.0, 0.0, 8.0],
    forward: [0.0, 0.0, -1.0],
    up: [0.0, 1.0, 0.0],
    fov: 60.0,
    exposure: 1.0,
    aperture: 0.0,
    focusDistance: 8.0,
    yaw: -Math.PI / 2,
    pitch: 0.0
};

// Render settings
let settings = {
    maxBounces: 8,
    sceneIndex: 0,
    quality: 1.0
};

// Quadrics data (up to 4 quadrics)
let quadrics = [
    // Sphere at position (-2, 0, 0)
    { A: 1.0, B: 1.0, C: 1.0, D: 0.0, E: 0.0, F: 0.0, G: 4.0, H: 0.0, I: 0.0, J: 3.0,
      bboxMin: [-3, -1, -1], bboxMax: [-1, 1, 1], material: 2 },
    // Hyperboloid at (0, 0, 0)
    { A: 1.0, B: -1.0, C: 1.0, D: 0.0, E: 0.0, F: 0.0, G: 0.0, H: 0.0, I: 0.0, J: -0.5,
      bboxMin: [-2, -2, -2], bboxMax: [2, 2, 2], material: 6 },
    // Paraboloid at (2, 0, 0)
    { A: 1.0, B: 1.0, C: 0.0, D: 0.0, E: 0.0, F: 0.0, G: -4.0, H: 0.0, I: -1.0, J: 4.0,
      bboxMin: [1, -2, -2], bboxMax: [3, 2, 0], material: 7 },
    // Cylinder at (0, 0, 0)
    { A: 1.0, B: 0.0, C: 1.0, D: 0.0, E: 0.0, F: 0.0, G: 0.0, H: 0.0, I: 0.0, J: -0.5,
      bboxMin: [-1, -2, -1], bboxMax: [1, 2, 1], material: 3 }
];

let currentQuadric = 0;

// Input state
let keys = {};
let mouseDown = false;
let lastMouseX = 0;
let lastMouseY = 0;

// ============================================================================
// Vertex Shader (simple fullscreen quad)
// ============================================================================
const vertexShaderSource = `#version 300 es
precision highp float;

in vec2 aPosition;
out vec2 vUV;

void main() {
    vUV = aPosition * 0.5 + 0.5;
    gl_Position = vec4(aPosition, 0.0, 1.0);
}
`;

// ============================================================================
// Path Trace Fragment Shader
// ============================================================================
const pathTraceFragmentSource = `#version 300 es
precision highp float;

in vec2 vUV;
out vec4 fragColor;

uniform int uFrame;
uniform int uBounces;
uniform vec2 uResolution;
uniform vec3 uCameraPosition;
uniform vec3 uCameraForward;
uniform vec3 uCameraUp;
uniform float uFOV;
uniform float uTime;
uniform float uAperture;
uniform float uFocusDistance;
uniform int uSceneIndex;

// Quadrics
uniform int uNumQuadrics;
uniform float uQuadrics_A[4];
uniform float uQuadrics_B[4];
uniform float uQuadrics_C[4];
uniform float uQuadrics_D[4];
uniform float uQuadrics_E[4];
uniform float uQuadrics_F[4];
uniform float uQuadrics_G[4];
uniform float uQuadrics_H[4];
uniform float uQuadrics_I[4];
uniform float uQuadrics_J[4];
uniform vec3 uQuadrics_bboxMin[4];
uniform vec3 uQuadrics_bboxMax[4];
uniform int uQuadrics_materialIndex[4];

#define PI 3.14159265359
#define TWO_PI 6.28318530718
#define EPSILON 0.0001
#define MAX_DISTANCE 1000.0

// ============================================================================
// Random Number Generation
// ============================================================================
uint rngState;

uint pcgHash(uint seed) {
    uint state = seed * 747796405u + 2891336453u;
    uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}

float randomFloat() {
    rngState = pcgHash(rngState);
    return float(rngState) / 4294967295.0;
}

vec3 randomOnUnitSphere() {
    float z = randomFloat() * 2.0 - 1.0;
    float a = randomFloat() * TWO_PI;
    float r = sqrt(1.0 - z * z);
    return vec3(r * cos(a), r * sin(a), z);
}

vec3 randomInUnitSphere() {
    vec3 p = randomOnUnitSphere();
    return p * pow(randomFloat(), 1.0 / 3.0);
}

vec3 randomCosineDirection() {
    float r1 = randomFloat();
    float r2 = randomFloat();
    float z = sqrt(1.0 - r2);
    float phi = TWO_PI * r1;
    float sqrtR2 = sqrt(r2);
    return vec3(cos(phi) * sqrtR2, sin(phi) * sqrtR2, z);
}

mat3 createONB(vec3 n) {
    vec3 t, b;
    if (n.z < -0.999999) {
        t = vec3(0.0, -1.0, 0.0);
        b = vec3(-1.0, 0.0, 0.0);
    } else {
        float a = 1.0 / (1.0 + n.z);
        float bb = -n.x * n.y * a;
        t = vec3(1.0 - n.x * n.x * a, bb, -n.x);
        b = vec3(bb, 1.0 - n.y * n.y * a, -n.y);
    }
    return mat3(t, b, n);
}

// ============================================================================
// Ray Structure
// ============================================================================
struct Ray {
    vec3 origin;
    vec3 direction;
};

// ============================================================================
// Material Structure
// ============================================================================
struct Material {
    vec3 albedo;
    vec3 emission;
    float roughness;
    float metallic;
    float ior;
    int type; // 0=diffuse, 1=metal, 2=glass, 3=emission
};

// ============================================================================
// Hit Record
// ============================================================================
struct HitRecord {
    bool hit;
    float t;
    vec3 point;
    vec3 normal;
    Material material;
};

// ============================================================================
// Materials Database
// ============================================================================
Material getMaterial(int idx) {
    if (idx == 0) return Material(vec3(0.9, 0.1, 0.1), vec3(0.0), 0.5, 0.0, 1.5, 0); // Red wall
    if (idx == 1) return Material(vec3(0.1, 0.9, 0.1), vec3(0.0), 0.5, 0.0, 1.5, 0); // Green wall
    if (idx == 2) return Material(vec3(0.95, 0.95, 0.95), vec3(0.0), 0.02, 1.0, 1.5, 1); // Chrome
    if (idx == 3) return Material(vec3(1.0, 1.0, 1.0), vec3(0.0), 0.0, 0.0, 1.5, 2); // Glass
    if (idx == 4) return Material(vec3(0.9, 0.9, 0.9), vec3(0.0), 0.8, 0.0, 1.5, 0); // White diffuse
    if (idx == 5) return Material(vec3(0.0), vec3(15.0, 15.0, 15.0), 0.0, 0.0, 1.5, 3); // Light
    if (idx == 6) return Material(vec3(0.2, 0.5, 0.9), vec3(0.0), 0.3, 0.0, 1.5, 0); // Blue glossy
    if (idx == 7) return Material(vec3(1.0, 0.84, 0.0), vec3(0.0), 0.1, 1.0, 1.5, 1); // Gold
    if (idx == 8) return Material(vec3(0.95, 0.64, 0.54), vec3(0.0), 0.2, 1.0, 1.5, 1); // Copper/Bronze
    return Material(vec3(0.8), vec3(0.0), 0.5, 0.0, 1.5, 0);
}

// ============================================================================
// AABB Intersection
// ============================================================================
bool intersectAABB(Ray ray, vec3 bboxMin, vec3 bboxMax) {
    vec3 invDir = 1.0 / ray.direction;
    vec3 t0 = (bboxMin - ray.origin) * invDir;
    vec3 t1 = (bboxMax - ray.origin) * invDir;
    vec3 tmin = min(t0, t1);
    vec3 tmax = max(t0, t1);
    float tnear = max(max(tmin.x, tmin.y), tmin.z);
    float tfar = min(min(tmax.x, tmax.y), tmax.z);
    return tnear <= tfar && tfar > 0.0;
}

// ============================================================================
// Quadric Intersection
// ============================================================================
bool intersectQuadric(Ray ray, int idx, inout HitRecord rec) {
    // Check bounding box first
    if (!intersectAABB(ray, uQuadrics_bboxMin[idx], uQuadrics_bboxMax[idx])) {
        return false;
    }
    
    float A = uQuadrics_A[idx];
    float B = uQuadrics_B[idx];
    float C = uQuadrics_C[idx];
    float D = uQuadrics_D[idx];
    float E = uQuadrics_E[idx];
    float F = uQuadrics_F[idx];
    float G = uQuadrics_G[idx];
    float H = uQuadrics_H[idx];
    float I = uQuadrics_I[idx];
    float J = uQuadrics_J[idx];
    
    vec3 o = ray.origin;
    vec3 d = ray.direction;
    
    // Quadratic formula coefficients: a*t^2 + b*t + c = 0
    float a = A*d.x*d.x + B*d.y*d.y + C*d.z*d.z + 
              D*d.x*d.y + E*d.x*d.z + F*d.y*d.z;
              
    float b = 2.0*A*o.x*d.x + 2.0*B*o.y*d.y + 2.0*C*o.z*d.z +
              D*(o.x*d.y + o.y*d.x) + E*(o.x*d.z + o.z*d.x) + F*(o.y*d.z + o.z*d.y) +
              G*d.x + H*d.y + I*d.z;
              
    float c = A*o.x*o.x + B*o.y*o.y + C*o.z*o.z +
              D*o.x*o.y + E*o.x*o.z + F*o.y*o.z +
              G*o.x + H*o.y + I*o.z + J;
    
    float discriminant = b*b - 4.0*a*c;
    
    if (discriminant < 0.0) return false;
    
    float sqrtD = sqrt(discriminant);
    float t1 = (-b - sqrtD) / (2.0 * a);
    float t2 = (-b + sqrtD) / (2.0 * a);
    
    float t = t1;
    if (t < EPSILON) t = t2;
    if (t < EPSILON || t > rec.t) return false;
    
    rec.t = t;
    rec.point = ray.origin + t * ray.direction;
    
    // Calculate normal using gradient
    vec3 grad = vec3(
        2.0*A*rec.point.x + D*rec.point.y + E*rec.point.z + G,
        2.0*B*rec.point.y + D*rec.point.x + F*rec.point.z + H,
        2.0*C*rec.point.z + E*rec.point.x + F*rec.point.y + I
    );
    
    rec.normal = normalize(grad);
    rec.material = getMaterial(uQuadrics_materialIndex[idx]);
    rec.hit = true;
    
    return true;
}

// ============================================================================
// Sphere Intersection
// ============================================================================
bool intersectSphere(Ray ray, vec3 center, float radius, Material mat, inout HitRecord rec) {
    vec3 oc = ray.origin - center;
    float a = dot(ray.direction, ray.direction);
    float b = 2.0 * dot(oc, ray.direction);
    float c = dot(oc, oc) - radius * radius;
    float discriminant = b * b - 4.0 * a * c;
    
    if (discriminant < 0.0) return false;
    
    float t = (-b - sqrt(discriminant)) / (2.0 * a);
    if (t < EPSILON || t > rec.t) return false;
    
    rec.t = t;
    rec.point = ray.origin + t * ray.direction;
    rec.normal = normalize(rec.point - center);
    rec.material = mat;
    rec.hit = true;
    return true;
}

// ============================================================================
// Box Intersection
// ============================================================================
bool intersectBox(Ray ray, vec3 bmin, vec3 bmax, Material mat, inout HitRecord rec) {
    vec3 invDir = 1.0 / ray.direction;
    vec3 t0 = (bmin - ray.origin) * invDir;
    vec3 t1 = (bmax - ray.origin) * invDir;
    vec3 tmin = min(t0, t1);
    vec3 tmax = max(t0, t1);
    
    float tnear = max(max(tmin.x, tmin.y), tmin.z);
    float tfar = min(min(tmax.x, tmax.y), tmax.z);
    
    if (tnear > tfar || tfar < EPSILON || tnear > rec.t) return false;
    
    float t = tnear > EPSILON ? tnear : tfar;
    rec.t = t;
    rec.point = ray.origin + t * ray.direction;
    
    // Calculate normal
    vec3 center = (bmin + bmax) * 0.5;
    vec3 p = rec.point - center;
    vec3 d = (bmax - bmin) * 0.5;
    float bias = 1.001;
    
    rec.normal = vec3(0.0);
    if (abs(p.x) > d.x * bias - EPSILON) rec.normal = vec3(sign(p.x), 0.0, 0.0);
    else if (abs(p.y) > d.y * bias - EPSILON) rec.normal = vec3(0.0, sign(p.y), 0.0);
    else rec.normal = vec3(0.0, 0.0, sign(p.z));
    
    rec.material = mat;
    rec.hit = true;
    return true;
}

// ============================================================================
// Scene Intersection
// ============================================================================
HitRecord intersectScene(Ray ray) {
    HitRecord rec;
    rec.hit = false;
    rec.t = MAX_DISTANCE;
    
    // Scene 0: Cornell Box Showcase
    if (uSceneIndex == 0) {
        // Cornell Box walls
        intersectBox(ray, vec3(-5.01, -5.0, -5.0), vec3(-5.0, 5.0, 5.0), getMaterial(0), rec);
        intersectBox(ray, vec3(5.0, -5.0, -5.0), vec3(5.01, 5.0, 5.0), getMaterial(1), rec);
        intersectBox(ray, vec3(-5.0, -5.01, -5.0), vec3(5.0, -5.0, 5.0), getMaterial(4), rec);
        intersectBox(ray, vec3(-5.0, 5.0, -5.0), vec3(5.0, 5.01, 5.0), getMaterial(4), rec);
        intersectBox(ray, vec3(-5.0, -5.0, -5.01), vec3(5.0, 5.0, -5.0), getMaterial(4), rec);
        intersectBox(ray, vec3(-1.5, 4.99, -1.5), vec3(1.5, 5.0, 1.5), getMaterial(5), rec);
        
        // Spheres
        intersectSphere(ray, vec3(-1.0, -2.0, -1.0), 1.0, getMaterial(2), rec); // Chrome
        intersectSphere(ray, vec3(1.5, -2.2, 0.5), 0.8, getMaterial(7), rec); // Gold
        intersectSphere(ray, vec3(0.0, -2.3, 1.5), 0.7, getMaterial(3), rec); // Glass
        intersectSphere(ray, vec3(-2.0, -2.5, 1.5), 0.5, getMaterial(6), rec); // Blue
        intersectSphere(ray, vec3(2.0, -1.5, -1.5), 0.4, getMaterial(5), rec); // Emissive
    }
    // Scene 1: Simple Spheres
    else if (uSceneIndex == 1) {
        intersectSphere(ray, vec3(0.0, 0.0, 0.0), 1.0, getMaterial(0), rec); // Pink
        intersectSphere(ray, vec3(2.5, 0.0, 0.0), 1.0, getMaterial(5), rec); // Orange emissive
        intersectSphere(ray, vec3(0.0, -101.0, 0.0), 100.0, getMaterial(6), rec); // Ground
    }
    // Scene 2: Glass & Metal Study
    else if (uSceneIndex == 2) {
        intersectBox(ray, vec3(-5.01, -5.0, -5.0), vec3(-5.0, 5.0, 5.0), getMaterial(0), rec);
        intersectBox(ray, vec3(5.0, -5.0, -5.0), vec3(5.01, 5.0, 5.0), getMaterial(1), rec);
        intersectBox(ray, vec3(-5.0, -5.01, -5.0), vec3(5.0, -5.0, 5.0), getMaterial(4), rec);
        intersectBox(ray, vec3(-5.0, 5.0, -5.0), vec3(5.0, 5.01, 5.0), getMaterial(4), rec);
        intersectBox(ray, vec3(-5.0, -5.0, -5.01), vec3(5.0, 5.0, -5.0), getMaterial(4), rec);
        intersectBox(ray, vec3(-1.5, 4.99, -1.5), vec3(1.5, 5.0, 1.5), getMaterial(5), rec);
        
        intersectSphere(ray, vec3(0.0, -1.5, 0.0), 1.5, getMaterial(3), rec); // Glass
        intersectSphere(ray, vec3(-2.5, -2.2, 0.5), 0.8, getMaterial(2), rec); // Chrome
        intersectSphere(ray, vec3(2.5, -2.2, 0.5), 0.8, getMaterial(7), rec); // Gold
        intersectSphere(ray, vec3(0.0, -2.5, -2.0), 0.5, getMaterial(5), rec); // Emissive
    }
    // Scene 3: Metals Lineup
    else if (uSceneIndex == 3) {
        intersectBox(ray, vec3(-10.0, -3.01, -10.0), vec3(10.0, -3.0, 10.0), getMaterial(4), rec);
        
        intersectSphere(ray, vec3(-2.0, -2.0, 0.0), 1.0, getMaterial(2), rec); // Chrome
        intersectSphere(ray, vec3(0.0, -2.0, 0.0), 1.0, getMaterial(7), rec); // Gold
        intersectSphere(ray, vec3(2.0, -2.0, 0.0), 1.0, getMaterial(7), rec); // Bronze
        intersectSphere(ray, vec3(0.0, 1.0, 2.0), 0.5, getMaterial(5), rec); // Light
    }
    // Scene 4: Quadric Surfaces
    else if (uSceneIndex == 4) {
        for (int i = 0; i < uNumQuadrics && i < 4; i++) {
            intersectQuadric(ray, i, rec);
        }
        intersectBox(ray, vec3(-10.0, -3.01, -10.0), vec3(10.0, -3.0, 10.0), getMaterial(4), rec);
    }
    
    return rec;
}

// ============================================================================
// Fresnel (Schlick's approximation)
// ============================================================================
float fresnel(vec3 direction, vec3 normal, float ior) {
    float cosTheta = abs(dot(direction, normal));
    float r0 = (1.0 - ior) / (1.0 + ior);
    r0 = r0 * r0;
    return r0 + (1.0 - r0) * pow(1.0 - cosTheta, 5.0);
}

// ============================================================================
// Path Tracing
// ============================================================================
vec3 trace(Ray ray) {
    vec3 color = vec3(0.0);
    vec3 throughput = vec3(1.0);
    
    for (int bounce = 0; bounce < 32; bounce++) {
        if (bounce >= uBounces) break;
        
        HitRecord rec = intersectScene(ray);
        
        if (!rec.hit) {
            // Sky color
            float t = 0.5 * (ray.direction.y + 1.0);
            color += throughput * mix(vec3(1.0), vec3(0.5, 0.7, 1.0), t);
            break;
        }
        
        // Emission
        color += throughput * rec.material.emission;
        
        // Russian roulette
        if (bounce > 3) {
            float p = max(throughput.r, max(throughput.g, throughput.b));
            if (randomFloat() > p) break;
            throughput /= p;
        }
        
        // Material interaction
        if (rec.material.type == 0) { // Diffuse
            mat3 onb = createONB(rec.normal);
            vec3 direction = onb * randomCosineDirection();
            ray.origin = rec.point + rec.normal * EPSILON;
            ray.direction = direction;
            throughput *= rec.material.albedo;
        }
        else if (rec.material.type == 1) { // Metal
            vec3 reflected = reflect(ray.direction, rec.normal);
            vec3 fuzz = rec.material.roughness * randomInUnitSphere();
            ray.origin = rec.point + rec.normal * EPSILON;
            ray.direction = normalize(reflected + fuzz);
            throughput *= rec.material.albedo;
            if (dot(ray.direction, rec.normal) < 0.0) break;
        }
        else if (rec.material.type == 2) { // Glass
            float ior = rec.material.ior;
            bool frontFace = dot(ray.direction, rec.normal) < 0.0;
            vec3 normal = frontFace ? rec.normal : -rec.normal;
            float etai_over_etat = frontFace ? (1.0 / ior) : ior;
            
            float cosTheta = min(dot(-ray.direction, normal), 1.0);
            float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
            bool cannotRefract = etai_over_etat * sinTheta > 1.0;
            
            vec3 direction;
            if (cannotRefract || fresnel(-ray.direction, normal, etai_over_etat) > randomFloat()) {
                direction = reflect(ray.direction, normal);
            } else {
                direction = refract(ray.direction, normal, etai_over_etat);
            }
            
            ray.origin = rec.point + direction * EPSILON;
            ray.direction = direction;
            throughput *= rec.material.albedo;
        }
        else if (rec.material.type == 3) { // Emission
            break;
        }
    }
    
    return color;
}

// ============================================================================
// Main
// ============================================================================
void main() {
    // Initialize RNG
    uint seed = uint(gl_FragCoord.x) * uint(1973) + 
                uint(gl_FragCoord.y) * uint(9277) + 
                uint(uFrame) * uint(26699);
    rngState = pcgHash(seed);
    
    // Calculate ray direction
    vec3 right = normalize(cross(uCameraForward, uCameraUp));
    vec3 up = cross(right, uCameraForward);
    
    float aspectRatio = uResolution.x / uResolution.y;
    float tanHalfFov = tan(radians(uFOV) * 0.5);
    
    vec2 uv = vUV * 2.0 - 1.0;
    uv.x *= aspectRatio;
    
    // Add jitter for antialiasing
    vec2 jitter = (vec2(randomFloat(), randomFloat()) - 0.5) / uResolution;
    uv += jitter;
    
    vec3 rayDir = normalize(
        uCameraForward +
        right * uv.x * tanHalfFov +
        up * uv.y * tanHalfFov
    );
    
    Ray ray;
    ray.origin = uCameraPosition;
    ray.direction = rayDir;
    
    // Depth of field
    if (uAperture > 0.0) {
        vec3 focalPoint = ray.origin + ray.direction * uFocusDistance;
        vec2 random = (vec2(randomFloat(), randomFloat()) - 0.5) * uAperture;
        ray.origin += right * random.x + up * random.y;
        ray.direction = normalize(focalPoint - ray.origin);
    }
    
    vec3 color = trace(ray);
    fragColor = vec4(color, 1.0);
}
`;

// ============================================================================
// Accumulation Fragment Shader
// ============================================================================
const accumulateFragmentSource = `#version 300 es
precision highp float;

in vec2 vUV;
out vec4 fragColor;

uniform sampler2D uCurrentFrame;
uniform sampler2D uAccumulated;
uniform int uFrame;

void main() {
    vec3 current = texture(uCurrentFrame, vUV).rgb;
    vec3 accumulated = texture(uAccumulated, vUV).rgb;
    
    float weight = 1.0 / float(uFrame + 1);
    vec3 result = mix(accumulated, current, weight);
    
    fragColor = vec4(result, 1.0);
}
`;

// ============================================================================
// Display Fragment Shader (tone mapping)
// ============================================================================
const displayFragmentSource = `#version 300 es
precision highp float;

in vec2 vUV;
out vec4 fragColor;

uniform sampler2D uTexture;
uniform float uExposure;

// ACES tone mapping
vec3 ACESFilm(vec3 x) {
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return clamp((x*(a*x+b))/(x*(c*x+d)+e), 0.0, 1.0);
}

void main() {
    vec3 color = texture(uTexture, vUV).rgb;
    
    // Exposure
    color *= uExposure;
    
    // Tone mapping
    color = ACESFilm(color);
    
    // Gamma correction
    color = pow(color, vec3(1.0 / 2.2));
    
    fragColor = vec4(color, 1.0);
}
`;

// ============================================================================
// WebGL Initialization
// ============================================================================
function initWebGL() {
    canvas = document.getElementById('canvas');
    gl = canvas.getContext('webgl2', { 
        alpha: false,
        antialias: false,
        depth: false
    });
    
    if (!gl) {
        alert('WebGL 2 não suportado pelo seu navegador!');
        return false;
    }
    
    // Check for float texture support
    const ext = gl.getExtension('EXT_color_buffer_float');
    if (!ext) {
        console.warn('EXT_color_buffer_float not supported, using RGBA16F');
    }
    
    console.log('WebGL initialized successfully');
    
    // Create shaders
    pathTraceProgram = createProgram(vertexShaderSource, pathTraceFragmentSource);
    console.log('PathTrace program:', pathTraceProgram ? 'OK' : 'FAILED');
    
    accumulateProgram = createProgram(vertexShaderSource, accumulateFragmentSource);
    console.log('Accumulate program:', accumulateProgram ? 'OK' : 'FAILED');
    
    displayProgram = createProgram(vertexShaderSource, displayFragmentSource);
    console.log('Display program:', displayProgram ? 'OK' : 'FAILED');
    
    if (!pathTraceProgram || !accumulateProgram || !displayProgram) {
        alert('Falha ao criar shaders! Verifique o console.');
        return false;
    }
    
    // Create fullscreen quad
    const vertices = new Float32Array([
        -1, -1,
         1, -1,
        -1,  1,
         1,  1
    ]);
    
    quadVAO = gl.createVertexArray();
    gl.bindVertexArray(quadVAO);
    
    const vbo = gl.createBuffer();
    gl.bindBuffer(gl.ARRAY_BUFFER, vbo);
    gl.bufferData(gl.ARRAY_BUFFER, vertices, gl.STATIC_DRAW);
    
    // Get attribute location from shader
    const aPosition = gl.getAttribLocation(pathTraceProgram, 'aPosition');
    console.log('aPosition attribute location:', aPosition);
    
    if (aPosition >= 0) {
        gl.enableVertexAttribArray(aPosition);
        gl.vertexAttribPointer(aPosition, 2, gl.FLOAT, false, 0, 0);
    } else {
        console.error('Could not find aPosition attribute');
    }
    
    gl.bindVertexArray(null);
    console.log('Quad VAO created');
    
    // Create framebuffers and textures
    resizeCanvas();
    
    return true;
}

function createShader(type, source) {
    const shader = gl.createShader(type);
    gl.shaderSource(shader, source);
    gl.compileShader(shader);
    
    if (!gl.getShaderParameter(shader, gl.COMPILE_STATUS)) {
        const info = gl.getShaderInfoLog(shader);
        console.error('Shader compile error:', info);
        console.error('Shader source:', source);
        alert('Erro no shader: ' + info);
        gl.deleteShader(shader);
        return null;
    }
    
    return shader;
}

function createProgram(vertSource, fragSource) {
    const vertShader = createShader(gl.VERTEX_SHADER, vertSource);
    const fragShader = createShader(gl.FRAGMENT_SHADER, fragSource);
    
    if (!vertShader || !fragShader) {
        console.error('Failed to create shaders');
        return null;
    }
    
    const program = gl.createProgram();
    gl.attachShader(program, vertShader);
    gl.attachShader(program, fragShader);
    gl.linkProgram(program);
    
    if (!gl.getProgramParameter(program, gl.LINK_STATUS)) {
        const info = gl.getProgramInfoLog(program);
        console.error('Program link error:', info);
        alert('Erro ao linkar programa: ' + info);
        return null;
    }
    
    return program;
}

function createTexture(width, height) {
    const texture = gl.createTexture();
    gl.bindTexture(gl.TEXTURE_2D, texture);
    
    // Try RGBA32F first, fallback to RGBA16F
    try {
        gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA32F, width, height, 0, gl.RGBA, gl.FLOAT, null);
        const error = gl.getError();
        if (error !== gl.NO_ERROR) {
            console.warn('RGBA32F failed, using RGBA16F');
            gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA16F, width, height, 0, gl.RGBA, gl.FLOAT, null);
        }
    } catch(e) {
        console.warn('Exception creating float texture, using RGBA16F', e);
        gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA16F, width, height, 0, gl.RGBA, gl.FLOAT, null);
    }
    
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
    
    console.log('Texture created:', width, 'x', height);
    return texture;
}

function createFramebuffer(texture) {
    const fbo = gl.createFramebuffer();
    gl.bindFramebuffer(gl.FRAMEBUFFER, fbo);
    gl.framebufferTexture2D(gl.FRAMEBUFFER, gl.COLOR_ATTACHMENT0, gl.TEXTURE_2D, texture, 0);
    
    const status = gl.checkFramebufferStatus(gl.FRAMEBUFFER);
    if (status !== gl.FRAMEBUFFER_COMPLETE) {
        console.error('Framebuffer incomplete:', status);
        const statusNames = {
            [gl.FRAMEBUFFER_INCOMPLETE_ATTACHMENT]: 'INCOMPLETE_ATTACHMENT',
            [gl.FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT]: 'MISSING_ATTACHMENT',
            [gl.FRAMEBUFFER_INCOMPLETE_DIMENSIONS]: 'INCOMPLETE_DIMENSIONS',
            [gl.FRAMEBUFFER_UNSUPPORTED]: 'UNSUPPORTED'
        };
        console.error('Status:', statusNames[status] || 'UNKNOWN');
        return null;
    }
    
    gl.bindFramebuffer(gl.FRAMEBUFFER, null);
    console.log('Framebuffer created successfully');
    return fbo;
}

function resizeCanvas() {
    const width = Math.floor(canvas.width * settings.quality);
    const height = Math.floor(canvas.height * settings.quality);
    
    // Delete old textures and framebuffers
    if (pathTraceTexture) gl.deleteTexture(pathTraceTexture);
    if (accumTextures[0]) gl.deleteTexture(accumTextures[0]);
    if (accumTextures[1]) gl.deleteTexture(accumTextures[1]);
    if (pathTraceFBO) gl.deleteFramebuffer(pathTraceFBO);
    if (accumFBOs[0]) gl.deleteFramebuffer(accumFBOs[0]);
    if (accumFBOs[1]) gl.deleteFramebuffer(accumFBOs[1]);
    
    // Create new textures and framebuffers
    pathTraceTexture = createTexture(width, height);
    accumTextures[0] = createTexture(width, height);
    accumTextures[1] = createTexture(width, height);
    
    pathTraceFBO = createFramebuffer(pathTraceTexture);
    accumFBOs[0] = createFramebuffer(accumTextures[0]);
    accumFBOs[1] = createFramebuffer(accumTextures[1]);
    
    resetAccumulation = true;
}

// ============================================================================
// Camera Update
// ============================================================================
function updateCamera(deltaTime) {
    let moved = false;
    
    const speed = (keys['Shift'] ? 9.0 : 3.0) * deltaTime;
    
    // Calculate right vector
    const right = [
        camera.forward[1] * camera.up[2] - camera.forward[2] * camera.up[1],
        camera.forward[2] * camera.up[0] - camera.forward[0] * camera.up[2],
        camera.forward[0] * camera.up[1] - camera.forward[1] * camera.up[0]
    ];
    
    // Normalize right
    const rightLen = Math.sqrt(right[0]*right[0] + right[1]*right[1] + right[2]*right[2]);
    right[0] /= rightLen; right[1] /= rightLen; right[2] /= rightLen;
    
    // Movement
    if (keys['w'] || keys['W']) {
        camera.position[0] += camera.forward[0] * speed;
        camera.position[1] += camera.forward[1] * speed;
        camera.position[2] += camera.forward[2] * speed;
        moved = true;
    }
    if (keys['s'] || keys['S']) {
        camera.position[0] -= camera.forward[0] * speed;
        camera.position[1] -= camera.forward[1] * speed;
        camera.position[2] -= camera.forward[2] * speed;
        moved = true;
    }
    if (keys['a'] || keys['A']) {
        camera.position[0] -= right[0] * speed;
        camera.position[1] -= right[1] * speed;
        camera.position[2] -= right[2] * speed;
        moved = true;
    }
    if (keys['d'] || keys['D']) {
        camera.position[0] += right[0] * speed;
        camera.position[1] += right[1] * speed;
        camera.position[2] += right[2] * speed;
        moved = true;
    }
    if (keys['q'] || keys['Q']) {
        camera.position[1] -= speed;
        moved = true;
    }
    if (keys['e'] || keys['E']) {
        camera.position[1] += speed;
        moved = true;
    }
    
    if (moved) {
        resetAccumulation = true;
    }
}

function updateCameraRotation() {
    camera.forward = [
        Math.cos(camera.yaw) * Math.cos(camera.pitch),
        Math.sin(camera.pitch),
        Math.sin(camera.yaw) * Math.cos(camera.pitch)
    ];
    
    // Normalize
    const len = Math.sqrt(
        camera.forward[0] * camera.forward[0] +
        camera.forward[1] * camera.forward[1] +
        camera.forward[2] * camera.forward[2]
    );
    camera.forward[0] /= len;
    camera.forward[1] /= len;
    camera.forward[2] /= len;
    
    resetAccumulation = true;
}

// ============================================================================
// Rendering
// ============================================================================
let renderStarted = false;

function render() {
    const currentTime = performance.now() / 1000;
    const deltaTime = currentTime - lastTime;
    lastTime = currentTime;
    
    // Update FPS
    fps = Math.round(1 / deltaTime);
    document.getElementById('fps').textContent = fps;
    
    // Update camera
    updateCamera(deltaTime);
    
    // Reset accumulation if needed
    if (resetAccumulation) {
        samples = 0;
        frameIndex = 0;
        resetAccumulation = false;
        if (!renderStarted) {
            console.log('Starting render loop');
            renderStarted = true;
        }
    }
    
    const width = Math.floor(canvas.width * settings.quality);
    const height = Math.floor(canvas.height * settings.quality);
    
    gl.viewport(0, 0, width, height);
    
    // Check for GL errors
    let error = gl.getError();
    if (error !== gl.NO_ERROR) {
        console.error('GL Error before render:', error);
    }
    
    // Path trace pass
    gl.bindFramebuffer(gl.FRAMEBUFFER, pathTraceFBO);
    gl.useProgram(pathTraceProgram);
    
    const loc = (name) => {
        const location = gl.getUniformLocation(pathTraceProgram, name);
        if (location === null && frameIndex === 0) {
            console.warn('Uniform not found:', name);
        }
        return location;
    };
    
    gl.uniform1i(loc('uFrame'), frameIndex);
    gl.uniform1i(loc('uBounces'), settings.maxBounces);
    gl.uniform2f(loc('uResolution'), width, height);
    gl.uniform3fv(loc('uCameraPosition'), camera.position);
    gl.uniform3fv(loc('uCameraForward'), camera.forward);
    gl.uniform3fv(loc('uCameraUp'), camera.up);
    gl.uniform1f(loc('uFOV'), camera.fov);
    gl.uniform1f(loc('uTime'), currentTime);
    gl.uniform1f(loc('uAperture'), camera.aperture);
    gl.uniform1f(loc('uFocusDistance'), camera.focusDistance);
    gl.uniform1i(loc('uSceneIndex'), settings.sceneIndex);
    
    // Quadrics (only for scene 4)
    const numQuadrics = settings.sceneIndex === 4 ? 4 : 0;
    gl.uniform1i(loc('uNumQuadrics'), numQuadrics);
    
    if (frameIndex === 0 && numQuadrics > 0) {
        console.log('Sending', numQuadrics, 'quadrics to shader');
    }
    
    for (let i = 0; i < 4; i++) {
        gl.uniform1f(loc(`uQuadrics_A[${i}]`), quadrics[i].A);
        gl.uniform1f(loc(`uQuadrics_B[${i}]`), quadrics[i].B);
        gl.uniform1f(loc(`uQuadrics_C[${i}]`), quadrics[i].C);
        gl.uniform1f(loc(`uQuadrics_D[${i}]`), quadrics[i].D);
        gl.uniform1f(loc(`uQuadrics_E[${i}]`), quadrics[i].E);
        gl.uniform1f(loc(`uQuadrics_F[${i}]`), quadrics[i].F);
        gl.uniform1f(loc(`uQuadrics_G[${i}]`), quadrics[i].G);
        gl.uniform1f(loc(`uQuadrics_H[${i}]`), quadrics[i].H);
        gl.uniform1f(loc(`uQuadrics_I[${i}]`), quadrics[i].I);
        gl.uniform1f(loc(`uQuadrics_J[${i}]`), quadrics[i].J);
        gl.uniform3fv(loc(`uQuadrics_bboxMin[${i}]`), quadrics[i].bboxMin);
        gl.uniform3fv(loc(`uQuadrics_bboxMax[${i}]`), quadrics[i].bboxMax);
        gl.uniform1i(loc(`uQuadrics_materialIndex[${i}]`), quadrics[i].material);
    }
    
    gl.bindVertexArray(quadVAO);
    gl.drawArrays(gl.TRIANGLE_STRIP, 0, 4);
    
    // Accumulation pass
    const readIdx = frameIndex % 2;
    const writeIdx = 1 - readIdx;
    
    gl.bindFramebuffer(gl.FRAMEBUFFER, accumFBOs[writeIdx]);
    gl.useProgram(accumulateProgram);
    
    gl.activeTexture(gl.TEXTURE0);
    gl.bindTexture(gl.TEXTURE_2D, pathTraceTexture);
    gl.uniform1i(gl.getUniformLocation(accumulateProgram, 'uCurrentFrame'), 0);
    
    gl.activeTexture(gl.TEXTURE1);
    gl.bindTexture(gl.TEXTURE_2D, accumTextures[readIdx]);
    gl.uniform1i(gl.getUniformLocation(accumulateProgram, 'uAccumulated'), 1);
    
    gl.uniform1i(gl.getUniformLocation(accumulateProgram, 'uFrame'), frameIndex);
    
    gl.bindVertexArray(quadVAO);
    gl.drawArrays(gl.TRIANGLE_STRIP, 0, 4);
    
    // Display pass
    gl.bindFramebuffer(gl.FRAMEBUFFER, null);
    gl.viewport(0, 0, canvas.width, canvas.height);
    gl.useProgram(displayProgram);
    
    gl.activeTexture(gl.TEXTURE0);
    gl.bindTexture(gl.TEXTURE_2D, accumTextures[writeIdx]);
    gl.uniform1i(gl.getUniformLocation(displayProgram, 'uTexture'), 0);
    gl.uniform1f(gl.getUniformLocation(displayProgram, 'uExposure'), camera.exposure);
    
    gl.bindVertexArray(quadVAO);
    gl.drawArrays(gl.TRIANGLE_STRIP, 0, 4);
    
    // Check for GL errors after render
    error = gl.getError();
    if (error !== gl.NO_ERROR && frameIndex < 5) {
        console.error('GL Error after render:', error);
    }
    
    frameIndex++;
    samples = frameIndex;
    document.getElementById('samples').textContent = samples;
    
    requestAnimationFrame(render);
}

// ============================================================================
// Event Handlers
// ============================================================================
function setupEventHandlers() {
    // Keyboard
    window.addEventListener('keydown', (e) => {
        keys[e.key] = true;
        
        if (e.key === 'r' || e.key === 'R') {
            camera.position = [0.0, 0.0, 8.0];
            camera.yaw = -Math.PI / 2;
            camera.pitch = 0.0;
            updateCameraRotation();
        }
    });
    
    window.addEventListener('keyup', (e) => {
        keys[e.key] = false;
    });
    
    // Mouse
    canvas.addEventListener('mousedown', (e) => {
        if (e.button === 2) { // Right click
            mouseDown = true;
            lastMouseX = e.clientX;
            lastMouseY = e.clientY;
            canvas.requestPointerLock();
        }
    });
    
    window.addEventListener('mouseup', () => {
        mouseDown = false;
        document.exitPointerLock();
    });
    
    canvas.addEventListener('mousemove', (e) => {
        if (mouseDown) {
            const dx = e.movementX;
            const dy = e.movementY;
            
            camera.yaw -= dx * 0.002;
            camera.pitch -= dy * 0.002;
            camera.pitch = Math.max(-Math.PI/2 + 0.01, Math.min(Math.PI/2 - 0.01, camera.pitch));
            
            updateCameraRotation();
        }
    });
    
    canvas.addEventListener('contextmenu', (e) => e.preventDefault());
    
    // UI Controls
    document.getElementById('sceneSelect').addEventListener('change', (e) => {
        settings.sceneIndex = parseInt(e.target.value);
        
        // Adjust camera for quadric scene
        if (settings.sceneIndex === 4) {
            camera.position = [0.0, 0.0, 6.0];
            camera.yaw = -Math.PI / 2;
            camera.pitch = 0.0;
            updateCameraRotation();
        } else if (settings.sceneIndex === 1) {
            camera.position = [0.0, 2.0, 5.0];
            camera.yaw = -Math.PI / 2;
            camera.pitch = -0.2;
            updateCameraRotation();
        } else {
            camera.position = [0.0, 0.0, 8.0];
            camera.yaw = -Math.PI / 2;
            camera.pitch = 0.0;
            updateCameraRotation();
        }
        
        resetAccumulation = true;
    });
    
    document.getElementById('resetBtn').addEventListener('click', () => {
        resetAccumulation = true;
    });
    
    document.getElementById('fovSlider').addEventListener('input', (e) => {
        camera.fov = parseFloat(e.target.value);
        document.getElementById('fovValue').textContent = camera.fov + '°';
        resetAccumulation = true;
    });
    
    document.getElementById('exposureSlider').addEventListener('input', (e) => {
        camera.exposure = parseFloat(e.target.value);
        document.getElementById('exposureValue').textContent = camera.exposure.toFixed(1);
    });
    
    document.getElementById('dofSlider').addEventListener('input', (e) => {
        camera.aperture = parseFloat(e.target.value);
        document.getElementById('dofValue').textContent = camera.aperture.toFixed(2);
        resetAccumulation = true;
    });
    
    document.getElementById('bouncesSlider').addEventListener('input', (e) => {
        settings.maxBounces = parseInt(e.target.value);
        document.getElementById('bouncesValue').textContent = settings.maxBounces;
        resetAccumulation = true;
    });
    
    document.getElementById('qualitySelect').addEventListener('change', (e) => {
        settings.quality = parseFloat(e.target.value);
        resizeCanvas();
    });
    
    document.getElementById('quadricSelect').addEventListener('change', (e) => {
        currentQuadric = parseInt(e.target.value);
        loadQuadricToUI();
    });
    
    // Quadric coefficient inputs
    const coeffIds = ['A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J'];
    coeffIds.forEach(id => {
        document.getElementById(`coeff_${id}`).addEventListener('input', () => {
            updateQuadricFromUI();
        });
    });
}

function loadQuadricToUI() {
    const q = quadrics[currentQuadric];
    document.getElementById('coeff_A').value = q.A;
    document.getElementById('coeff_B').value = q.B;
    document.getElementById('coeff_C').value = q.C;
    document.getElementById('coeff_D').value = q.D;
    document.getElementById('coeff_E').value = q.E;
    document.getElementById('coeff_F').value = q.F;
    document.getElementById('coeff_G').value = q.G;
    document.getElementById('coeff_H').value = q.H;
    document.getElementById('coeff_I').value = q.I;
    document.getElementById('coeff_J').value = q.J;
}

function updateQuadricFromUI() {
    const q = quadrics[currentQuadric];
    q.A = parseFloat(document.getElementById('coeff_A').value) || 0;
    q.B = parseFloat(document.getElementById('coeff_B').value) || 0;
    q.C = parseFloat(document.getElementById('coeff_C').value) || 0;
    q.D = parseFloat(document.getElementById('coeff_D').value) || 0;
    q.E = parseFloat(document.getElementById('coeff_E').value) || 0;
    q.F = parseFloat(document.getElementById('coeff_F').value) || 0;
    q.G = parseFloat(document.getElementById('coeff_G').value) || 0;
    q.H = parseFloat(document.getElementById('coeff_H').value) || 0;
    q.I = parseFloat(document.getElementById('coeff_I').value) || 0;
    q.J = parseFloat(document.getElementById('coeff_J').value) || 0;
}

function updateQuadric() {
    updateQuadricFromUI();
    resetAccumulation = true;
}

// Preset functions
function applyPreset(type) {
    const q = quadrics[currentQuadric];
    
    switch(type) {
        case 'sphere':
            q.A=1; q.B=1; q.C=1; q.D=0; q.E=0; q.F=0; q.G=0; q.H=0; q.I=0; q.J=-1;
            break;
        case 'ellipsoid':
            q.A=1; q.B=0.5; q.C=1; q.D=0; q.E=0; q.F=0; q.G=0; q.H=0; q.I=0; q.J=-1;
            break;
        case 'cylinder':
            q.A=1; q.B=0; q.C=1; q.D=0; q.E=0; q.F=0; q.G=0; q.H=0; q.I=0; q.J=-1;
            break;
        case 'cone':
            q.A=1; q.B=-1; q.C=1; q.D=0; q.E=0; q.F=0; q.G=0; q.H=0; q.I=0; q.J=0;
            break;
        case 'hyperboloid':
            q.A=1; q.B=-1; q.C=1; q.D=0; q.E=0; q.F=0; q.G=0; q.H=0; q.I=0; q.J=-1;
            break;
        case 'paraboloid':
            q.A=1; q.B=1; q.C=0; q.D=0; q.E=0; q.F=0; q.G=0; q.H=0; q.I=-1; q.J=0;
            break;
    }
    
    loadQuadricToUI();
    updateQuadric();
}

// ============================================================================
// Main
// ============================================================================
window.addEventListener('load', () => {
    console.log('Page loaded, initializing WebGL...');
    if (initWebGL()) {
        console.log('WebGL initialized successfully, setting up handlers...');
        setupEventHandlers();
        
        // Clear canvas initially
        gl.clearColor(0.1, 0.1, 0.1, 1.0);
        gl.clear(gl.COLOR_BUFFER_BIT);
        
        lastTime = performance.now() / 1000;
        console.log('Starting render loop...');
        requestAnimationFrame(render);
    } else {
        console.error('Failed to initialize WebGL');
    }
});
