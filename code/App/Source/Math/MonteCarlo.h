#ifndef MONTECARLO_H
#define MONTECARLO_H

#include "Vec3.h"
#include <random>

class MonteCarlo {
public:
    // Initialize the random number generator
    static void init();
    static void setSeed(unsigned int seed);

    // Generate random number between 0 and 1
    static float randomFloat();
    
    // Generate random number between min and max
    static float randomFloat(float min, float max);

    // Generate random vector in unit sphere (for diffuse directions)
    static Vec3 randomInUnitSphere();
    
    // Generate random vector in hemisphere (for diffuse directions around the normal)
    static Vec3 randomInHemisphere(const Vec3& normal);
    
    // Generate random vector in unit disk (for depth of field)
    static Vec3 randomInUnitDisk();

    // Cosine-weighted sampling (more accurate for Lambertian surfaces)
    static Vec3 randomCosineDirection();
    static Vec3 randomCosineDirectionInHemisphere(const Vec3& normal);

private:
    static std::mt19937 generator;
    static std::uniform_real_distribution<float> distribution;
};

#endif
