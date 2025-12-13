// ============================================================================
// SCENE MANAGER - Implementation
// ============================================================================
//
// This file implements the OBJ/MTL file parser and GPU upload functionality.
// See SceneManager.h for detailed API documentation.
//
// ARCHITECTURE OVERVIEW:
// ----------------------
//
//   ┌─────────────────────────────────────────────────────────────────────┐
//   │                         OBJ FILE                                    │
//   │  ┌─────────────────────────────────────────────────────────────┐   │
//   │  │ v 1.0 0.0 0.0      # Vertices                               │   │
//   │  │ vn 0.0 1.0 0.0     # Normals                                │   │
//   │  │ mtllib box.mtl     # Material library reference             │   │
//   │  │ usemtl red         # Use material                           │   │
//   │  │ f 1 2 3            # Face (triangle)                        │   │
//   │  └─────────────────────────────────────────────────────────────┘   │
//   └─────────────────────────────────────────────────────────────────────┘
//                                    │
//                                    ▼
//   ┌─────────────────────────────────────────────────────────────────────┐
//   │                      SceneManager::LoadOBJ()                        │
//   │  ┌─────────────────────────────────────────────────────────────┐   │
//   │  │ 1. Read file line by line (FileManager::ReadLines)          │   │
//   │  │ 2. Parse vertices into m_TempVertices                       │   │
//   │  │ 3. Parse normals into m_TempNormals                         │   │
//   │  │ 4. Load MTL file when mtllib encountered                    │   │
//   │  │ 5. Process faces into Triangle structs                      │   │
//   │  │ 6. Normalize scene to fit target size                       │   │
//   │  └─────────────────────────────────────────────────────────────┘   │
//   └─────────────────────────────────────────────────────────────────────┘
//                                    │
//                                    ▼
//   ┌─────────────────────────────────────────────────────────────────────┐
//   │                     SceneManager::UploadToGPU()                     │
//   │  ┌─────────────────────────────────────────────────────────────┐   │
//   │  │ Pack data into float arrays:                                │   │
//   │  │   triangleData[i*12..i*12+11] = V0.xyz, V1.xyz, V2.xyz     │   │
//   │  │   normalData[i*12..i*12+11]   = N0.xyz, N1.xyz, N2.xyz     │   │
//   │  │   triMatData[i*4..i*4+3]      = materialIndex, 0, 0, 0     │   │
//   │  │   materialData[i*12..i*12+11] = albedo, emission, params    │   │
//   │  │                                                             │   │
//   │  │ Create GL_TEXTURE_2D with GL_RGBA32F format                 │   │
//   │  │ Use GL_NEAREST filtering (no interpolation for data)        │   │
//   │  └─────────────────────────────────────────────────────────────┘   │
//   └─────────────────────────────────────────────────────────────────────┘
//                                    │
//                                    ▼
//   ┌─────────────────────────────────────────────────────────────────────┐
//   │                    PathTrace.glsl (Shader)                          │
//   │  ┌─────────────────────────────────────────────────────────────┐   │
//   │  │ Sample textures to retrieve triangle data:                  │   │
//   │  │   vec3 v0 = texture(uTrianglesTex, vec2(0.5/3, (i+0.5)/N))  │   │
//   │  │   vec3 v1 = texture(uTrianglesTex, vec2(1.5/3, (i+0.5)/N))  │   │
//   │  │   vec3 v2 = texture(uTrianglesTex, vec2(2.5/3, (i+0.5)/N))  │   │
//   │  │                                                             │   │
//   │  │ Perform Möller-Trumbore ray-triangle intersection           │   │
//   │  └─────────────────────────────────────────────────────────────┘   │
//   └─────────────────────────────────────────────────────────────────────┘
//
// ============================================================================

#include "SceneManager.h"
#include "FileManager.h"

#include <iostream>
#include <sstream>
#include <algorithm>
#include <cmath>

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

// ----------------------------------------------------------------------------
// Split
// ----------------------------------------------------------------------------
// Splits a string by the given delimiter character.
// Empty tokens are skipped (consecutive delimiters are treated as one).
//
// Example:
//   Split("v 1.0  2.0 3.0", ' ') returns ["v", "1.0", "2.0", "3.0"]
// ----------------------------------------------------------------------------
static std::vector<std::string> Split(const std::string& str, char delimiter = ' ')
{
	std::vector<std::string> tokens;
	std::istringstream stream(str);
	std::string token;
	
	while (std::getline(stream, token, delimiter))
	{
		if (!token.empty())
		{
			tokens.push_back(token);
		}
	}
	
	return tokens;
}

