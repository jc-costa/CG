#version 410 core

// ============================================================================
// CINEMATIC PATH TRACER - Production Quality GPU Implementation
// Inspired by Pixar's RenderMan and modern PBR techniques
// ============================================================================

in vec2 vUV;
out vec4 FragColor;

// ----------------------------------------------------------------------------
// UNIFORMS
// ----------------------------------------------------------------------------
uniform int uFrame;
uniform int uBounces;
uniform vec2 uResolution;
uniform vec3 uCameraPosition;
uniform mat4 uInverseProjection;
uniform mat4 uInverseView;
uniform float uTime;
uniform float uFocalLength;
uniform float uAperture;
uniform float uFocusDistance;

// Quadrics from C++
uniform int uNumQuadrics;

// Quadric coefficients as separate arrays (GLSL 410 compatibility)
uniform float uQuadrics_A[8];
uniform float uQuadrics_B[8];
uniform float uQuadrics_C[8];
uniform float uQuadrics_D[8];
uniform float uQuadrics_E[8];
uniform float uQuadrics_F[8];
uniform float uQuadrics_G[8];
uniform float uQuadrics_H[8];
uniform float uQuadrics_I[8];
uniform float uQuadrics_J[8];
uniform vec3 uQuadrics_bboxMin[8];
uniform vec3 uQuadrics_bboxMax[8];
uniform int uQuadrics_materialIndex[8];

// ----------------------------------------------------------------------------
// CONSTANTS
// ----------------------------------------------------------------------------
#define PI 3.14159265359
#define TWO_PI 6.28318530718
#define INV_PI 0.31830988618
#define INV_TWO_PI 0.15915494309
#define EPSILON 0.0001
#define MAX_DISTANCE 1000.0

// ----------------------------------------------------------------------------
// RANDOM NUMBER GENERATION - PCG Hash (High Quality)
// Based on "Hash Functions for GPU Rendering" - Jarzynski & Olano
// ----------------------------------------------------------------------------
uint rngState;

uint pcgHash(uint seed)
{
    uint state = seed * 747796405u + 2891336453u;
    uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}

float randomFloat()
{
    rngState = pcgHash(rngState);
    return float(rngState) / 4294967295.0;
}

vec2 randomVec2()
{
    return vec2(randomFloat(), randomFloat());
}

vec3 randomVec3()
{
    return vec3(randomFloat(), randomFloat(), randomFloat());
}

// ----------------------------------------------------------------------------
// MONTE CARLO SAMPLING - Importance Sampling Functions
// Translated from Math/MonteCarlo.cpp
// ----------------------------------------------------------------------------

// Uniform sampling on unit sphere
vec3 randomOnUnitSphere()
{
    float z = randomFloat() * 2.0 - 1.0;
    float a = randomFloat() * TWO_PI;
    float r = sqrt(1.0 - z * z);
    return vec3(r * cos(a), r * sin(a), z);
}

// Uniform sampling in unit sphere
vec3 randomInUnitSphere()
{
    vec3 p = randomOnUnitSphere();
    return p * pow(randomFloat(), 1.0 / 3.0);
}

// Cosine-weighted hemisphere sampling (optimal for Lambertian BRDF)
// PDF = cos(theta) / PI
vec3 randomCosineDirection()
{
    float r1 = randomFloat();
    float r2 = randomFloat();
    float z = sqrt(1.0 - r2);
    
    float phi = TWO_PI * r1;
    float sqrtR2 = sqrt(r2);
    float x = cos(phi) * sqrtR2;
    float y = sin(phi) * sqrtR2;
    
    return vec3(x, y, z);
}

// Create orthonormal basis from normal (Frisvad's method - fast, robust)
mat3 createONB(vec3 n)
{
    vec3 t, b;
    if (n.z < -0.999999)
    {
        t = vec3(0.0, -1.0, 0.0);
        b = vec3(-1.0, 0.0, 0.0);
    }
    else
    {
        float a = 1.0 / (1.0 + n.z);
        float bb = -n.x * n.y * a;
        t = vec3(1.0 - n.x * n.x * a, bb, -n.x);
        b = vec3(bb, 1.0 - n.y * n.y * a, -n.y);
    }
    return mat3(t, b, n);
}

