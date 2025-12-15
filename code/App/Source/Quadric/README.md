# Quadric Surfaces Library

This directory contains the standalone quadric surface implementation for CPU-side testing and intersection calculations.

## Related Components

- **QuadricManager** (`../QuadricManager/`): Runtime quadric management with ImGui editor
- **PathTrace.glsl** (`../../Shaders/PathTrace/`): GPU-side quadric intersection in the shader

## How to Compile and Test

### Option 1: Using the test script (easiest)
```bash
cd code/App/Source/Quadric
./test.sh
```

### Option 2: Manual compilation
```bash
cd code/App/Source/Quadric
g++ -std=c++17 -Wall -I../../.. QuadricTest.cpp Quadric.cpp -o quadric_test -lm
./quadric_test
```

### Option 3: Using Makefile
```bash
cd code/App/Source/Quadric
make
make run
```

## How to Use Quadrics in Your Code

### 1. Create a Quadric with Custom Coefficients

```cpp
#include "Quadric/Quadric.h"

// Example: Create a sphere with radius 2
// Equation: x² + y² + z² - 4 = 0
Quadric::QuadricCoefficients coeffs;
coeffs.A = 1.0f;  // coefficient of x²
coeffs.B = 1.0f;  // coefficient of y²
coeffs.C = 1.0f;  // coefficient of z²
coeffs.D = 0.0f;  // coefficient of xy
coeffs.E = 0.0f;  // coefficient of xz
coeffs.F = 0.0f;  // coefficient of yz
coeffs.G = 0.0f;  // coefficient of x
coeffs.H = 0.0f;  // coefficient of y
coeffs.I = 0.0f;  // coefficient of z
coeffs.J = -4.0f; // constant term

Quadric::QuadricSurface quadric(coeffs);
```

### 2. Use Factory Methods (more convenient)

```cpp
// Sphere
auto sphere = Quadric::QuadricSurface::CreateSphere(2.0f);

// Ellipsoid
auto ellipsoid = Quadric::QuadricSurface::CreateEllipsoid(2.0f, 1.5f, 1.0f);

// Cylinder (with bounding box)
auto cylinder = Quadric::QuadricSurface::CreateCylinder(1.5f, 10.0f);

// Cone
auto cone = Quadric::QuadricSurface::CreateCone(0.785f, 10.0f);  // 45 degrees

// Elliptic paraboloid
auto paraboloid = Quadric::QuadricSurface::CreateEllipticParaboloid(1.0f, 1.0f, 5.0f);

// Hyperbolic paraboloid (saddle)
auto saddle = Quadric::QuadricSurface::CreateHyperbolicParaboloid(1.0f, 1.0f, 5.0f);

// Hyperboloid of one sheet
auto hyp1 = Quadric::QuadricSurface::CreateHyperboloidOneSheet(1.0f, 1.0f, 1.0f, 10.0f);

// Hyperboloid of two sheets
auto hyp2 = Quadric::QuadricSurface::CreateHyperboloidTwoSheets(1.0f, 1.0f, 1.0f, 10.0f);
```

### 3. Test Ray-Quadric Intersection

```cpp
// Define ray
glm::vec3 rayOrigin(0.0f, 0.0f, 5.0f);
glm::vec3 rayDirection(0.0f, 0.0f, -1.0f);

// Intersect with quadric
Quadric::IntersectionResult result = quadric.Intersect(rayOrigin, rayDirection);

if (result.Hit)
{
    std::cout << "Hit!" << std::endl;
    std::cout << "Distance: " << result.Distance << std::endl;
    std::cout << "Point: (" << result.Point.x << ", " 
              << result.Point.y << ", " << result.Point.z << ")" << std::endl;
    std::cout << "Normal: (" << result.Normal.x << ", " 
              << result.Normal.y << ", " << result.Normal.z << ")" << std::endl;
}
else
{
    std::cout << "No intersection" << std::endl;
}
```

### 4. Use Bounding Box for Unbounded Surfaces