// ----------------------------------------------------------------------------
// Trim
// ----------------------------------------------------------------------------
// Removes leading and trailing whitespace from a string.
// Handles spaces, tabs, carriage returns, and newlines.
// ----------------------------------------------------------------------------
static std::string Trim(const std::string& str)
{
	size_t start = str.find_first_not_of(" \t\r\n");
	size_t end = str.find_last_not_of(" \t\r\n");
	
	if (start == std::string::npos)
		return "";
	
	return str.substr(start, end - start + 1);
}

// ============================================================================
// SCENE MANAGER IMPLEMENTATION
// ============================================================================

SceneManager::~SceneManager()
{
	Clear();
}

// ----------------------------------------------------------------------------
// Clear
// ----------------------------------------------------------------------------
// Releases all resources and resets to initial state.
// Called by destructor and before loading a new scene.
// ----------------------------------------------------------------------------
void SceneManager::Clear()
{
	// Reset scene data
	m_SceneData = SceneData();
	
	// Clear temporary parsing buffers
	m_TempVertices.clear();
	m_TempNormals.clear();
	m_TempTexCoords.clear();
	m_MaterialMap.clear();
	m_CurrentMaterialIndex = 0;
	m_CurrentMaterial = nullptr;
	
	// Delete GPU textures
	if (m_TriangleTexture) glDeleteTextures(1, &m_TriangleTexture);
	if (m_NormalTexture) glDeleteTextures(1, &m_NormalTexture);
	if (m_MaterialTexture) glDeleteTextures(1, &m_MaterialTexture);
	if (m_TriMatTexture) glDeleteTextures(1, &m_TriMatTexture);
	
	m_TriangleTexture = 0;
	m_NormalTexture = 0;
	m_MaterialTexture = 0;
	m_TriMatTexture = 0;
	m_GPUDataValid = false;
}

// ----------------------------------------------------------------------------
// LoadOBJ
// ----------------------------------------------------------------------------
// Main entry point for loading an OBJ file.
//
// Processing steps:
//   1. Read all lines from file
//   2. Create default material (index 0)
//   3. Parse each line (vertices, normals, faces, materials)
//   4. Normalize scene to target size
// ----------------------------------------------------------------------------
bool SceneManager::LoadOBJ(const std::filesystem::path& path)
{
	auto lines = FileManager::ReadLines(path);
	
	if (!lines.has_value())
	{
		std::cerr << "[SceneManager] Failed to load OBJ file: " << path.string() << std::endl;
		return false;
	}
	
	// Store base path for resolving relative MTL paths
	m_BasePath = path;
	
	// Create default material (used when no material is specified)
	OBJMaterial defaultMat;
	defaultMat.Name = "default";
	defaultMat.Albedo = glm::vec3(0.8f);  // Light gray
	m_SceneData.Materials.push_back(defaultMat);
	m_MaterialMap["default"] = 0;
	
	std::cout << "[SceneManager] Loading OBJ: " << path.string() << std::endl;
	
	// Parse each line
	for (const auto& line : lines.value())
	{
		ParseOBJLine(line);
	}
	
	// Normalize scene to fit in a 6x6x6 box centered at origin
	NormalizeScene(6.0f);
	
	std::cout << "[SceneManager] Loaded " << m_SceneData.Triangles.size() << " triangles, "
			  << m_SceneData.Materials.size() << " materials" << std::endl;
	
	return !m_SceneData.Triangles.empty();
}