// Cosine-weighted direction in hemisphere around normal
vec3 randomCosineDirectionInHemisphere(vec3 normal)
{
    vec3 localDir = randomCosineDirection();
    return createONB(normal) * localDir;
}

// GGX/Trowbridge-Reitz importance sampling
vec3 sampleGGX(vec3 N, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    
    float r1 = randomFloat();
    float r2 = randomFloat();
    
    float phi = TWO_PI * r1;
    float cosTheta = sqrt((1.0 - r2) / (1.0 + (a2 - 1.0) * r2));
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
    
    // Spherical to Cartesian (in tangent space)
    vec3 H = vec3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);
    
    // Transform to world space
    return normalize(createONB(N) * H);
}

// Sampling on unit disk (for depth of field)
vec2 randomInUnitDisk()
{
    float r = sqrt(randomFloat());
    float theta = TWO_PI * randomFloat();
    return vec2(r * cos(theta), r * sin(theta));
}

// ----------------------------------------------------------------------------
// MATERIAL SYSTEM - PBR Disney-inspired
// ----------------------------------------------------------------------------
struct Material
{
    vec3 albedo;           // Base color
    float roughness;       // Surface roughness [0-1]
    float metallic;        // Metalness [0-1]
    vec3 emission;         // Emissive color
    float emissionStrength;// Emission intensity
    float ior;             // Index of refraction (glass)
    float transmission;    // Transmission amount (glass)
    float subsurface;      // Subsurface scattering amount
};

Material createMaterial(vec3 albedo, float roughness, float metallic)
{
    Material m;
    m.albedo = albedo;
    m.roughness = max(roughness, 0.04); // Prevent perfect mirror (numerical issues)
    m.metallic = metallic;
    m.emission = vec3(0.0);
    m.emissionStrength = 0.0;
    m.ior = 1.5;
    m.transmission = 0.0;
    m.subsurface = 0.0;
    return m;
}

Material createEmissive(vec3 color, float strength)
{
    Material m = createMaterial(color, 1.0, 0.0);
    m.emission = color;
    m.emissionStrength = strength;
    return m;
}

Material createGlass(vec3 tint, float roughness, float ior)
{
    Material m = createMaterial(tint, roughness, 0.0);
    m.ior = ior;
    m.transmission = 1.0;
    return m;
}

// ----------------------------------------------------------------------------
// BRDF - Physically Based Rendering Functions
// ----------------------------------------------------------------------------

// Fresnel-Schlick approximation
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// Fresnel with roughness for environment reflections
vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// GGX/Trowbridge-Reitz Normal Distribution Function
float distributionGGX(float NdotH, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH2 = NdotH * NdotH;
    
    float num = a2;
    float denom = NdotH2 * (a2 - 1.0) + 1.0;
    denom = PI * denom * denom;
    
    return num / max(denom, 0.0001);
}

// Smith's Geometry function (Schlick-GGX)
float geometrySchlickGGX(float NdotV, float roughness)
{
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    
    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    
    return num / max(denom, 0.0001);
}

float geometrySmith(float NdotV, float NdotL, float roughness)
{
    float ggx1 = geometrySchlickGGX(NdotV, roughness);
    float ggx2 = geometrySchlickGGX(NdotL, roughness);
    return ggx1 * ggx2;
}

// Full Cook-Torrance BRDF evaluation
vec3 evaluateBRDF(vec3 V, vec3 L, vec3 N, Material mat)
{
    vec3 H = normalize(V + L);
    
    float NdotV = max(dot(N, V), 0.0001);
    float NdotL = max(dot(N, L), 0.0001);
    float NdotH = max(dot(N, H), 0.0);
    float HdotV = max(dot(H, V), 0.0);
    
    // Base reflectivity (dielectric = 0.04, metal = albedo)
    vec3 F0 = mix(vec3(0.04), mat.albedo, mat.metallic);
    
    // Cook-Torrance specular BRDF
    float D = distributionGGX(NdotH, mat.roughness);
    vec3 F = fresnelSchlick(HdotV, F0);
    float G = geometrySmith(NdotV, NdotL, mat.roughness);
    
    vec3 specular = (D * G * F) / (4.0 * NdotV * NdotL + 0.0001);
    
    // Diffuse BRDF (Lambertian with energy conservation)
    vec3 kS = F;
    vec3 kD = (1.0 - kS) * (1.0 - mat.metallic);
    vec3 diffuse = kD * mat.albedo * INV_PI;
    
    return diffuse + specular;
}

