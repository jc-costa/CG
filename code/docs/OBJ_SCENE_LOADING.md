# Scene Loading System Documentation

## Overview

The Scene Loading System provides functionality to load 3D geometry from Wavefront OBJ files and render them using GPU-based path tracing. The system consists of three main components:

1. **FileManager** - Low-level file I/O utilities
2. **SceneManager** - OBJ/MTL parsing and GPU upload
3. **PathTrace.glsl** - Shader-side triangle intersection and material handling

## Architecture

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                              USER INPUT                                      │
│                         Press 'O' to load OBJ                               │
└─────────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                              Main.cpp                                        │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │ KeyCallback():                                                       │   │
│  │   1. Load OBJ file via SceneManager                                 │   │
│  │   2. Upload to GPU                                                   │   │
│  │   3. Set camera from OBJ if available                               │   │
│  │   4. Set s_UseOBJScene = true                                       │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                            SceneManager                                      │
│  ┌───────────────────┐    ┌───────────────────┐    ┌──────────────────┐   │
│  │   LoadOBJ()       │───▶│   ParseOBJLine()  │───▶│ ProcessFace()    │   │
│  │                   │    │                   │    │ (triangulate)    │   │
│  └───────────────────┘    └───────────────────┘    └──────────────────┘   │
│           │                        │                                        │
│           │                        ▼                                        │
│           │               ┌───────────────────┐                             │
│           │               │   LoadMTL()       │                             │
│           │               │   ParseMTLLine()  │                             │
│           │               └───────────────────┘                             │
│           ▼                                                                  │
│  ┌───────────────────┐    ┌───────────────────────────────────────────┐   │
│  │ NormalizeScene()  │───▶│ UploadToGPU()                             │   │
│  │ (center & scale)  │    │   • Create RGBA32F textures               │   │
│  └───────────────────┘    │   • Pack vertex/normal/material data      │   │
│                           └───────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                         GPU (OpenGL Textures)                               │
│  ┌────────────────┐ ┌────────────────┐ ┌──────────────┐ ┌──────────────┐  │
│  │ uTrianglesTex  │ │ uNormalsTex    │ │ uTriMatTex   │ │ uMaterialsTex│  │
│  │ (3 x numTris)  │ │ (3 x numTris)  │ │ (1 x numTris)│ │ (3 x numMats)│  │
│  │ V0, V1, V2     │ │ N0, N1, N2     │ │ matIndex     │ │ mat props    │  │
│  └────────────────┘ └────────────────┘ └──────────────┘ └──────────────┘  │
└─────────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                          PathTrace.glsl                                      │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │ intersectOBJMesh():                                                  │   │
│  │   for each triangle:                                                 │   │
│  │     1. Sample vertices from uTrianglesTex                           │   │
│  │     2. Sample normals from uNormalsTex                              │   │
│  │     3. Sample material index from uTriMatTex                        │   │
│  │     4. Perform Möller-Trumbore intersection                         │   │
│  │                                                                      │   │
│  │ getMaterialFromTexture():                                            │   │
│  │     Sample material properties from uMaterialsTex                    │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────────────────┘
```

## File Formats

### OBJ Format Support

The parser supports standard Wavefront OBJ commands:

| Command | Format | Description |
|---------|--------|-------------|
| `v` | `v x y z [w]` | Vertex position |
| `vn` | `vn x y z` | Vertex normal |
| `vt` | `vt u v [w]` | Texture coordinate |
| `f` | `f v/vt/vn ...` | Face definition |
| `mtllib` | `mtllib file.mtl` | Material library |
| `usemtl` | `usemtl name` | Use material |
| `g` | `g name` | Group (ignored) |
| `o` | `o name` | Object (ignored) |
| `s` | `s on/off` | Smooth shading (ignored) |

**Custom Extensions** (Cornell Box format):

| Command | Format | Description |
|---------|--------|-------------|
| `c` | `c eye target up` | Camera definition |
| `lp` | `lp vertex` | Light point |

**Face Format Variants:**
- `f 1 2 3` - Vertex indices only
- `f 1/1 2/2 3/3` - Vertex/texture indices
- `f 1/1/1 2/2/2 3/3/3` - Vertex/texture/normal indices
- `f 1//1 2//2 3//3` - Vertex/normal indices (no texture)

**Index Handling:**
- Indices are 1-based (OBJ standard)
- Negative indices are relative to current position (-1 = last vertex)

### MTL Format Support

| Command | Format | PBR Mapping |
|---------|--------|-------------|
| `newmtl` | `newmtl name` | Define material |
| `Kd` | `Kd r g b` | Albedo (base color) |
| `Ke` | `Ke r g b` | Emission color |
| `Ns` | `Ns value` | Roughness = 1 - (Ns/1000) |
| `Ni` | `Ni value` | Index of refraction |
| `d` | `d value` | Opacity → Transmission |
| `Tr` | `Tr value` | Transparency → Transmission |
| `illum` | `illum model` | See below |

**Illumination Models:**
- `illum 3` → Metallic = 1.0 (mirror)
- `illum 7` → Transmission = 1.0 (glass)

## GPU Texture Layout

### Triangle Texture (uTrianglesTex)