// ----------------------------------------------------------------------------
// ParseOBJLine
// ----------------------------------------------------------------------------
// Parses a single line from an OBJ file.
//
// Supported commands:
//   v x y z       - Vertex position
//   vn x y z      - Vertex normal  
//   vt u v        - Texture coordinate
//   f v/vt/vn ... - Face definition
//   mtllib file   - Material library
//   usemtl name   - Use material
//   c e t u       - Camera (custom extension)
//   lp v          - Light point (custom extension)
//   g, o, s       - Grouping (ignored)
// ----------------------------------------------------------------------------
void SceneManager::ParseOBJLine(const std::string& line)
{
	std::string trimmed = Trim(line);
	
	// Skip empty lines and comments
	if (trimmed.empty() || trimmed[0] == '#')
		return;
	
	std::vector<std::string> tokens = Split(trimmed);
	
	if (tokens.empty())
		return;
	
	const std::string& cmd = tokens[0];
	
	// ========================================================================
	// VERTEX POSITION: v x y z [w]
	// ========================================================================
	// Defines a vertex in 3D space. The optional w component is ignored.
	// OBJ indices are 1-based, so the first vertex is index 1.
	// ========================================================================
	if (cmd == "v" && tokens.size() >= 4)
	{
		glm::vec3 v;
		v.x = std::stof(tokens[1]);
		v.y = std::stof(tokens[2]);
		v.z = std::stof(tokens[3]);
		m_TempVertices.push_back(v);
	}
	// ========================================================================
	// VERTEX NORMAL: vn x y z
	// ========================================================================
	// Defines a normal vector. Automatically normalized on load.
	// ========================================================================
	else if (cmd == "vn" && tokens.size() >= 4)
	{
		glm::vec3 n;
		n.x = std::stof(tokens[1]);
		n.y = std::stof(tokens[2]);
		n.z = std::stof(tokens[3]);
		m_TempNormals.push_back(glm::normalize(n));
	}
	// ========================================================================
	// TEXTURE COORDINATE: vt u v [w]
	// ========================================================================
	// Defines a texture coordinate. Currently parsed but not used in shader.
	// ========================================================================
	else if (cmd == "vt" && tokens.size() >= 3)
	{
		glm::vec2 t;
		t.x = std::stof(tokens[1]);
		t.y = std::stof(tokens[2]);
		m_TempTexCoords.push_back(t);
	}
	// ========================================================================
	// FACE: f v1[/vt1][/vn1] v2[/vt2][/vn2] v3[/vt3][/vn3] ...
	// ========================================================================
	// Defines a polygonal face. Automatically triangulated if more than 3 vertices.
	// Supports formats: f v, f v/vt, f v/vt/vn, f v//vn
	// ========================================================================
	else if (cmd == "f" && tokens.size() >= 4)
	{
		ProcessFace(tokens);
	}
	// ========================================================================
	// MATERIAL LIBRARY: mtllib filename.mtl
	// ========================================================================
	// References an external material library file.
	// Path is resolved relative to the OBJ file's directory.
	// ========================================================================
	else if (cmd == "mtllib" && tokens.size() >= 2)
	{
		std::filesystem::path mtlPath = FileManager::ResolvePath(m_BasePath, tokens[1]);
		LoadMTL(mtlPath);
	}
	// ========================================================================
	// USE MATERIAL: usemtl material_name
	// ========================================================================
	// Sets the active material for subsequent faces.
	// ========================================================================
	else if (cmd == "usemtl" && tokens.size() >= 2)
	{
		m_CurrentMaterialIndex = GetMaterialIndex(tokens[1]);
	}
	// ========================================================================
	// CAMERA (Custom Extension): c eye_idx target_idx up_idx
	// ========================================================================
	// Defines camera position and orientation using vertex/normal indices.
	// Used in Cornell Box OBJ format.
	// Negative indices are relative to current position (OBJ standard).
	// ========================================================================
	else if (cmd == "c" && tokens.size() >= 4)
	{
		int eyeIdx = std::stoi(tokens[1]);
		int targetIdx = std::stoi(tokens[2]);
		int upIdx = std::stoi(tokens[3]);
		
		// Handle negative indices (relative to end of list)
		if (eyeIdx < 0) eyeIdx = (int)m_TempVertices.size() + eyeIdx + 1;
		if (targetIdx < 0) targetIdx = (int)m_TempVertices.size() + targetIdx + 1;
		
		if (eyeIdx > 0 && eyeIdx <= (int)m_TempVertices.size() &&
			targetIdx > 0 && targetIdx <= (int)m_TempVertices.size())
		{
			m_SceneData.CameraPosition = m_TempVertices[eyeIdx - 1];
			m_SceneData.CameraTarget = m_TempVertices[targetIdx - 1];
			
			// Handle normal index for up vector
			if (upIdx < 0) upIdx = (int)m_TempNormals.size() + upIdx + 1;
			if (upIdx > 0 && upIdx <= (int)m_TempNormals.size())
			{
				m_SceneData.CameraUp = m_TempNormals[upIdx - 1];
			}
			
			m_SceneData.HasCamera = true;
			std::cout << "[SceneManager] Camera found at: " 
					  << m_SceneData.CameraPosition.x << ", "
					  << m_SceneData.CameraPosition.y << ", "
					  << m_SceneData.CameraPosition.z << std::endl;
		}
	}
	// ========================================================================
	// LIGHT POINT (Custom Extension): lp vertex_idx
	// ========================================================================
	// Defines a point light position using a vertex index.
	// Used in Cornell Box OBJ format.
	// ========================================================================
	else if (cmd == "lp" && tokens.size() >= 2)
	{
		int idx = std::stoi(tokens[1]);
		if (idx < 0) idx = (int)m_TempVertices.size() + idx + 1;
		
		if (idx > 0 && idx <= (int)m_TempVertices.size())
		{
			m_SceneData.LightPosition = m_TempVertices[idx - 1];
			m_SceneData.HasLight = true;
			std::cout << "[SceneManager] Light found at: "
					  << m_SceneData.LightPosition.x << ", "
					  << m_SceneData.LightPosition.y << ", "
					  << m_SceneData.LightPosition.z << std::endl;
		}
	}
	// ========================================================================
	// GROUPING COMMANDS: g, o, s
	// ========================================================================
	// These define groups, objects, and smoothing groups.
	// Currently ignored but recognized to avoid "unknown command" warnings.
	// ========================================================================
	else if (cmd == "g" || cmd == "o" || cmd == "s")
	{
		// Grouping commands - ignored for now
	}
}