// Sample BRDF direction (importance sampling)
vec3 sampleBRDF(vec3 V, vec3 N, Material mat, out vec3 throughput)
{
    float diffuseWeight = (1.0 - mat.metallic) * 0.5;
    
    vec3 L;
    
    if (randomFloat() < diffuseWeight)
    {
        // Sample diffuse (cosine-weighted)
        L = randomCosineDirectionInHemisphere(N);
        
        // Evaluate full BRDF for throughput
        float NdotL = max(dot(N, L), 0.0);
        vec3 brdf = evaluateBRDF(V, L, N, mat);
        
        // PDF for cosine-weighted sampling = cos(theta) / PI
        float pdf = NdotL * INV_PI;
        
        throughput = brdf * NdotL / max(pdf, 0.0001);
    }
    else
    {
        // Sample specular (GGX)
        vec3 H = sampleGGX(N, mat.roughness);
        L = reflect(-V, H);
        
        if (dot(L, N) <= 0.0)
        {
            throughput = vec3(0.0);
            return L;
        }
        
        float NdotL = max(dot(N, L), 0.0);
        float NdotH = max(dot(N, H), 0.0);
        float HdotV = max(dot(H, V), 0.0);
        
        vec3 brdf = evaluateBRDF(V, L, N, mat);
        
        // GGX PDF = D * NdotH / (4 * HdotV)
        float D = distributionGGX(NdotH, mat.roughness);
        float pdf = D * NdotH / (4.0 * HdotV + 0.0001);
        
        throughput = brdf * NdotL / max(pdf, 0.0001);
    }
    
    return L;
}

// ----------------------------------------------------------------------------
// SCENE GEOMETRY
// ----------------------------------------------------------------------------
struct Sphere
{
    vec3 center;
    float radius;
    int materialIndex;
};

struct Plane
{
    vec3 point;
    vec3 normal;
    int materialIndex;
};

// Quadric surface: Ax^2 + By^2 + Cz^2 + Dxy + Exz + Fyz + Gx + Hy + Iz + J = 0
struct Quadric
{
    float A, B, C;      // x^2, y^2, z^2 coefficients
    float D, E, F;      // xy, xz, yz cross terms
    float G, H, I;      // x, y, z linear terms
    float J;            // constant term
    vec3 bboxMin;       // Bounding box minimum
    vec3 bboxMax;       // Bounding box maximum
    int materialIndex;
};

struct HitRecord
{
    float t;
    vec3 position;
    vec3 normal;
    int materialIndex;
    bool frontFace;
};

// Ray-sphere intersection
bool intersectSphere(vec3 ro, vec3 rd, Sphere sphere, inout HitRecord hit)
{
    vec3 oc = ro - sphere.center;
    float a = dot(rd, rd);
    float halfB = dot(oc, rd);
    float c = dot(oc, oc) - sphere.radius * sphere.radius;
    float discriminant = halfB * halfB - a * c;
    
    if (discriminant < 0.0) return false;
    
    float sqrtD = sqrt(discriminant);
    float t = (-halfB - sqrtD) / a;
    
    if (t < EPSILON || t > hit.t)
    {
        t = (-halfB + sqrtD) / a;
        if (t < EPSILON || t > hit.t) return false;
    }
    
    hit.t = t;
    hit.position = ro + rd * t;
    vec3 outwardNormal = (hit.position - sphere.center) / sphere.radius;
    hit.frontFace = dot(rd, outwardNormal) < 0.0;
    hit.normal = hit.frontFace ? outwardNormal : -outwardNormal;
    hit.materialIndex = sphere.materialIndex;
    
    return true;
}

// Ray-plane intersection
bool intersectPlane(vec3 ro, vec3 rd, Plane plane, inout HitRecord hit)
{
    float denom = dot(plane.normal, rd);
    if (abs(denom) < EPSILON) return false;
    
    float t = dot(plane.point - ro, plane.normal) / denom;
    
    if (t < EPSILON || t > hit.t) return false;
    
    hit.t = t;
    hit.position = ro + rd * t;
    hit.frontFace = denom < 0.0;
    hit.normal = hit.frontFace ? plane.normal : -plane.normal;
    hit.materialIndex = plane.materialIndex;
    
    return true;
}