```
Width: 3 pixels (one per vertex)
Height: numTriangles
Format: GL_RGBA32F

       Column 0       Column 1       Column 2
      ┌─────────────┬─────────────┬─────────────┐
Row 0 │ V0.xyz, 1.0 │ V1.xyz, 1.0 │ V2.xyz, 1.0 │  Triangle 0
      ├─────────────┼─────────────┼─────────────┤
Row 1 │ V0.xyz, 1.0 │ V1.xyz, 1.0 │ V2.xyz, 1.0 │  Triangle 1
      ├─────────────┼─────────────┼─────────────┤
  ... │     ...     │     ...     │     ...     │
      └─────────────┴─────────────┴─────────────┘

Shader access:
  float texY = (float(triIndex) + 0.5) / float(numTriangles);
  vec3 v0 = texture(uTrianglesTex, vec2(0.5/3.0, texY)).xyz;
  vec3 v1 = texture(uTrianglesTex, vec2(1.5/3.0, texY)).xyz;
  vec3 v2 = texture(uTrianglesTex, vec2(2.5/3.0, texY)).xyz;
```

### Normal Texture (uNormalsTex)

Same layout as triangle texture but contains per-vertex normals.

### Triangle Material Texture (uTriMatTex)

```
Width: 1 pixel
Height: numTriangles
Format: GL_RGBA32F

       Column 0
      ┌─────────────────┐
Row 0 │ matIdx, 0, 0, 0 │  Triangle 0
      ├─────────────────┤
Row 1 │ matIdx, 0, 0, 0 │  Triangle 1
      └─────────────────┘

Shader access:
  int matIdx = int(texture(uTriMatTex, vec2(0.5, texY)).r);
```

### Material Texture (uMaterialsTex)

```
Width: 3 pixels (property groups)
Height: numMaterials
Format: GL_RGBA32F

       Column 0              Column 1              Column 2
      ┌────────────────────┬────────────────────┬────────────────────┐
Row 0 │ albedo.rgb,        │ emission.rgb,      │ emitStr, ior,      │  Material 0
      │ roughness          │ metallic           │ transmission, 0    │
      ├────────────────────┼────────────────────┼────────────────────┤
Row 1 │     ...            │     ...            │     ...            │  Material 1
      └────────────────────┴────────────────────┴────────────────────┘

Shader access:
  float texY = (float(matIndex) + 0.5) / float(numMaterials);
  vec4 row0 = texture(uMaterialsTex, vec2(0.5/3.0, texY));  // albedo + roughness
  vec4 row1 = texture(uMaterialsTex, vec2(1.5/3.0, texY));  // emission + metallic
  vec4 row2 = texture(uMaterialsTex, vec2(2.5/3.0, texY));  // emitStr, ior, trans
```

## Keyboard Controls

| Key | Action |
|-----|--------|
| **O** | Load `assets/cornell_box.obj` |
| **P** | Toggle between OBJ mesh and procedural scene |
| **S** | Cycle through procedural scenes |

## Usage Example

### Loading a Custom OBJ File

```cpp
// In Main.cpp KeyCallback or initialization:
SceneManager sceneManager;

std::filesystem::path objPath = "path/to/model.obj";
if (sceneManager.LoadOBJ(objPath))
{
    sceneManager.UploadToGPU();
    
    // Use camera from OBJ if available
    const SceneData& scene = sceneManager.GetSceneData();
    if (scene.HasCamera)
    {
        camera.Position = scene.CameraPosition;
        camera.Forward = glm::normalize(scene.CameraTarget - scene.CameraPosition);
    }
}
```

### Binding in Render Loop

```cpp
void RenderPathTrace()
{
    glUseProgram(s_PathTraceShader);
    
    // Set other uniforms...
    
    // Bind OBJ scene if enabled
    glUniform1i(glGetUniformLocation(shader, "uUseOBJScene"), s_UseOBJScene);
    if (s_UseOBJScene)
    {
        s_SceneManager.BindTextures(s_PathTraceShader);
    }
    
    // Draw...
}
```

## Performance Considerations

### Current Limitations

1. **Linear Triangle Search**: The shader iterates through all triangles for each ray intersection. For large meshes, this can be slow.

2. **No BVH**: A Bounding Volume Hierarchy would significantly improve performance for complex scenes.

3. **Texture Size Limits**: Very large meshes may exceed texture dimension limits.

### Optimization Opportunities

1. **BVH Acceleration Structure**: Store BVH nodes in a texture for logarithmic intersection time.

2. **Early Ray Termination**: Skip intersection tests for triangles outside ray's current t-range.

3. **Spatial Hashing**: For scenes with clustered geometry.

## File Structure

```
code/App/
├── Source/
│   ├── SceneManager/           # Scene loading subsystem
│   │   ├── FileManager.h       # File I/O utilities
│   │   ├── FileManager.cpp
│   │   ├── SceneManager.h      # OBJ/MTL parser & GPU upload
│   │   ├── SceneManager.cpp
│   │   └── SceneManagerTest/   # Test suite for scene loading
│   │       ├── SceneManagerTest.cpp
│   │       ├── test.sh
│   │       └── test_assets/    # OBJ/MTL test files
│   ├── QuadricManager/         # Quadric surface management
│   │   ├── QuadricManager.h    # Quadric data & ImGui editor
│   │   └── QuadricManager.cpp
│   └── Main.cpp                # Integration & key handling
├── Shaders/PathTrace/
│   └── PathTrace.glsl          # Triangle intersection & materials
├── assets/
│   ├── cornell_box.obj         # Example scene
│   └── cornell_box.mtl         # Materials for cornell box
code/docs/
    └── OBJ_SCENE_LOADING.md    # This documentation
```

## References

- [Wavefront OBJ Specification](http://www.martinreddy.net/gfx/3d/OBJ.spec)
- [MTL Material Format](http://paulbourke.net/dataformats/mtl/)
- [Cornell Box Data](http://www.graphics.cornell.edu/online/box/data.html)
- [Möller–Trumbore Algorithm](https://en.wikipedia.org/wiki/M%C3%B6ller%E2%80%93Trumbore_intersection_algorithm)

