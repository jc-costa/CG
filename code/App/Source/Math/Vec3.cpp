#include "Vec3.h"

// Arithmetic operators
Vec3 Vec3::operator+(const Vec3& v) const {
    return Vec3(x + v.x, y + v.y, z + v.z);
}

Vec3 Vec3::operator-(const Vec3& v) const {
    return Vec3(x - v.x, y - v.y, z - v.z);
}

Vec3 Vec3::operator*(float t) const {
    return Vec3(x * t, y * t, z * t);
}

Vec3 Vec3::operator/(float t) const {
    return Vec3(x / t, y / t, z / t);
}

Vec3 Vec3::operator-() const {
    return Vec3(-x, -y, -z);
}

Vec3& Vec3::operator+=(const Vec3& v) {
    x += v.x;
    y += v.y;
    z += v.z;
    return *this;
}

Vec3& Vec3::operator-=(const Vec3& v) {
    x -= v.x;
    y -= v.y;
    z -= v.z;
    return *this;
}

Vec3& Vec3::operator*=(float t) {
    x *= t;
    y *= t;
    z *= t;
    return *this;
}

Vec3& Vec3::operator/=(float t) {
    return *this *= 1/t;
}

// Dot product
float Vec3::dot(const Vec3& v) const {
    return x * v.x + y * v.y + z * v.z;
}

// Cross product
Vec3 Vec3::cross(const Vec3& v) const {
    return Vec3(
        y * v.z - z * v.y,
        z * v.x - x * v.z,
        x * v.y - y * v.x
    );
}

// Length
float Vec3::length() const {
    return std::sqrt(lengthSquared());
}

float Vec3::lengthSquared() const {
    return x * x + y * y + z * z;
}

// Normalization
Vec3 Vec3::normalized() const {
    float len = length();
    if (len > 0) {
        return *this / len;
    }
    return *this;
}

void Vec3::normalize() {
    float len = length();
    if (len > 0) {
        *this /= len;
    }
}

// Index access
float Vec3::operator[](int i) const {
    if (i == 0) return x;
    if (i == 1) return y;
    return z;
}

float& Vec3::operator[](int i) {
    if (i == 0) return x;
    if (i == 1) return y;
    return z;
}

// Operator for scalar multiplication from the left
Vec3 operator*(float t, const Vec3& v) {
    return v * t;
}

// Output operator for debugging
std::ostream& operator<<(std::ostream& os, const Vec3& v) {
    os << "(" << v.x << ", " << v.y << ", " << v.z << ")";
    return os;
}