// Ray-bounding box intersection (AABB)
bool intersectAABB(vec3 ro, vec3 rd, vec3 bboxMin, vec3 bboxMax)
{
    vec3 invDir = 1.0 / rd;
    vec3 t0 = (bboxMin - ro) * invDir;
    vec3 t1 = (bboxMax - ro) * invDir;
    
    vec3 tmin = min(t0, t1);
    vec3 tmax = max(t0, t1);
    
    float tNear = max(max(tmin.x, tmin.y), tmin.z);
    float tFar = min(min(tmax.x, tmax.y), tmax.z);
    
    return tNear <= tFar && tFar >= EPSILON;
}

// Evaluate quadric at point P
// Q(P) = Ax^2 + By^2 + Cz^2 + Dxy + Exz + Fyz + Gx + Hy + Iz + J
float evaluateQuadric(Quadric q, vec3 P)
{
    float x = P.x, y = P.y, z = P.z;
    return q.A * x * x + q.B * y * y + q.C * z * z +
           q.D * x * y + q.E * x * z + q.F * y * z +
           q.G * x + q.H * y + q.I * z + q.J;
}

// Compute gradient (normal) at point P
// ∇Q = (∂Q/∂x, ∂Q/∂y, ∂Q/∂z)
// ∂Q/∂x = 2Ax + Dy + Ez + G
// ∂Q/∂y = 2By + Dx + Fz + H
// ∂Q/∂z = 2Cz + Ex + Fy + I
vec3 quadricGradient(Quadric q, vec3 P)
{
    float x = P.x, y = P.y, z = P.z;
    return vec3(
        2.0 * q.A * x + q.D * y + q.E * z + q.G,
        2.0 * q.B * y + q.D * x + q.F * z + q.H,
        2.0 * q.C * z + q.E * x + q.F * y + q.I
    );
}

// Ray-quadric intersection
// Ray: P(t) = ro + t * rd
// Substitute into quadric equation and solve quadratic: at^2 + bt + c = 0
bool intersectQuadric(vec3 ro, vec3 rd, Quadric q, inout HitRecord hit)
{
    // Quadric: Ax² + By² + Cz² + Dxy + Exz + Fyz + Gx + Hy + Iz + J = 0
    // Ray: P(t) = ro + t*rd
    
    vec3 d = rd;
    vec3 o = ro;
    
    // Coefficient of t²
    float Aq = q.A * d.x * d.x + q.B * d.y * d.y + q.C * d.z * d.z +
               q.D * d.x * d.y + q.E * d.x * d.z + q.F * d.y * d.z;
    
    // Coefficient of t
    float Bq = 2.0 * (q.A * o.x * d.x + q.B * o.y * d.y + q.C * o.z * d.z) +
               q.D * (o.x * d.y + o.y * d.x) +
               q.E * (o.x * d.z + o.z * d.x) +
               q.F * (o.y * d.z + o.z * d.y) +
               q.G * d.x + q.H * d.y + q.I * d.z;
    
    // Constant term
    float Cq = q.A * o.x * o.x + q.B * o.y * o.y + q.C * o.z * o.z +
               q.D * o.x * o.y + q.E * o.x * o.z + q.F * o.y * o.z +
               q.G * o.x + q.H * o.y + q.I * o.z + q.J;
    
    // Solve Aq*t² + Bq*t + Cq = 0
    float discriminant = Bq * Bq - 4.0 * Aq * Cq;
    
    if (discriminant < 0.0) return false;
    
    float sqrtDisc = sqrt(discriminant);
    float t1 = (-Bq - sqrtDisc) / (2.0 * Aq);
    float t2 = (-Bq + sqrtDisc) / (2.0 * Aq);
    
    // Try nearest intersection first
    float t = t1;
    if (t < EPSILON || t >= hit.t)
    {
        t = t2;
        if (t < EPSILON || t >= hit.t)
            return false;
    }
    
    vec3 P = ro + rd * t;
    
    // Compute normal via gradient
    vec3 grad = quadricGradient(q, P);
    float gradLen = length(grad);
    
    if (gradLen < EPSILON) return false;
    
    vec3 normal = grad / gradLen;
    
    // Update hit record
    hit.t = t;
    hit.position = P;
    hit.frontFace = dot(rd, normal) < 0.0;
    hit.normal = hit.frontFace ? normal : -normal;
    hit.materialIndex = q.materialIndex;
    
    return true;
}