// ----------------------------------------------------------------------------
// ProcessFace
// ----------------------------------------------------------------------------
// Processes a face definition and creates Triangle structs.
//
// Face triangulation:
//   For polygons with N > 3 vertices, we use fan triangulation:
//   Triangle 1: V0, V1, V2
//   Triangle 2: V0, V2, V3
//   Triangle 3: V0, V3, V4
//   ...
//   This works correctly for convex polygons.
//
// Normal handling:
//   If per-vertex normals are provided (vn), use smooth shading.
//   Otherwise, compute flat face normal from cross product.
// ----------------------------------------------------------------------------
void SceneManager::ProcessFace(const std::vector<std::string>& tokens)
{
	// tokens[0] is "f", rest are vertex definitions
	std::vector<int> vertexIndices;
	std::vector<int> normalIndices;
	
	// Parse each vertex definition
	for (size_t i = 1; i < tokens.size(); ++i)
	{
		int vIdx, vtIdx, vnIdx;
		ParseFaceVertex(tokens[i], vIdx, vtIdx, vnIdx);
		
		// Handle negative indices (relative to current position in list)
		// -1 means last element, -2 means second to last, etc.
		if (vIdx < 0) vIdx = (int)m_TempVertices.size() + vIdx + 1;
		if (vnIdx < 0) vnIdx = (int)m_TempNormals.size() + vnIdx + 1;
		
		vertexIndices.push_back(vIdx);
		normalIndices.push_back(vnIdx);
	}
	
	// Triangulate face using fan method
	// For a polygon with vertices [0,1,2,3,4], creates triangles:
	// (0,1,2), (0,2,3), (0,3,4)
	for (size_t i = 1; i + 1 < vertexIndices.size(); ++i)
	{
		Triangle tri;
		
		// Convert from 1-based OBJ indices to 0-based array indices
		int idx0 = vertexIndices[0] - 1;
		int idx1 = vertexIndices[i] - 1;
		int idx2 = vertexIndices[i + 1] - 1;
		
		// Validate vertex indices
		if (idx0 < 0 || idx0 >= (int)m_TempVertices.size() ||
			idx1 < 0 || idx1 >= (int)m_TempVertices.size() ||
			idx2 < 0 || idx2 >= (int)m_TempVertices.size())
		{
			std::cerr << "[SceneManager] Invalid vertex index in face" << std::endl;
			continue;
		}
		
		// Store vertex positions
		tri.V0 = m_TempVertices[idx0];
		tri.V1 = m_TempVertices[idx1];
		tri.V2 = m_TempVertices[idx2];
		
		// Handle normals
		int nIdx0 = normalIndices[0] - 1;
		int nIdx1 = normalIndices[i] - 1;
		int nIdx2 = normalIndices[i + 1] - 1;
		
		if (nIdx0 >= 0 && nIdx0 < (int)m_TempNormals.size() &&
			nIdx1 >= 0 && nIdx1 < (int)m_TempNormals.size() &&
			nIdx2 >= 0 && nIdx2 < (int)m_TempNormals.size())
		{
			// Use per-vertex normals for smooth shading
			tri.N0 = m_TempNormals[nIdx0];
			tri.N1 = m_TempNormals[nIdx1];
			tri.N2 = m_TempNormals[nIdx2];
		}
		else
		{
			// Compute face normal for flat shading
			// Normal = normalize(cross(V1-V0, V2-V0))
			glm::vec3 edge1 = tri.V1 - tri.V0;
			glm::vec3 edge2 = tri.V2 - tri.V0;
			glm::vec3 faceNormal = glm::normalize(glm::cross(edge1, edge2));
			tri.N0 = tri.N1 = tri.N2 = faceNormal;
		}
		
		// Assign current material
		tri.MaterialIndex = m_CurrentMaterialIndex;
		m_SceneData.Triangles.push_back(tri);
	}
}