```cpp
// Create cylinder without bounding box
Quadric::QuadricCoefficients coeffs;
coeffs.A = 1.0f;
coeffs.B = 1.0f;
coeffs.J = -4.0f;  // radius = 2

Quadric::QuadricSurface cylinder(coeffs);

// Add bounding box
Quadric::BoundingBox bbox;
bbox.Min = glm::vec3(-3.0f, -3.0f, -5.0f);
bbox.Max = glm::vec3(3.0f, 3.0f, 5.0f);

cylinder.SetBoundingBox(bbox);
cylinder.SetBoundingBoxEnabled(true);
```

### 5. Evaluate Function and Calculate Normal

```cpp
// Evaluate f(x,y,z) at a point
glm::vec3 point(1.0f, 0.0f, 0.0f);
float value = quadric.Evaluate(point);
std::cout << "f(1,0,0) = " << value << std::endl;

// Calculate normal (gradient) at a point
glm::vec3 normal = quadric.CalculateNormal(point);
std::cout << "Normal: (" << normal.x << ", " 
          << normal.y << ", " << normal.z << ")" << std::endl;
```

## Common Quadric Examples

### Sphere: x² + y² + z² = r²
```cpp
// Coefficients: A=1, B=1, C=1, J=-r²
auto sphere = Quadric::QuadricSurface::CreateSphere(2.0f);
```

### Cylinder: x² + y² = r²
```cpp
// Coefficients: A=1, B=1, J=-r²
auto cylinder = Quadric::QuadricSurface::CreateCylinder(1.5f, 10.0f);
```

### Cone: x² + y² = (z·tan(θ))²
```cpp
// Coefficients: A=1, B=1, C=-tan²(θ)
auto cone = Quadric::QuadricSurface::CreateCone(0.785f, 8.0f);
```

### Ellipsoid: x²/a² + y²/b² + z²/c² = 1
```cpp
// Coefficients: A=1/a², B=1/b², C=1/c², J=-1
auto ellipsoid = Quadric::QuadricSurface::CreateEllipsoid(2.0f, 1.5f, 1.0f);
```

### Paraboloid: z = x²/a² + y²/b²
```cpp
// Coefficients: A=-1/a², B=-1/b², I=1
auto paraboloid = Quadric::QuadricSurface::CreateEllipticParaboloid(1.0f, 1.0f, 5.0f);
```

## Test Results

Automated tests verify:
- ✅ Ray-sphere intersection
- ✅ Ray-ellipsoid intersection
- ✅ Cylinder with bounding box
- ✅ Cone
- ✅ Paraboloid
- ✅ All quadric presets
- ✅ User-provided coefficients input

## Integration with Path Tracer

### Using QuadricManager (Recommended)

The `QuadricManager` class provides a higher-level interface for managing quadrics in the path tracer:

```cpp
#include "QuadricManager/QuadricManager.h"

QuadricManager quadricManager;
quadricManager.InitializeDefaults();  // Load default quadrics

// In render loop:
if (quadricManager.RenderEditor()) {
    // Quadric was modified, reset accumulation
}
quadricManager.UploadToShader(pathTraceShader);
```

### Using Quadric Library Directly (For Testing)

For CPU-side testing and standalone ray intersection:

```cpp
#include "Quadric/Quadric.h"

Quadric::QuadricSurface sphere = Quadric::QuadricSurface::CreateSphere(2.0f);

// Ray intersection test:
Quadric::IntersectionResult hit = sphere.Intersect(rayOrigin, rayDirection);
if (hit.Hit && hit.Distance < closestDistance)
{
    closestDistance = hit.Distance;
    hitPoint = hit.Point;
    hitNormal = hit.Normal;
}
```

## Notes

- Coefficients follow the general equation: **Ax² + By² + Cz² + Dxy + Exz + Fyz + Gx + Hy + Iz + J = 0**
- Normals are calculated via gradient: **∇f(x,y,z) = (∂f/∂x, ∂f/∂y, ∂f/∂z)**
- Bounding boxes are required for unbounded surfaces (cylinders, cones, paraboloids)
- Quadratic equation solving uses numerically stable formula