// ----------------------------------------------------------------------------
// SCENE DEFINITION - Cornell Box inspired
// ----------------------------------------------------------------------------
#define NUM_MATERIALS 10
#define NUM_SPHERES 6

Material materials[NUM_MATERIALS];
Sphere spheres[NUM_SPHERES];

void initScene()
{
    // Materials
    materials[0] = createMaterial(vec3(0.73, 0.73, 0.73), 0.9, 0.0);   // White diffuse (floor/ceiling)
    materials[1] = createMaterial(vec3(0.65, 0.05, 0.05), 0.9, 0.0);   // Red diffuse (left wall)
    materials[2] = createMaterial(vec3(0.12, 0.45, 0.15), 0.9, 0.0);   // Green diffuse (right wall)
    materials[3] = createMaterial(vec3(0.9, 0.9, 0.9), 0.02, 1.0);     // Chrome metal
    materials[4] = createMaterial(vec3(1.0, 0.78, 0.34), 0.1, 1.0);    // Gold metal
    materials[5] = createEmissive(vec3(1.0, 0.95, 0.85), 15.0);        // Warm area light
    materials[6] = createGlass(vec3(1.0, 1.0, 1.0), 0.0, 1.5);         // Clear glass
    materials[7] = createMaterial(vec3(0.1, 0.3, 0.8), 0.05, 0.0);     // Blue glossy
    materials[8] = createMaterial(vec3(0.95, 0.93, 0.88), 0.4, 0.0);   // Rough white
    materials[9] = createMaterial(vec3(0.85, 0.5, 0.2), 0.3, 0.5);     // Bronze
    
    // Spheres - showcase scene
    // Large chrome sphere
    spheres[0].center = vec3(-1.0, -2.0, -1.0);
    spheres[0].radius = 1.0;
    spheres[0].materialIndex = 3;
    
    // Gold sphere
    spheres[1].center = vec3(1.5, -2.2, 0.5);
    spheres[1].radius = 0.8;
    spheres[1].materialIndex = 4;
    
    // Glass sphere
    spheres[2].center = vec3(0.0, -2.3, 1.5);
    spheres[2].radius = 0.7;
    spheres[2].materialIndex = 6;
    
    // Blue glossy sphere  
    spheres[3].center = vec3(-2.0, -2.5, 1.5);
    spheres[3].radius = 0.5;
    spheres[3].materialIndex = 7;
    
    // Small emissive sphere (accent light)
    spheres[4].center = vec3(2.0, -1.5, -1.5);
    spheres[4].radius = 0.4;
    spheres[4].materialIndex = 5;
    
    // Bronze sphere
    spheres[5].center = vec3(0.5, -2.6, -1.8);
    spheres[5].radius = 0.4;
    spheres[5].materialIndex = 9;
}