// ----------------------------------------------------------------------------
// ParseFaceVertex
// ----------------------------------------------------------------------------
// Parses a face vertex definition in the format: v[/vt][/vn]
//
// Supported formats:
//   "1"        -> vIdx=1, vtIdx=0, vnIdx=0
//   "1/2"      -> vIdx=1, vtIdx=2, vnIdx=0
//   "1/2/3"    -> vIdx=1, vtIdx=2, vnIdx=3
//   "1//3"     -> vIdx=1, vtIdx=0, vnIdx=3
// ----------------------------------------------------------------------------
void SceneManager::ParseFaceVertex(const std::string& token, int& vIdx, int& vtIdx, int& vnIdx)
{
	vIdx = vtIdx = vnIdx = 0;
	
	std::vector<std::string> parts = Split(token, '/');
	
	if (parts.size() >= 1 && !parts[0].empty())
		vIdx = std::stoi(parts[0]);
	
	if (parts.size() >= 2 && !parts[1].empty())
		vtIdx = std::stoi(parts[1]);
	
	if (parts.size() >= 3 && !parts[2].empty())
		vnIdx = std::stoi(parts[2]);
}

// ----------------------------------------------------------------------------
// GetMaterialIndex
// ----------------------------------------------------------------------------
// Looks up a material by name and returns its index.
// Returns 0 (default material) if the material is not found.
// ----------------------------------------------------------------------------
int SceneManager::GetMaterialIndex(const std::string& name)
{
	auto it = m_MaterialMap.find(name);
	if (it != m_MaterialMap.end())
	{
		return it->second;
	}
	
	// Material not found - use default
	std::cerr << "[SceneManager] Warning: Material '" << name << "' not found, using default" << std::endl;
	return 0;
}

// ----------------------------------------------------------------------------
// LoadMTL
// ----------------------------------------------------------------------------
// Loads materials from a Wavefront MTL file.
// Called automatically when 'mtllib' is encountered in OBJ file.
// ----------------------------------------------------------------------------
bool SceneManager::LoadMTL(const std::filesystem::path& path)
{
	auto lines = FileManager::ReadLines(path);
	
	if (!lines.has_value())
	{
		std::cerr << "[SceneManager] Failed to load MTL file: " << path.string() << std::endl;
		return false;
	}
	
	std::cout << "[SceneManager] Loading MTL: " << path.string() << std::endl;
	
	for (const auto& line : lines.value())
	{
		ParseMTLLine(line);
	}
	
	m_CurrentMaterial = nullptr;
	return true;
}

