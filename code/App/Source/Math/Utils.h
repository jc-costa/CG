#ifndef UTILS_H
#define UTILS_H

#include <cmath>
#include <limits>
#include <algorithm>

// Mathematical constants
#define PI 3.1415926535897932385f
#define TWO_PI 6.283185307179586476925286766559f
#define INV_PI 0.31830988618379067154f
#define INV_TWO_PI 0.15915494309189533577f
#define PI_OVER_2 1.5707963267948966192f
#define PI_OVER_4 0.78539816339744830962f

// Tolerances and limits
#define EPS 1e-6f
#define EPSILON 1e-6f
#define INFINITY_F std::numeric_limits<float>::infinity()
#define FLOAT_MAX std::numeric_limits<float>::max()
#define FLOAT_MIN std::numeric_limits<float>::lowest()

namespace MathUtils {
    // Clamp: constrains value between min and max
    inline float clamp(float x, float min, float max) {
        if (x < min) return min;
        if (x > max) return max;
        return x;
    }

    // Convert degrees to radians
    inline float degreesToRadians(float degrees) {
        return degrees * PI / 180.0f;
    }

    // Convert radians to degrees
    inline float radiansToDegrees(float radians) {
        return radians * 180.0f / PI;
    }

    // Linear interpolation
    inline float lerp(float a, float b, float t) {
        return a + t * (b - a);
    }

    // Check if a float is approximately zero
    inline bool isZero(float x, float tolerance = EPS) {
        return std::abs(x) < tolerance;
    }

    // Check if two floats are approximately equal
    inline bool nearlyEqual(float a, float b, float tolerance = EPS) {
        return std::abs(a - b) < tolerance;
    }

    // Returns the sign of a number (-1, 0, or 1)
    inline float sign(float x) {
        if (x > 0) return 1.0f;
        if (x < 0) return -1.0f;
        return 0.0f;
    }

    // Square of a number
    inline float sqr(float x) {
        return x * x;
    }

    // Mix/blend between two values
    inline float mix(float x, float y, float a) {
        return x * (1.0f - a) + y * a;
    }

    // Smoothstep: smooth interpolation
    inline float smoothstep(float edge0, float edge1, float x) {
        float t = clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
        return t * t * (3.0f - 2.0f * t);
    }

    // Saturate: clamp between 0 and 1
    inline float saturate(float x) {
        return clamp(x, 0.0f, 1.0f);
    }

    // Safe power (avoids negative values)
    inline float safePow(float base, float exp) {
        return std::pow(std::max(0.0f, base), exp);
    }

    // Safe square root (avoids negative values)
    inline float safeSqrt(float x) {
        return std::sqrt(std::max(0.0f, x));
    }
}

#endif