// Scene intersection
bool intersectScene(vec3 ro, vec3 rd, inout HitRecord hit)
{
    bool hitAnything = false;
    
    // Spheres
    for (int i = 0; i < NUM_SPHERES; i++)
    {
        if (intersectSphere(ro, rd, spheres[i], hit))
            hitAnything = true;
    }
    
    // Quadrics (from uniforms) - DEBUG VERSION
    for (int i = 0; i < uNumQuadrics && i < 8; i++)
    {
        Quadric q;
        q.A = uQuadrics_A[i];
        q.B = uQuadrics_B[i];
        q.C = uQuadrics_C[i];
        q.D = uQuadrics_D[i];
        q.E = uQuadrics_E[i];
        q.F = uQuadrics_F[i];
        q.G = uQuadrics_G[i];
        q.H = uQuadrics_H[i];
        q.I = uQuadrics_I[i];
        q.J = uQuadrics_J[i];
        q.bboxMin = uQuadrics_bboxMin[i];
        q.bboxMax = uQuadrics_bboxMax[i];
        q.materialIndex = uQuadrics_materialIndex[i];
        
        bool quadricHit = intersectQuadric(ro, rd, q, hit);
        if (quadricHit)
            hitAnything = true;
    }
    
    // Cornell box walls
    Plane floor;
    floor.point = vec3(0.0, -3.0, 0.0);
    floor.normal = vec3(0.0, 1.0, 0.0);
    floor.materialIndex = 0;
    if (intersectPlane(ro, rd, floor, hit)) hitAnything = true;
    
    Plane ceiling;
    ceiling.point = vec3(0.0, 3.0, 0.0);
    ceiling.normal = vec3(0.0, -1.0, 0.0);
    ceiling.materialIndex = 0;
    if (intersectPlane(ro, rd, ceiling, hit)) hitAnything = true;
    
    Plane backWall;
    backWall.point = vec3(0.0, 0.0, -4.0);
    backWall.normal = vec3(0.0, 0.0, 1.0);
    backWall.materialIndex = 0;
    if (intersectPlane(ro, rd, backWall, hit)) hitAnything = true;
    
    Plane leftWall;
    leftWall.point = vec3(-3.5, 0.0, 0.0);
    leftWall.normal = vec3(1.0, 0.0, 0.0);
    leftWall.materialIndex = 1;
    if (intersectPlane(ro, rd, leftWall, hit)) hitAnything = true;
    
    Plane rightWall;
    rightWall.point = vec3(3.5, 0.0, 0.0);
    rightWall.normal = vec3(-1.0, 0.0, 0.0);
    rightWall.materialIndex = 2;
    if (intersectPlane(ro, rd, rightWall, hit)) hitAnything = true;
    
    return hitAnything;
}

// ----------------------------------------------------------------------------
// ENVIRONMENT LIGHTING - HDR Sky
// ----------------------------------------------------------------------------
vec3 sampleEnvironment(vec3 rd)
{
    // Gradient sky with sun
    float t = 0.5 * (rd.y + 1.0);
    vec3 skyColor = mix(vec3(0.8, 0.85, 0.9), vec3(0.4, 0.6, 0.9), t);
    
    // Sun
    vec3 sunDir = normalize(vec3(0.5, 0.8, 0.3));
    float sunDot = max(dot(rd, sunDir), 0.0);
    vec3 sunColor = vec3(1.0, 0.95, 0.85) * pow(sunDot, 256.0) * 50.0;
    
    // Sun glow
    vec3 sunGlow = vec3(1.0, 0.9, 0.7) * pow(sunDot, 8.0) * 0.5;
    
    // Ground reflection
    if (rd.y < 0.0)
    {
        skyColor = mix(skyColor, vec3(0.2, 0.15, 0.1), -rd.y);
    }
    
    return vec3(0.0, 0.0, 0.0); //skyColor * 0.5; + sunColor + sunGlow;
}