// ----------------------------------------------------------------------------
// ParseMTLLine
// ----------------------------------------------------------------------------
// Parses a single line from an MTL file.
//
// Supported commands:
//   newmtl name  - Define new material
//   Kd r g b     - Diffuse color -> Albedo
//   Ke r g b     - Emissive color -> Emission
//   Ns value     - Specular exponent -> Roughness (inverse)
//   Ni value     - Index of refraction -> IOR
//   d value      - Dissolve (opacity) -> Transmission
//   Tr value     - Transparency -> Transmission
//   illum model  - Illumination model -> Metallic/Transmission
//
// MTL to PBR Conversion:
//   Roughness = 1.0 - (Ns / 1000.0)
//   Metallic  = 1.0 if illum == 3 (mirror)
//   Transmission = 1.0 - d, or 1.0 if illum == 7 (glass)
// ----------------------------------------------------------------------------
void SceneManager::ParseMTLLine(const std::string& line)
{
	std::string trimmed = Trim(line);
	
	// Skip empty lines and comments
	if (trimmed.empty() || trimmed[0] == '#')
		return;
	
	std::vector<std::string> tokens = Split(trimmed);
	
	if (tokens.empty())
		return;
	
	const std::string& cmd = tokens[0];
	
	// ========================================================================
	// NEW MATERIAL: newmtl name
	// ========================================================================
	if (cmd == "newmtl" && tokens.size() >= 2)
	{
		OBJMaterial mat;
		mat.Name = tokens[1];
		m_SceneData.Materials.push_back(mat);
		m_MaterialMap[mat.Name] = (int)m_SceneData.Materials.size() - 1;
		m_CurrentMaterial = &m_SceneData.Materials.back();
		
		std::cout << "[SceneManager] Material: " << mat.Name << std::endl;
	}
	else if (m_CurrentMaterial)
	{
		// ====================================================================
		// DIFFUSE COLOR: Kd r g b
		// ====================================================================
		if (cmd == "Kd" && tokens.size() >= 4)
		{
			m_CurrentMaterial->Albedo.r = std::stof(tokens[1]);
			m_CurrentMaterial->Albedo.g = std::stof(tokens[2]);
			m_CurrentMaterial->Albedo.b = std::stof(tokens[3]);
		}
		// ====================================================================
		// EMISSIVE COLOR: Ke r g b
		// ====================================================================
		else if (cmd == "Ke" && tokens.size() >= 4)
		{
			m_CurrentMaterial->Emission.r = std::stof(tokens[1]);
			m_CurrentMaterial->Emission.g = std::stof(tokens[2]);
			m_CurrentMaterial->Emission.b = std::stof(tokens[3]);
			
			// Auto-calculate emission strength from color magnitude
			float emissionMagnitude = glm::length(m_CurrentMaterial->Emission);
			if (emissionMagnitude > 0.01f)
			{
				m_CurrentMaterial->EmissionStrength = emissionMagnitude * 10.0f;
			}
		}
		// ====================================================================
		// SPECULAR EXPONENT: Ns value
		// ====================================================================
		// Ns ranges from 0 (rough) to 1000 (mirror-like).
		// Convert to roughness: roughness = 1.0 - (Ns / 1000.0)
		// Clamp to minimum 0.04 to avoid numerical issues.
		// ====================================================================
		else if (cmd == "Ns" && tokens.size() >= 2)
		{
			float ns = std::stof(tokens[1]);
			m_CurrentMaterial->Roughness = 1.0f - std::min(ns / 1000.0f, 1.0f);
			m_CurrentMaterial->Roughness = std::max(m_CurrentMaterial->Roughness, 0.04f);
		}
		// ====================================================================
		// INDEX OF REFRACTION: Ni value
		// ====================================================================
		// Typical values: air=1.0, water=1.33, glass=1.5, diamond=2.4
		// ====================================================================
		else if (cmd == "Ni" && tokens.size() >= 2)
		{
			m_CurrentMaterial->IOR = std::stof(tokens[1]);
		}
		// ====================================================================
		// DISSOLVE/TRANSPARENCY: d value or Tr value
		// ====================================================================
		// d = opacity (1.0 = fully opaque, 0.0 = fully transparent)
		// Tr = transparency (1.0 = fully transparent, 0.0 = fully opaque)
		// ====================================================================
		else if ((cmd == "d" || cmd == "Tr") && tokens.size() >= 2)
		{
			float value = std::stof(tokens[1]);
			// Convert Tr to d (they're inverses)
			if (cmd == "Tr")
				value = 1.0f - value;
			
			// Set transmission if not fully opaque
			if (value < 0.99f)
			{
				m_CurrentMaterial->Transmission = 1.0f - value;
			}
		}
		// ====================================================================
		// ILLUMINATION MODEL: illum value
		// ====================================================================
		// illum 0: Color on, ambient off
		// illum 1: Color on, ambient on
		// illum 2: Highlight on (default)
		// illum 3: Reflection on, ray trace on (MIRROR)
		// illum 4: Glass on, ray trace on
		// illum 5: Fresnel on, ray trace on
		// illum 6: Refraction on, fresnel off, ray trace on
		// illum 7: Refraction on, fresnel on, ray trace on (GLASS)
		// ====================================================================
		else if (cmd == "illum" && tokens.size() >= 2)
		{
			int illum = std::stoi(tokens[1]);
			if (illum == 3)
			{
				// Mirror-like reflection
				m_CurrentMaterial->Metallic = 1.0f;
				m_CurrentMaterial->Roughness = 0.1f;
			}
			else if (illum == 7)
			{
				// Glass with refraction
				m_CurrentMaterial->Transmission = 1.0f;
			}
		}
	}
}

// ----------------------------------------------------------------------------
// NormalizeScene
// ----------------------------------------------------------------------------
// Scales and centers the scene to fit within a target size.
//
// This ensures that models of any size render consistently in the path tracer.
// The scene is centered at the origin and scaled so the largest dimension
// equals targetSize.
//
// Also transforms camera and light positions if they were defined in the OBJ.
// ----------------------------------------------------------------------------
void SceneManager::NormalizeScene(float targetSize)
{
	if (m_SceneData.Triangles.empty())
		return;
	
	// Calculate axis-aligned bounding box
	glm::vec3 minBounds(FLT_MAX);
	glm::vec3 maxBounds(-FLT_MAX);
	
	for (const auto& tri : m_SceneData.Triangles)
	{
		minBounds = glm::min(minBounds, tri.V0);
		minBounds = glm::min(minBounds, tri.V1);
		minBounds = glm::min(minBounds, tri.V2);
		
		maxBounds = glm::max(maxBounds, tri.V0);
		maxBounds = glm::max(maxBounds, tri.V1);
		maxBounds = glm::max(maxBounds, tri.V2);
	}
	
	// Calculate center and scale factor
	glm::vec3 center = (minBounds + maxBounds) * 0.5f;
	glm::vec3 size = maxBounds - minBounds;
	float maxDim = std::max({size.x, size.y, size.z});
	
	// Avoid division by zero for degenerate meshes
	if (maxDim < 0.0001f)
		return;
	
	float scale = targetSize / maxDim;
	
	std::cout << "[SceneManager] Normalizing scene: center=(" 
			  << center.x << ", " << center.y << ", " << center.z 
			  << "), scale=" << scale << std::endl;
	
	// Transform all triangle vertices
	for (auto& tri : m_SceneData.Triangles)
	{
		tri.V0 = (tri.V0 - center) * scale;
		tri.V1 = (tri.V1 - center) * scale;
		tri.V2 = (tri.V2 - center) * scale;
	}
	
	// Transform camera position and target
	if (m_SceneData.HasCamera)
	{
		m_SceneData.CameraPosition = (m_SceneData.CameraPosition - center) * scale;
		m_SceneData.CameraTarget = (m_SceneData.CameraTarget - center) * scale;
	}
	
	// Transform light position
	if (m_SceneData.HasLight)
	{
		m_SceneData.LightPosition = (m_SceneData.LightPosition - center) * scale;
	}
}

