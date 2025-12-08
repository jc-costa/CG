#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <filesystem>

#include <glm/glm.hpp>
#include <glad/gl.h>

// ============================================================================
// SCENE MANAGER - OBJ/MTL Scene Loading System
// ============================================================================
//
// This module provides functionality to load 3D scenes from Wavefront OBJ files
// and their associated MTL material files. The loaded geometry is uploaded to
// the GPU as textures for use in the path tracing shader.
//
// SUPPORTED OBJ FEATURES:
// -----------------------
//   v x y z          - Vertex positions
//   vn x y z         - Vertex normals
//   vt u v           - Texture coordinates (parsed but not used in shader)
//   f v1 v2 v3 ...   - Faces (triangulated automatically)
//   f v/vt/vn ...    - Faces with texture coords and normals
//   f v//vn ...      - Faces with normals only
//   mtllib file.mtl  - Material library reference
//   usemtl name      - Use material for subsequent faces
//   g name           - Group name (parsed but ignored)
//   o name           - Object name (parsed but ignored)
//
// CUSTOM EXTENSIONS (Cornell Box format):
// ---------------------------------------
//   c eye target up  - Camera definition (indices into vertex/normal arrays)
//   lp vertex        - Light point position (index into vertex array)
//
// SUPPORTED MTL FEATURES:
// -----------------------
//   newmtl name      - Define new material
//   Kd r g b         - Diffuse color (albedo)
//   Ke r g b         - Emissive color
//   Ns value         - Specular exponent (converted to roughness)
//   Ni value         - Index of refraction
//   d value          - Dissolve/opacity (converted to transmission)
//   Tr value         - Transparency (converted to transmission)
//   illum model      - Illumination model (3=reflective, 7=glass)
//
// GPU DATA LAYOUT:
// ----------------
// Scene data is uploaded to the GPU as 2D textures with RGBA32F format:
//
//   uTrianglesTex (width=3, height=numTriangles):
//     Each row contains one triangle's three vertices (V0, V1, V2)
//     Pixel (0,i) = V0.xyz, Pixel (1,i) = V1.xyz, Pixel (2,i) = V2.xyz
//
//   uNormalsTex (width=3, height=numTriangles):
//     Each row contains one triangle's three vertex normals (N0, N1, N2)
//     Pixel (0,i) = N0.xyz, Pixel (1,i) = N1.xyz, Pixel (2,i) = N2.xyz
//
//   uTriMatTex (width=1, height=numTriangles):
//     Each row contains the material index for that triangle
//     Pixel (0,i).r = materialIndex
//
//   uMaterialsTex (width=3, height=numMaterials):
//     Each row contains one material's properties:
//     Pixel (0,i) = (albedo.rgb, roughness)
//     Pixel (1,i) = (emission.rgb, metallic)
//     Pixel (2,i) = (emissionStrength, ior, transmission, 0)
//
// USAGE EXAMPLE:
// --------------
//   SceneManager sceneManager;
//   
//   // Load OBJ file (also loads referenced MTL files)
//   if (sceneManager.LoadOBJ("assets/cornell_box.obj"))
//   {
//       // Upload to GPU
//       sceneManager.UploadToGPU();
//       
//       // In render loop:
//       sceneManager.BindTextures(shaderProgram);
//       
//       // Access camera if defined in OBJ
//       const SceneData& scene = sceneManager.GetSceneData();
//       if (scene.HasCamera) {
//           camera.Position = scene.CameraPosition;
//       }
//   }
//
// ============================================================================

// ============================================================================
// DATA STRUCTURES
// ============================================================================

// ----------------------------------------------------------------------------
// OBJMaterial
// ----------------------------------------------------------------------------
// Represents a material loaded from an MTL file. Properties are mapped to
// PBR parameters used by the path tracer.
//
// MTL to PBR Mapping:
//   Kd          -> Albedo (base color)
//   Ke          -> Emission color
//   Ns          -> Roughness (inverse relationship: high Ns = low roughness)
//   Ni          -> Index of Refraction
//   d/Tr        -> Transmission (for glass materials)
//   illum 3     -> Metallic = 1.0 (mirror)
//   illum 7     -> Transmission = 1.0 (glass)
// ----------------------------------------------------------------------------
struct OBJMaterial
{
	std::string Name;                           // Material name from MTL file
	glm::vec3 Albedo = glm::vec3(0.8f);         // Kd - diffuse/base color
	glm::vec3 Emission = glm::vec3(0.0f);       // Ke - emissive color
	float Roughness = 0.9f;                     // Derived from Ns (specular exp)
	float Metallic = 0.0f;                      // 0=dielectric, 1=metal
	float EmissionStrength = 0.0f;              // Multiplier for emission
	float IOR = 1.5f;                           // Ni - index of refraction
	float Transmission = 0.0f;                  // For glass/transparent materials
};

// ----------------------------------------------------------------------------
// Triangle
// ----------------------------------------------------------------------------
// Represents a single triangle in the scene with per-vertex data.
// Supports smooth shading via per-vertex normals.
// ----------------------------------------------------------------------------
struct Triangle
{
	glm::vec3 V0, V1, V2;     // Vertex positions (counter-clockwise winding)
	glm::vec3 N0, N1, N2;     // Per-vertex normals (for smooth shading)
	int MaterialIndex = 0;    // Index into SceneData::Materials array
};

// ----------------------------------------------------------------------------
// OBJMesh
// ----------------------------------------------------------------------------
// A named collection of triangles (corresponds to 'g' or 'o' in OBJ).
// Currently not used directly but available for future mesh-level operations.
// ----------------------------------------------------------------------------
struct OBJMesh
{
	std::string Name;
	std::vector<Triangle> Triangles;
};