// ----------------------------------------------------------------------------
// PATH TRACING KERNEL
// ----------------------------------------------------------------------------
vec3 pathTrace(vec3 rayOrigin, vec3 rayDirection)
{
    vec3 radiance = vec3(0.0);
    vec3 throughput = vec3(1.0);
    
    vec3 ro = rayOrigin;
    vec3 rd = rayDirection;
    
    int maxBounces = uBounces > 0 ? uBounces : 8;
    
    for (int bounce = 0; bounce < 16; bounce++)
    {
        if (bounce >= maxBounces) break;
        
        HitRecord hit;
        hit.t = MAX_DISTANCE;
        
        if (!intersectScene(ro, rd, hit))
        {
            // Hit environment
            radiance += throughput * sampleEnvironment(rd);
            break;
        }
        
        Material mat = materials[hit.materialIndex];
        
        // Add emission
        radiance += throughput * mat.emission * mat.emissionStrength;
        
        // Russian roulette (after a few bounces)
        if (bounce > 3)
        {
            float p = max(max(throughput.r, throughput.g), throughput.b);
            if (randomFloat() > p) break;
            throughput /= p;
        }
        
        // Sample next direction
        vec3 V = -rd;
        vec3 N = hit.normal;
        //    function traceRay(scene, P, d):
        //      (t, N, mtrl) = scene intersect (P, d)
        //      Q = ray (P, d) evaluated at t
        //      I = shade(mtrl, scene, Q, N, d)
        //      R = reflectDirection(N, -d)
        //      I < I + mtrl.k, * traceRay(scene, Q, R)
        //      if ray is entering object then
        //          ni = index_of_air
        //          nt = mtrl.index
        //      else
        //          n_i = mtrl.index
        //          n_t= index of air
        //          if (mtrl.k_t > 0 and notTIR (n_i, n_t, N, -d)) then
        //          T = refractDirection (n_i, n_t, N, -d)
        //          1<1+ mtrl.k, * traceRay(scene, Q, T)
        //      end if
        //      return I
        //    end function
        
        // Handle transmission (glass)
        if (mat.transmission > 0.0)
        {
            float eta = hit.frontFace ? (1.0 / mat.ior) : mat.ior;
            vec3 refracted = refract(rd, N, eta);
            
            // Fresnel reflection probability
            float cosTheta = min(dot(V, N), 1.0);
            float r0 = (1.0 - eta) / (1.0 + eta);
            r0 = r0 * r0;
            float fresnel = r0 + (1.0 - r0) * pow(1.0 - cosTheta, 5.0);
            
            if (length(refracted) < 0.001 || randomFloat() < fresnel)
            {
                // Reflect
                rd = reflect(rd, N);
            }
            else
            {
                // Refract
                rd = refracted;
                throughput *= mat.albedo;
            }
            ro = hit.position + rd * EPSILON * 10.0;
        }
        else
        {
            // Sample BRDF
            vec3 brdfThroughput;
            rd = sampleBRDF(V, N, mat, brdfThroughput);
            throughput *= brdfThroughput;
            
            ro = hit.position + N * EPSILON;
        }
        
        // Check for NaN/Inf
        if (any(isnan(throughput)) || any(isinf(throughput)))
        {
            break;
        }
    }
    
    return radiance;
}

// ----------------------------------------------------------------------------
// MAIN
// ----------------------------------------------------------------------------
void main()
{
    // Initialize scene
    initScene();

    // Initialize RNG with pixel position and frame number
    ivec2 pixelCoord = ivec2(gl_FragCoord.xy);
    rngState = uint(pixelCoord.x + pixelCoord.y * int(uResolution.x)) * uint(uFrame * 719393 + 1);
    rngState = pcgHash(rngState);
    
    // Anti-aliasing jitter
    vec2 jitter = randomVec2() - 0.5;
    vec2 uv = (gl_FragCoord.xy + jitter) / uResolution;
    // S = PointInPixel
    vec2 ndc = uv * 2.0 - 1.0;

    // Calculate ray direction

    vec4 target = uInverseProjection * vec4(ndc, 1.0, 1.0);
    // rayDir: d = (S-P) / |S-P|
    vec3 rayDir = normalize(vec3(uInverseView * vec4(normalize(target.xyz / target.w), 0.0)));
    // rayOrigin: P = CameraOrigin
    vec3 rayOrigin = uCameraPosition;
    
    // Depth of field (optional - controlled by aperture uniform)
    if (uAperture > 0.0)
    {
        vec3 focalPoint = rayOrigin + rayDir * uFocusDistance;
        vec2 diskSample = randomInUnitDisk() * uAperture;
        vec3 right = vec3(uInverseView[0]);
        vec3 up = vec3(uInverseView[1]);
        // rayOrigin: P = CameraOrigin
        rayOrigin = rayOrigin + right * diskSample.x + up * diskSample.y;
        // rayDir: d = (S-P) / |S-P|
        rayDir = normalize(focalPoint - rayOrigin);
    }
    
    // Path trace
    //    for each pixel (i,j) in image
    //      S = PointInPixel
    //      P = CameraOrigin
    //      d = (S-P) / |S-P|
    //      I(1,j) = traceRay(scene, P, d)
    //    end for
    vec3 color = pathTrace(rayOrigin, rayDir);
    
    // Clamp fireflies
    float luminance = dot(color, vec3(0.2126, 0.7152, 0.0722));
    if (luminance > 10.0)
    {
        color *= 10.0 / luminance;
    }
    
    FragColor = vec4(color, 1.0);
}