// ============================================================================
// GPU UPLOAD AND BINDING
// ============================================================================

// ----------------------------------------------------------------------------
// UploadToGPU
// ----------------------------------------------------------------------------
// Packs scene data into GPU textures for shader access.
//
// TEXTURE LAYOUT:
//
// uTrianglesTex (3 x numTriangles):
//   ┌─────────────┬─────────────┬─────────────┐
//   │ V0.xyz (0,0)│ V1.xyz (1,0)│ V2.xyz (2,0)│  Triangle 0
//   ├─────────────┼─────────────┼─────────────┤
//   │ V0.xyz (0,1)│ V1.xyz (1,1)│ V2.xyz (2,1)│  Triangle 1
//   ├─────────────┼─────────────┼─────────────┤
//   │     ...     │     ...     │     ...     │
//   └─────────────┴─────────────┴─────────────┘
//
// uMaterialsTex (3 x numMaterials):
//   ┌─────────────────┬─────────────────┬─────────────────┐
//   │ albedo.rgb,     │ emission.rgb,   │ emissionStr,    │  Material 0
//   │ roughness       │ metallic        │ ior, trans, 0   │
//   ├─────────────────┼─────────────────┼─────────────────┤
//   │     ...         │     ...         │     ...         │
//   └─────────────────┴─────────────────┴─────────────────┘
// ----------------------------------------------------------------------------
bool SceneManager::UploadToGPU()
{
	if (m_SceneData.Triangles.empty())
	{
		std::cerr << "[SceneManager] No triangles to upload" << std::endl;
		return false;
	}
	
	// Delete any existing textures
	if (m_TriangleTexture) glDeleteTextures(1, &m_TriangleTexture);
	if (m_NormalTexture) glDeleteTextures(1, &m_NormalTexture);
	if (m_MaterialTexture) glDeleteTextures(1, &m_MaterialTexture);
	if (m_TriMatTexture) glDeleteTextures(1, &m_TriMatTexture);
	
	size_t numTriangles = m_SceneData.Triangles.size();
	
	// Allocate CPU-side buffers for texture data
	// Layout: 3 pixels per row (V0, V1, V2), numTriangles rows
	// Each pixel is RGBA (4 floats)
	std::vector<float> triangleData(numTriangles * 3 * 4);
	std::vector<float> normalData(numTriangles * 3 * 4);
	std::vector<float> triMatData(numTriangles * 4);
	
	// Pack triangle data into texture format
	for (size_t i = 0; i < numTriangles; ++i)
	{
		const Triangle& tri = m_SceneData.Triangles[i];
		
		// Vertex positions (3 vertices * 4 components each)
		size_t baseIdx = i * 3 * 4;
		
		// Vertex 0
		triangleData[baseIdx + 0] = tri.V0.x;
		triangleData[baseIdx + 1] = tri.V0.y;
		triangleData[baseIdx + 2] = tri.V0.z;
		triangleData[baseIdx + 3] = 1.0f;  // w component (padding)
		
		// Vertex 1
		triangleData[baseIdx + 4] = tri.V1.x;
		triangleData[baseIdx + 5] = tri.V1.y;
		triangleData[baseIdx + 6] = tri.V1.z;
		triangleData[baseIdx + 7] = 1.0f;
		
		// Vertex 2
		triangleData[baseIdx + 8] = tri.V2.x;
		triangleData[baseIdx + 9] = tri.V2.y;
		triangleData[baseIdx + 10] = tri.V2.z;
		triangleData[baseIdx + 11] = 1.0f;
		
		// Normal vectors (same layout)
		normalData[baseIdx + 0] = tri.N0.x;
		normalData[baseIdx + 1] = tri.N0.y;
		normalData[baseIdx + 2] = tri.N0.z;
		normalData[baseIdx + 3] = 0.0f;
		
		normalData[baseIdx + 4] = tri.N1.x;
		normalData[baseIdx + 5] = tri.N1.y;
		normalData[baseIdx + 6] = tri.N1.z;
		normalData[baseIdx + 7] = 0.0f;
		
		normalData[baseIdx + 8] = tri.N2.x;
		normalData[baseIdx + 9] = tri.N2.y;
		normalData[baseIdx + 10] = tri.N2.z;
		normalData[baseIdx + 11] = 0.0f;
		
		// Material index (1 pixel per triangle)
		size_t matIdx = i * 4;
		triMatData[matIdx + 0] = (float)tri.MaterialIndex;
		triMatData[matIdx + 1] = 0.0f;
		triMatData[matIdx + 2] = 0.0f;
		triMatData[matIdx + 3] = 0.0f;
	}
	
	// Create triangle vertex texture
	glGenTextures(1, &m_TriangleTexture);
	glBindTexture(GL_TEXTURE_2D, m_TriangleTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 3, (GLsizei)numTriangles, 0, GL_RGBA, GL_FLOAT, triangleData.data());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);  // No interpolation!
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	
	// Create normal texture
	glGenTextures(1, &m_NormalTexture);
	glBindTexture(GL_TEXTURE_2D, m_NormalTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 3, (GLsizei)numTriangles, 0, GL_RGBA, GL_FLOAT, normalData.data());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	
	// Create triangle-to-material index texture
	glGenTextures(1, &m_TriMatTexture);
	glBindTexture(GL_TEXTURE_2D, m_TriMatTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 1, (GLsizei)numTriangles, 0, GL_RGBA, GL_FLOAT, triMatData.data());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	
	// Pack and create material texture
	size_t numMaterials = m_SceneData.Materials.size();
	std::vector<float> materialData(numMaterials * 3 * 4);
	
	for (size_t i = 0; i < numMaterials; ++i)
	{
		const OBJMaterial& mat = m_SceneData.Materials[i];
		size_t baseIdx = i * 3 * 4;
		
		// Row 0: albedo.rgb + roughness
		materialData[baseIdx + 0] = mat.Albedo.r;
		materialData[baseIdx + 1] = mat.Albedo.g;
		materialData[baseIdx + 2] = mat.Albedo.b;
		materialData[baseIdx + 3] = mat.Roughness;
		
		// Row 1: emission.rgb + metallic
		materialData[baseIdx + 4] = mat.Emission.r;
		materialData[baseIdx + 5] = mat.Emission.g;
		materialData[baseIdx + 6] = mat.Emission.b;
		materialData[baseIdx + 7] = mat.Metallic;
		
		// Row 2: emissionStrength + ior + transmission + padding
		materialData[baseIdx + 8] = mat.EmissionStrength;
		materialData[baseIdx + 9] = mat.IOR;
		materialData[baseIdx + 10] = mat.Transmission;
		materialData[baseIdx + 11] = 0.0f;
	}
	
	glGenTextures(1, &m_MaterialTexture);
	glBindTexture(GL_TEXTURE_2D, m_MaterialTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 3, (GLsizei)numMaterials, 0, GL_RGBA, GL_FLOAT, materialData.data());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	
	glBindTexture(GL_TEXTURE_2D, 0);
	
	m_GPUDataValid = true;
	
	std::cout << "[SceneManager] Uploaded to GPU: " << numTriangles << " triangles, " 
			  << numMaterials << " materials" << std::endl;
	
	return true;
}