// ----------------------------------------------------------------------------
// SceneData
// ----------------------------------------------------------------------------
// Complete scene data extracted from OBJ/MTL files.
// Contains all geometry, materials, and optional camera/light information.
// ----------------------------------------------------------------------------
struct SceneData
{
	std::vector<OBJMaterial> Materials;     // All materials (index 0 = default)
	std::vector<Triangle> Triangles;        // All triangles (flattened)
	
	// Camera data (from custom 'c' command in OBJ)
	glm::vec3 CameraPosition = glm::vec3(0.0f, 0.0f, 5.0f);
	glm::vec3 CameraTarget = glm::vec3(0.0f);
	glm::vec3 CameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
	bool HasCamera = false;
	
	// Light data (from custom 'lp' command in OBJ)
	glm::vec3 LightPosition = glm::vec3(0.0f, 5.0f, 0.0f);
	bool HasLight = false;
};

// ============================================================================
// SCENE MANAGER CLASS
// ============================================================================
class SceneManager
{
public:
	SceneManager() = default;
	~SceneManager();
	
	// ========================================================================
	// LoadOBJ
	// ========================================================================
	// Loads a scene from a Wavefront OBJ file.
	//
	// Parameters:
	//   path - Path to the OBJ file
	//
	// Returns:
	//   bool - true if at least one triangle was loaded successfully
	//
	// Notes:
	//   - Automatically loads referenced MTL files (mtllib command)
	//   - Creates a default gray material if none specified
	//   - Triangulates polygons with more than 3 vertices (fan method)
	//   - Normalizes scene to fit in a 6x6x6 unit box centered at origin
	//   - Handles negative indices (relative to current position)
	// ========================================================================
	bool LoadOBJ(const std::filesystem::path& path);
	
	// ========================================================================
	// LoadMTL
	// ========================================================================
	// Loads materials from a Wavefront MTL file.
	//
	// Parameters:
	//   path - Path to the MTL file
	//
	// Returns:
	//   bool - true if file was loaded successfully
	//
	// Notes:
	//   - Usually called automatically by LoadOBJ via 'mtllib' command
	//   - Can be called manually to load additional material libraries
	//   - Materials are added to the existing material list
	// ========================================================================
	bool LoadMTL(const std::filesystem::path& path);
	
	// ========================================================================
	// GetSceneData
	// ========================================================================
	// Returns a const reference to the loaded scene data.
	//
	// Returns:
	//   const SceneData& - Reference to scene data (materials, triangles, etc.)
	// ========================================================================
	const SceneData& GetSceneData() const { return m_SceneData; }
	
	// ========================================================================
	// UploadToGPU
	// ========================================================================
	// Uploads the scene geometry and materials to GPU textures.
	//
	// Returns:
	//   bool - true if upload was successful
	//
	// Notes:
	//   - Creates four RGBA32F textures (triangles, normals, tri-mat, materials)
	//   - Deletes any previously uploaded textures
	//   - Must be called after LoadOBJ and before BindTextures
	//   - See GPU DATA LAYOUT section for texture format details
	// ========================================================================
	bool UploadToGPU();
	
	// ========================================================================
	// BindTextures
	// ========================================================================
	// Binds the scene textures to shader texture units.
	//
	// Parameters:
	//   shaderProgram - OpenGL shader program handle
	//
	// Notes:
	//   - Binds textures to units 2-5 (0-1 reserved for accumulation)
	//   - Sets the following uniforms:
	//       uTrianglesTex  (sampler2D) - texture unit 2
	//       uNormalsTex    (sampler2D) - texture unit 3
	//       uTriMatTex     (sampler2D) - texture unit 4
	//       uMaterialsTex  (sampler2D) - texture unit 5
	//       uNumTriangles  (int)       - number of triangles
	// ========================================================================
	void BindTextures(GLuint shaderProgram) const;
	
	// ========================================================================
	// GetTriangleCount / GetMaterialCount
	// ========================================================================
	// Returns the number of triangles/materials in the loaded scene.
	// ========================================================================
	size_t GetTriangleCount() const { return m_SceneData.Triangles.size(); }
	size_t GetMaterialCount() const { return m_SceneData.Materials.size(); }
	
	// ========================================================================
	// Clear
	// ========================================================================
	// Clears all loaded data and releases GPU resources.
	//
	// Notes:
	//   - Deletes all GPU textures
	//   - Resets scene data to empty state
	//   - Called automatically by destructor
	// ========================================================================
	void Clear();

private:
	// Parsing helpers
	void ParseOBJLine(const std::string& line);
	void ParseMTLLine(const std::string& line);
	void ProcessFace(const std::vector<std::string>& tokens);
	void ParseFaceVertex(const std::string& token, int& vIdx, int& vtIdx, int& vnIdx);
	int GetMaterialIndex(const std::string& name);
	void NormalizeScene(float targetSize = 6.0f);
	
private:
	SceneData m_SceneData;
	
	// Temporary parsing state
	std::vector<glm::vec3> m_TempVertices;      // v commands
	std::vector<glm::vec3> m_TempNormals;       // vn commands
	std::vector<glm::vec2> m_TempTexCoords;     // vt commands
	std::unordered_map<std::string, int> m_MaterialMap;  // name -> index
	int m_CurrentMaterialIndex = 0;
	std::filesystem::path m_BasePath;
	
	// MTL parsing state
	OBJMaterial* m_CurrentMaterial = nullptr;
	
	// GPU resources (OpenGL texture handles)
	GLuint m_TriangleTexture = 0;    // Triangle vertex positions
	GLuint m_NormalTexture = 0;      // Triangle vertex normals
	GLuint m_MaterialTexture = 0;    // Material properties
	GLuint m_TriMatTexture = 0;      // Triangle-to-material index mapping
	bool m_GPUDataValid = false;
};
