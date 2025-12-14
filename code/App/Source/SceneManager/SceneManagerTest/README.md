# SceneManager Test Suite

A comprehensive test suite for the `SceneManager` class, verifying OBJ/MTL file parsing, face triangulation, material handling, custom extensions, and scene normalization.

## Table of Contents

- [Quick Start](#quick-start)
- [Architecture](#architecture)
- [Test Suites](#test-suites)
- [Test Assets](#test-assets)
- [Quadric Surfaces Reference](#quadric-surfaces-reference)
- [OBJ File Format Reference](#obj-file-format-reference)
- [MTL File Format Reference](#mtl-file-format-reference)
- [Adding New Tests](#adding-new-tests)
- [Troubleshooting](#troubleshooting)

---

## Quick Start

```bash
cd code/App/Source/SceneManager/SceneManagerTest
chmod +x test.sh
./test.sh
```

Expected output:
```
╔════════════════════════════════════════════════════════════╗
║          SCENE MANAGER TEST SUITE                          ║
║          Comprehensive OBJ/MTL Parser Tests                ║
╚════════════════════════════════════════════════════════════╝
...
════════════════════════════════════════
 TEST SUMMARY
════════════════════════════════════════
  Tests:      33 passed
  Assertions: 75 passed
════════════════════════════════════════
  ✓ ALL TESTS PASSED!
════════════════════════════════════════
```

---

## Architecture

### Directory Structure

```
code/App/Source/
├── SceneManager/                   # Scene loading subsystem
│   ├── FileManager.h               # File I/O utilities
│   ├── FileManager.cpp
│   ├── SceneManager.h              # OBJ/MTL parser & GPU upload
│   ├── SceneManager.cpp
│   └── SceneManagerTest/           # Test suite (this directory)
│       ├── CMakeLists.txt          # CMake build configuration
│       ├── test.sh                 # Build and run script
│       ├── SceneManagerTest.cpp    # Main test file (33 tests)
│       ├── README.md               # This documentation
│       ├── build/                  # Build artifacts (generated)
│       │   ├── scene_manager_test  # Test executable
│       │   ├── mock_gl.h           # Generated mock OpenGL header
│       │   └── test_assets/        # Copied test files
│       └── test_assets/            # Test OBJ/MTL files
│           ├── quadric_materials.mtl   # Material library
│           ├── box.obj                 # Unit cube
│           ├── sphere.obj              # Icosphere
│           ├── cylinder.obj            # Circular cylinder
│           ├── cone.obj                # Circular cone
│           ├── ellipsoid.obj           # Stretched sphere
│           ├── elliptic_paraboloid.obj # Bowl shape
│           ├── hyperbolic_paraboloid.obj # Saddle shape
│           ├── hyperboloid_one_sheet.obj # Cooling tower shape
│           ├── hyperboloid_two_sheets.obj # Two bowls
│           ├── test_minimal.obj        # Single triangle
│           ├── test_with_camera.obj    # Camera/light extensions
│           ├── test_polygon.obj        # Triangulation test
│           ├── test_negative_indices.obj # Relative indexing
│           └── test_all_materials.obj  # All material types
├── QuadricManager/                 # Quadric surface management
│   ├── QuadricManager.h
│   └── QuadricManager.cpp
└── Quadric/                        # Quadric surface library
    ├── Quadric.h
    ├── Quadric.cpp
    └── README.md
```

### Design Principles

1. **Standalone Execution**: Tests compile and run without the main application or OpenGL
2. **Mock OpenGL**: Uses generated stub header for GL types/functions
3. **Automatic Dependencies**: GLM fetched via CMake FetchContent
4. **Self-Documenting**: Each test file contains comprehensive documentation
5. **Colored Output**: Visual feedback for pass/fail status

### Test Framework

The test framework (`TestFramework` namespace) provides:

```cpp
// Assertions
AssertTrue(condition, message);
AssertFalse(condition, message);
AssertEqual(expected, actual, message);
AssertFloatEqual(expected, actual, message);
AssertVec3Equal(expected, actual, message);
AssertGreaterThan(actual, threshold, message);
AssertStringEqual(expected, actual, message);

// Test structure
BeginTest("Test name");
// ... assertions ...
EndTest();

// Reporting
PrintSectionHeader("Section name");
PrintSummary();
```

---

## Test Suites

### Suite 1: Basic Loading Tests

| Test | Description |
|------|-------------|
| `TestMinimalOBJLoading` | Load single triangle OBJ |
| `TestBoxOBJLoading` | Load cube with materials |
| `TestNonExistentFile` | Handle missing file gracefully |
| `TestClearFunction` | Verify Clear() resets state |

### Suite 2: Quadric Surface Loading Tests

| Test | Description | Triangles |
|------|-------------|-----------|
| `TestSphereLoading` | Icosphere approximation | 76 |
| `TestCylinderLoading` | Cylinder with caps | 48 |
| `TestConeLoading` | Cone with base | 24 |
| `TestEllipsoidLoading` | Stretched sphere | 20 |
| `TestEllipticParaboloidLoading` | Bowl surface | 128 |
| `TestHyperbolicParaboloidLoading` | Saddle surface | 128 |
| `TestHyperboloidOneSheetLoading` | Single connected surface | 192 |
| `TestHyperboloidTwoSheetsLoading` | Two separate surfaces | 168 |

### Suite 3: Face Parsing & Triangulation Tests

| Test | Description |
|------|-------------|
| `TestPolygonTriangulation` | Quad/pentagon/hexagon → triangles |
| `TestNegativeIndices` | Relative vertex/normal indexing |
| `TestFaceFormats` | v, v/vt, v/vt/vn, v//vn formats |

### Suite 4: Material Parsing Tests

| Test | Description |
|------|-------------|
| `TestMaterialLoading` | MTL file loading, material count |
| `TestDiffuseMaterialProperties` | Kd (diffuse color) parsing |
| `TestMetallicMaterialProperties` | illum 3 → Metallic=1.0 |
| `TestGlassMaterialProperties` | illum 7, Ni, d properties |
| `TestEmissiveMaterialProperties` | Ke (emission) parsing |
| `TestRoughnessConversion` | Ns → Roughness conversion |
| `TestTransparencyProperties` | d/Tr → Transmission |

### Suite 5: Custom Extension Tests

| Test | Description |
|------|-------------|
| `TestCameraExtension` | `c` command parsing |
| `TestLightPointExtension` | `lp` command parsing |

### Suite 6: Scene Normalization Tests

| Test | Description |
|------|-------------|
| `TestSceneNormalization` | Fit to 6×6×6, center at origin |
| `TestLargeSceneNormalization` | Large geometry normalization |

### Suite 7-10: Additional Tests

| Suite | Tests |
|-------|-------|
| Triangle-Material Association | Material index validity |
| Edge Cases & Error Handling | Empty files, missing MTL, auto-normals |
| Data Integrity | No NaN/Inf, valid ranges |
| API Access | GetSceneData() verification |

---

## Test Assets

### Quadric Surface Files

Each quadric surface OBJ file contains:
- Mathematical definition (implicit and parametric forms)
- Quadric coefficient matrix
- Geometry specifications
- Normal calculation formulas
- Mesh structure details

### Material Library (quadric_materials.mtl)

| Category | Materials | Properties Tested |
|----------|-----------|-------------------|
| Diffuse | white, red, green, blue | Kd |
| Metallic | gold, chrome | illum 3 |
| Glass | clear, tinted | illum 7, Ni, d |
| Emissive | warm, cool | Ke |
| Roughness | rough, smooth, semi_rough | Ns |
| Transparent | 50%, 25% | d |

---

## Quadric Surfaces Reference

### Classification

| Surface | Equation | Gaussian Curvature | Topology |
|---------|----------|-------------------|----------|
| Sphere | x² + y² + z² = r² | K > 0 | Closed |
| Ellipsoid | x²/a² + y²/b² + z²/c² = 1 | K > 0 | Closed |
| Cylinder | x² + y² = r² | K = 0 | Open |
| Cone | x² + y² = (z·tan α)² | K = 0 | Open |
| Elliptic Paraboloid | z = x² + y² | K > 0 | Open |
| Hyperbolic Paraboloid | z = x² - y² | K < 0 | Open |
| Hyperboloid (1 sheet) | x² + y² - z² = 1 | K < 0 | Connected |
| Hyperboloid (2 sheets) | -x² - y² + z² = 1 | K > 0 | Disconnected |

### General Quadric Equation

```
Ax² + By² + Cz² + Dxy + Exz + Fyz + Gx + Hy + Iz + J = 0
```

Matrix form:
```
[x y z 1] · [A   D/2 E/2 G/2]   [x]
            [D/2 B   F/2 H/2] · [y] = 0
            [E/2 F/2 C   I/2]   [z]
            [G/2 H/2 I/2 J  ]   [1]
```

---

## OBJ File Format Reference

### Supported Commands

| Command | Syntax | Description |
|---------|--------|-------------|
| v | `v x y z [w]` | Vertex position |
| vn | `vn x y z` | Vertex normal |
| vt | `vt u [v] [w]` | Texture coordinate |
| f | `f v1 v2 v3 ...` | Face definition |
| mtllib | `mtllib file.mtl` | Material library |
| usemtl | `usemtl name` | Use material |
| g | `g name` | Group (ignored) |
| o | `o name` | Object (ignored) |
| s | `s on/off/n` | Smoothing (ignored) |
| # | `# comment` | Comment |

### Custom Extensions (Cornell Box Format)

| Command | Syntax | Description |
|---------|--------|-------------|
| c | `c eye target up` | Camera definition |
| lp | `lp vertex` | Light point |

### Face Formats

```
f 1 2 3              # vertex only
f 1/1 2/2 3/3        # vertex/texcoord
f 1/1/1 2/2/2 3/3/3  # vertex/texcoord/normal
f 1//1 2//2 3//3     # vertex//normal (no texcoord)
```

### Index Types

- **Positive**: 1-based index from start of list
- **Negative**: Relative index from current position (-1 = last)

---

## MTL File Format Reference

### Supported Commands

| Command | Syntax | Description | PBR Mapping |
|---------|--------|-------------|-------------|
| newmtl | `newmtl name` | New material | - |
| Kd | `Kd r g b` | Diffuse color | Albedo |
| Ke | `Ke r g b` | Emissive color | Emission |
| Ns | `Ns value` | Specular exponent | 1 - Ns/1000 → Roughness |
| Ni | `Ni value` | Index of refraction | IOR |
| d | `d value` | Dissolve (opacity) | 1 - d → Transmission |
| Tr | `Tr value` | Transparency | Tr → Transmission |
| illum | `illum n` | Illumination model | See below |

### Illumination Models

| Model | Description | Effect |
|-------|-------------|--------|
| 0 | Color on, ambient off | - |
| 1 | Color on, ambient on | - |
| 2 | Highlight on | Default |
| 3 | Reflection on | Metallic = 1.0 |
| 4-6 | Various ray trace modes | - |
| 7 | Refraction + Fresnel | Transmission = 1.0 |

---

## Adding New Tests

### Step 1: Add Test Function

```cpp
void TestNewFeature()
{
    BeginTest("Description of what's being tested");
    
    SceneManager manager;
    bool loaded = manager.LoadOBJ(GetTestAssetPath("my_test.obj"));
    
    AssertTrue(loaded, "File should load");
    AssertEqual(size_t{10}, manager.GetTriangleCount(), "Triangle count");
    
    // Access scene data for detailed checks
    const auto& scene = manager.GetSceneData();
    AssertFloatEqual(0.5f, scene.Materials[0].Roughness, "Roughness value");
    
    EndTest();
}
```

### Step 2: Add to Main

```cpp
int main()
{
    // ... existing tests ...
    
    PrintSectionHeader("SUITE 11: New Feature Tests");
    TestNewFeature();
    
    PrintSummary();
    return GetExitCode();
}
```

### Step 3: Add Test Assets (if needed)

Create OBJ/MTL files in `test_assets/` with documentation:

```obj
# ============================================================================
# MY TEST FILE - Description
# ============================================================================
# Purpose and details...
# ============================================================================

mtllib ./quadric_materials.mtl

v 0 0 0
v 1 0 0
v 0 1 0

f 1 2 3
```

---

## Troubleshooting

### Build Fails: GLM Download Error

```
Failed to clone repository: 'https://github.com/g-truc/glm.git'
```

**Solution**: Ensure network access and run with `--all` permissions:
```bash
rm -rf build
./test.sh
```

### Test Fails: File Not Found

```
[FileManager] Failed to open file: test_assets/xxx.obj
```

**Solution**: Run from the `SceneManagerTest` directory:
```bash
cd code/App/Source/SceneManagerTest
./test.sh
```

### Assertion Failed

Check the error message for expected vs actual values:
```
✗ ASSERT FAILED: Triangle count (expected 12, got 10)
```

**Debug**: Add prints in the test or check the OBJ file for issues.

### OpenGL Errors During Testing

This shouldn't happen since we use mock OpenGL. If it does:
- Verify `USE_MOCK_GL` is defined
- Check that `mock_gl.h` was generated in `build/`
- Ensure SceneManager.h has the conditional include

---

## Dependencies

| Dependency | Version | Purpose | Source |
|------------|---------|---------|--------|
| CMake | 3.14+ | Build system | System |
| C++ Compiler | C++17 | Compilation | System |
| GLM | 1.0.1 | Math types | FetchContent |

---

## License

This test suite is part of the CG Path Tracer project.