// ----------------------------------------------------------------------------
// BindTextures
// ----------------------------------------------------------------------------
// Binds the scene textures to the shader for rendering.
//
// Texture unit assignments:
//   Unit 0, 1: Reserved for accumulation buffers
//   Unit 2: uTrianglesTex (triangle vertices)
//   Unit 3: uNormalsTex (triangle normals)
//   Unit 4: uTriMatTex (triangle material indices)
//   Unit 5: uMaterialsTex (material properties)
// ----------------------------------------------------------------------------
void SceneManager::BindTextures(GLuint shaderProgram) const
{
	if (!m_GPUDataValid)
		return;
	
	// Bind triangle vertex texture to unit 2
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, m_TriangleTexture);
	glUniform1i(glGetUniformLocation(shaderProgram, "uTrianglesTex"), 2);
	
	// Bind normal texture to unit 3
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, m_NormalTexture);
	glUniform1i(glGetUniformLocation(shaderProgram, "uNormalsTex"), 3);
	
	// Bind triangle material texture to unit 4
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, m_TriMatTexture);
	glUniform1i(glGetUniformLocation(shaderProgram, "uTriMatTex"), 4);
	
	// Bind material texture to unit 5
	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_2D, m_MaterialTexture);
	glUniform1i(glGetUniformLocation(shaderProgram, "uMaterialsTex"), 5);
	
	// Pass triangle count to shader
	glUniform1i(glGetUniformLocation(shaderProgram, "uNumTriangles"), (GLint)m_SceneData.Triangles.size());
}
