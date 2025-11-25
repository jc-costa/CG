#ifndef RAY_H
#define RAY_H

#include "Vec3.h"

class Ray {
public:
    Point3 origin;
    Vec3 direction;

    // Constructors
    Ray() {}
    Ray(const Point3& origin, const Vec3& direction) 
        : origin(origin), direction(direction) {}

    // Calculate point along the ray: P(t) = origin + t * direction
    Point3 at(float t) const {
        return origin + t * direction;
    }

    // Getters
    Point3 getOrigin() const { return origin; }
    Vec3 getDirection() const { return direction; }
};

#endif
