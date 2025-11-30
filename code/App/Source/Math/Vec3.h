#ifndef VEC3_H
#define VEC3_H

#include <cmath>
#include <iostream>

class Vec3 {
public:
    float x, y, z;

    // Constructors
    Vec3() : x(0), y(0), z(0) {}
    Vec3(float x, float y, float z) : x(x), y(y), z(z) {}

    // Arithmetic operators
    Vec3 operator+(const Vec3& v) const;
    Vec3 operator-(const Vec3& v) const;
    Vec3 operator*(float t) const;
    Vec3 operator/(float t) const;
    Vec3 operator-() const;
    
    Vec3& operator+=(const Vec3& v);
    Vec3& operator-=(const Vec3& v);
    Vec3& operator*=(float t);
    Vec3& operator/=(float t);

    // Dot product
    float dot(const Vec3& v) const;
    
    // Cross product
    Vec3 cross(const Vec3& v) const;
    
    // Length
    float length() const;
    float lengthSquared() const;
    
    // Normalization
    Vec3 normalized() const;
    void normalize();

    // Index access
    float operator[](int i) const;
    float& operator[](int i);
};

// Operator for scalar multiplication from the left
Vec3 operator*(float t, const Vec3& v);

// Output operator for debugging
std::ostream& operator<<(std::ostream& os, const Vec3& v);

// Useful aliases
using Point3 = Vec3;  // For positions
using Color = Vec3;   // For RGB colors

#endif
