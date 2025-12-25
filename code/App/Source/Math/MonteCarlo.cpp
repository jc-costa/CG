#include "MonteCarlo.h"

#define _USE_MATH_DEFINES
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Static members initialization
std::mt19937 MonteCarlo::generator;
std::uniform_real_distribution<float> MonteCarlo::distribution(0.0f, 1.0f);

void MonteCarlo::init() {
    std::random_device rd;
    generator.seed(rd());
}

void MonteCarlo::setSeed(unsigned int seed) {
    generator.seed(seed);
}

float MonteCarlo::randomFloat() {
    return distribution(generator);
}

float MonteCarlo::randomFloat(float min, float max) {
    return min + (max - min) * randomFloat();
}

Vec3 MonteCarlo::randomInUnitSphere() {
    while (true) {
        Vec3 p(randomFloat(-1, 1), randomFloat(-1, 1), randomFloat(-1, 1));
        if (p.lengthSquared() < 1) {
            return p;
        }
    }
}

Vec3 MonteCarlo::randomInHemisphere(const Vec3& normal) {
    Vec3 inUnitSphere = randomInUnitSphere();
    // If in same hemisphere as normal, return it; otherwise, flip it
    if (inUnitSphere.dot(normal) > 0.0) {
        return inUnitSphere;
    } else {
        return -inUnitSphere;
    }
}

Vec3 MonteCarlo::randomInUnitDisk() {
    while (true) {
        Vec3 p(randomFloat(-1, 1), randomFloat(-1, 1), 0);
        if (p.lengthSquared() < 1) {
            return p;
        }
    }
}

Vec3 MonteCarlo::randomCosineDirection() {
    float r1 = randomFloat();
    float r2 = randomFloat();
    float z = std::sqrt(1 - r2);

    float phi = 2 * M_PI * r1;
    float x = std::cos(phi) * std::sqrt(r2);
    float y = std::sin(phi) * std::sqrt(r2);

    return Vec3(x, y, z);
}

Vec3 MonteCarlo::randomCosineDirectionInHemisphere(const Vec3& normal) {
    // Generate cosine-weighted direction in local space
    Vec3 localDir = randomCosineDirection();
    
    // Create orthonormal coordinate system from normal
    Vec3 a;
    if (std::abs(normal.x) > 0.9) {
        a = Vec3(0, 1, 0);
    } else {
        a = Vec3(1, 0, 0);
    }
    
    Vec3 tangent = normal.cross(a).normalized();
    Vec3 bitangent = normal.cross(tangent);
    
    // Transform from local basis to world basis
    return tangent * localDir.x + bitangent * localDir.y + normal * localDir.z;
}
