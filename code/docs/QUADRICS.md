# Quadric Implementation

## Overview

This project implements quadric surfaces in the path tracer, enabling rendering of shapes like ellipsoids, hyperboloids, paraboloids, and cylinders through the general quadric equation.

## Quadric Equation

The general equation for a 3D quadric surface is:

```
Ax² + By² + Cz² + Dxy + Exz + Fyz + Gx + Hy + Iz + J = 0
```

Where:
- **A, B, C**: Coefficients of quadratic terms (x², y², z²)
- **D, E, F**: Coefficients of cross terms (xy, xz, yz)
- **G, H, I**: Coefficients of linear terms (x, y, z)
- **J**: Constant term

## Implemented Features

### 1. Ray-Quadric Intersection

The ray is defined by: **P(t) = O + t·D**

Where:
- **O** = ray origin
- **D** = ray direction
- **t** = ray parameter

Substituting into the quadric equation yields a quadratic equation:

```
at² + bt + c = 0
```

Solved using the quadratic formula.

### 2. Bounding Box

For unbounded surfaces (cylinders, paraboloids, hyperboloids), we associate an **AABB (Axis-Aligned Bounding Box)** defined by:

- **bboxMin**: Minimum point (x, y, z)
- **bboxMax**: Maximum point (x, y, z)

The intersection is only calculated if the ray intersects the bounding box first, optimizing performance.

### 3. Normal Calculation via Gradient

The normal at a point **P** on the surface is calculated using the gradient of the quadric function:

```
∇Q(P) = (∂Q/∂x, ∂Q/∂y, ∂Q/∂z)
```

Where:
```
∂Q/∂x = 2Ax + Dy + Ez + G
∂Q/∂y = 2By + Dx + Fz + H
∂Q/∂z = 2Cz + Ex + Fy + I
```

The gradient points in the direction perpendicular to the surface, providing the normal.

## Quadric Examples

### Ellipsoid

Equation: `x²/a² + y²/b² + z²/c² = 1`

Coefficients:
```glsl
A = 1/a², B = 1/b², C = 1/c²
D = E = F = G = H = I = 0
J = -1
```

**Example**: Ellipsoid with a=0.8, b=1.2, c=0.6
```glsl
A = 1.5625, B = 0.6944, C = 2.7778
J = -1.0
bboxMin = vec3(-0.8, -1.2, -0.6)
bboxMax = vec3(0.8, 1.2, 0.6)
```

### Sphere

Special case of ellipsoid where `a = b = c = r`:

```glsl
A = B = C = 1
J = -r²
```

### Hyperboloid of One Sheet

Equation: `x²/a² + y²/b² - z²/c² = 1`

```glsl
A = 1/a², B = 1/b², C = -1/c²
J = -1
```

**Example**: a=0.5, b=0.5, c=1.0
```glsl
A = 4.0, B = 4.0, C = -1.0
J = -1.0
bboxMin = vec3(-1.0, -1.0, -2.0)
bboxMax = vec3(1.0, 1.0, 2.0)
```

### Paraboloid

Equation: `z = x² + y²`

Rearranged: `x² + y² - z = 0`

```glsl
A = 1, B = 1, C = 0
I = -1  // linear term in z
J = 0
```

**Example**:
```glsl
A = 1.0, B = 1.0
I = -1.0
bboxMin = vec3(-1.5, -1.5, 0.0)
bboxMax = vec3(1.5, 1.5, 4.5)
```

### Cylinder

Equation: `x² + y² = r²`

Rearranged: `x² + y² - r² = 0`

```glsl
A = 1, B = 1, C = 0  // no z² term
J = -r²
```

**Example**: Cylinder with radius r=0.6
```glsl
A = 1.0, B = 1.0
J = -0.36
bboxMin = vec3(-0.6, -0.6, -2.0)
bboxMax = vec3(0.6, 0.6, 2.0)
```

### Cone

Equation: `x² + y² - z² = 0`

```glsl
A = 1, B = 1, C = -1
J = 0
```

### Hyperboloid of Two Sheets

Equation: `z²/c² - x²/a² - y²/b² = 1`

```glsl
A = -1/a², B = -1/b², C = 1/c²
J = -1
```

## How to Add a New Quadric

### Method 1: Using the ImGui Editor (Runtime)

1. Press **Ctrl+Q** or **Q** to open the Quadric Editor
2. Select a quadric slot (0-7) from the dropdown
3. Adjust the coefficients using the sliders
4. Set the bounding box and material
5. Changes take effect immediately

### Method 2: In Code (Permanent)

Add to `QuadricManager::InitializeDefaults()` in `Source/QuadricManager/QuadricManager.cpp`:

```cpp
m_Quadrics[index] = {
    A, B, C,           // Quadratic coefficients
    D, E, F,           // Cross terms
    G, H, I,           // Linear terms
    J,                 // Constant
    bboxMin,           // glm::vec3 - bbox minimum
    bboxMax,           // glm::vec3 - bbox maximum
    materialIndex      // material index
};
```

### Method 3: In Shader (Advanced)

In `PathTrace.glsl`, inside the `initScene()` function:

```glsl
quadrics[index] = createQuadric(
    A, B, C,           // Quadratic coefficients
    D, E, F,           // Cross terms
    G, H, I,           // Linear terms
    J,                 // Constant
    bboxMin,           // vec3 - bbox minimum
    bboxMax,           // vec3 - bbox maximum
    materialIndex      // material index
);
```

## Architecture

The quadric implementation is organized into two main components:

### QuadricManager (`Source/QuadricManager/`)

The `QuadricManager` class provides:
- **Quadric Storage**: Manages up to 8 simultaneous quadrics
- **ImGui Editor**: Visual interface for real-time coefficient editing
- **Shader Upload**: Sends quadric data to the GPU path tracer
- **Default Initialization**: Pre-configured example quadrics

### Quadric Library (`Source/Quadric/`)

The `Quadric` namespace provides:
- **QuadricSurface Class**: CPU-side quadric representation and ray intersection
- **Factory Methods**: `CreateSphere()`, `CreateCylinder()`, `CreateCone()`, etc.
- **Intersection Testing**: Standalone ray-quadric intersection for testing

## Limitations and Considerations

1. **Bounding Box**: Essential for unbounded surfaces. Without it, the quadric would extend infinitely.

2. **Numerical Precision**: Very small gradient values can cause issues. The code checks `|∇Q| < ε` before using.

3. **Performance**: Ray-quadric intersection requires solving a quadratic equation, which is more expensive than ray-sphere but cheaper than triangle meshes.

4. **Maximum Number**: Currently defined as `MAX_QUADRICS = 8` in `QuadricManager.h`. Adjust as needed.

## References

- **Physically Based Rendering** - Matt Pharr, Wenzel Jakob, Greg Humphreys
- **Ray Tracing Gems** - Eric Haines, Tomas Akenine-Möller
- **Computer Graphics: Principles and Practice** - Foley, van Dam, Feiner, Hughes
