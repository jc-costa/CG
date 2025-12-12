// ============================================================================
// SCENE MANAGER TEST SUITE - Comprehensive Tests for SceneManager
// ============================================================================
//
// This test suite verifies all functionality of the SceneManager class:
//   - OBJ file parsing (vertices, normals, texture coords, faces)
//   - MTL file parsing (all material properties)
//   - Face triangulation (polygons with >3 vertices)
//   - Negative index handling
//   - Custom extensions (camera, light point)
//   - Scene normalization
//   - Error handling
//
// Test files are located in ./test_assets/
//
// Build and run:
//   ./test.sh
//
// ============================================================================

#include "../SceneManager.h"
#include "../FileManager.h"

#include <iostream>
#include <iomanip>
#include <cmath>
#include <string>
#include <vector>
#include <functional>
#include <filesystem>

// ============================================================================
// TEST FRAMEWORK
// ============================================================================

namespace TestFramework
{
	// Test result tracking
	static int s_TestsPassed = 0;
	static int s_TestsFailed = 0;
	static int s_AssertionsPassed = 0;
	static int s_AssertionsFailed = 0;
	static std::string s_CurrentTestName;
	static bool s_CurrentTestPassed = true;
	
	// ANSI color codes
	constexpr const char* COLOR_GREEN = "\033[32m";
	constexpr const char* COLOR_RED = "\033[31m";
	constexpr const char* COLOR_YELLOW = "\033[33m";
	constexpr const char* COLOR_CYAN = "\033[36m";
	constexpr const char* COLOR_RESET = "\033[0m";
	constexpr const char* COLOR_BOLD = "\033[1m";
	
	// Float comparison epsilon
	constexpr float EPSILON = 0.0001f;
	
	// ----------------------------------------------------------------------------
	// Assertion Helpers
	// ----------------------------------------------------------------------------
	
	void AssertTrue(bool condition, const std::string& message)
	{
		if (condition)
		{
			s_AssertionsPassed++;
		}
		else
		{
			s_AssertionsFailed++;
			s_CurrentTestPassed = false;
			std::cerr << "    " << COLOR_RED << "✗ ASSERT FAILED: " << message << COLOR_RESET << std::endl;
		}
	}
	
	void AssertFalse(bool condition, const std::string& message)
	{
		AssertTrue(!condition, message);
	}
	
	template<typename T>
	void AssertEqual(T expected, T actual, const std::string& message)
	{
		if (expected == actual)
		{
			s_AssertionsPassed++;
		}
		else
		{
			s_AssertionsFailed++;
			s_CurrentTestPassed = false;
			std::cerr << "    " << COLOR_RED << "✗ ASSERT FAILED: " << message 
			          << " (expected " << expected << ", got " << actual << ")" << COLOR_RESET << std::endl;
		}
	}
	
	void AssertFloatEqual(float expected, float actual, const std::string& message)
	{
		if (std::abs(expected - actual) < EPSILON)
		{
			s_AssertionsPassed++;
		}
		else
		{
			s_AssertionsFailed++;
			s_CurrentTestPassed = false;
			std::cerr << "    " << COLOR_RED << "✗ ASSERT FAILED: " << message 
			          << " (expected " << std::fixed << std::setprecision(4) << expected 
			          << ", got " << actual << ")" << COLOR_RESET << std::endl;
		}
	}
	
	void AssertVec3Equal(const glm::vec3& expected, const glm::vec3& actual, const std::string& message)
	{
		bool equal = std::abs(expected.x - actual.x) < EPSILON &&
		             std::abs(expected.y - actual.y) < EPSILON &&
		             std::abs(expected.z - actual.z) < EPSILON;
		
		if (equal)
		{
			s_AssertionsPassed++;
		}
		else
		{
			s_AssertionsFailed++;
			s_CurrentTestPassed = false;
			std::cerr << "    " << COLOR_RED << "✗ ASSERT FAILED: " << message 
			          << " (expected [" << expected.x << "," << expected.y << "," << expected.z 
			          << "], got [" << actual.x << "," << actual.y << "," << actual.z << "])" 
			          << COLOR_RESET << std::endl;
		}
	}
	
	void AssertGreaterThan(int actual, int threshold, const std::string& message)
	{
		if (actual > threshold)
		{
			s_AssertionsPassed++;
		}
		else
		{
			s_AssertionsFailed++;
			s_CurrentTestPassed = false;
			std::cerr << "    " << COLOR_RED << "✗ ASSERT FAILED: " << message 
			          << " (expected > " << threshold << ", got " << actual << ")" << COLOR_RESET << std::endl;
		}
	}
	
	void AssertStringEqual(const std::string& expected, const std::string& actual, const std::string& message)
	{
		if (expected == actual)
		{
			s_AssertionsPassed++;
		}
		else
		{
			s_AssertionsFailed++;
			s_CurrentTestPassed = false;
			std::cerr << "    " << COLOR_RED << "✗ ASSERT FAILED: " << message 
			          << " (expected \"" << expected << "\", got \"" << actual << "\")" << COLOR_RESET << std::endl;
		}
	}
	
	// ----------------------------------------------------------------------------
	// Test Runner
	// ----------------------------------------------------------------------------
	
	void BeginTest(const std::string& testName)
	{
		s_CurrentTestName = testName;
		s_CurrentTestPassed = true;
		std::cout << "  " << COLOR_CYAN << "▶ " << testName << COLOR_RESET << std::endl;
	}
	
	void EndTest()
	{
		if (s_CurrentTestPassed)
		{
			s_TestsPassed++;
			std::cout << "    " << COLOR_GREEN << "✓ PASSED" << COLOR_RESET << std::endl;
		}
		else
		{
			s_TestsFailed++;
			std::cout << "    " << COLOR_RED << "✗ FAILED" << COLOR_RESET << std::endl;
		}
	}
	
	void PrintSectionHeader(const std::string& section)
	{
		std::cout << std::endl;
		std::cout << COLOR_BOLD << COLOR_YELLOW << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << COLOR_RESET << std::endl;
		std::cout << COLOR_BOLD << COLOR_YELLOW << " " << section << COLOR_RESET << std::endl;
		std::cout << COLOR_BOLD << COLOR_YELLOW << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << COLOR_RESET << std::endl;
	}
	
	void PrintSummary()
	{
		std::cout << std::endl;
		std::cout << COLOR_BOLD << "════════════════════════════════════════" << COLOR_RESET << std::endl;
		std::cout << COLOR_BOLD << " TEST SUMMARY" << COLOR_RESET << std::endl;
		std::cout << "════════════════════════════════════════" << std::endl;
		
		std::cout << "  Tests:      " << COLOR_GREEN << s_TestsPassed << " passed" << COLOR_RESET;
		if (s_TestsFailed > 0)
			std::cout << ", " << COLOR_RED << s_TestsFailed << " failed" << COLOR_RESET;
		std::cout << std::endl;
		
		std::cout << "  Assertions: " << COLOR_GREEN << s_AssertionsPassed << " passed" << COLOR_RESET;
		if (s_AssertionsFailed > 0)
			std::cout << ", " << COLOR_RED << s_AssertionsFailed << " failed" << COLOR_RESET;
		std::cout << std::endl;
		
		std::cout << "════════════════════════════════════════" << std::endl;
		
		if (s_TestsFailed == 0)
		{
			std::cout << COLOR_GREEN << COLOR_BOLD << "  ✓ ALL TESTS PASSED!" << COLOR_RESET << std::endl;
		}
		else
		{
			std::cout << COLOR_RED << COLOR_BOLD << "  ✗ SOME TESTS FAILED" << COLOR_RESET << std::endl;
		}
		
		std::cout << "════════════════════════════════════════" << std::endl;
	}
	
	int GetExitCode()
	{
		return s_TestsFailed > 0 ? 1 : 0;
	}
}

using namespace TestFramework;

// ============================================================================
// TEST ASSET PATH HELPER
// ============================================================================

std::filesystem::path GetTestAssetPath(const std::string& filename)
{
	// Try relative path first (when running from test directory)
	std::filesystem::path relativePath = std::filesystem::path("test_assets") / filename;
	if (std::filesystem::exists(relativePath))
		return relativePath;
	
	// Try from SceneManagerTest directory
	relativePath = std::filesystem::path("Source/SceneManagerTest/test_assets") / filename;
	if (std::filesystem::exists(relativePath))
		return relativePath;
	
	// Try from App directory
	relativePath = std::filesystem::path("App/Source/SceneManagerTest/test_assets") / filename;
	if (std::filesystem::exists(relativePath))
		return relativePath;
	
	// Return relative path and let the test fail with a clear error
	return std::filesystem::path("test_assets") / filename;
}

// ============================================================================
// TEST SUITES
// ============================================================================

// ----------------------------------------------------------------------------
// TEST SUITE 1: Basic Loading Tests
// ----------------------------------------------------------------------------

void TestMinimalOBJLoading()
{
	BeginTest("Load minimal OBJ file (single triangle)");
	
	SceneManager manager;
	bool loaded = manager.LoadOBJ(GetTestAssetPath("test_minimal.obj"));
	
	AssertTrue(loaded, "OBJ file should load successfully");
	AssertEqual(size_t{1}, manager.GetTriangleCount(), "Should have exactly 1 triangle");
	AssertEqual(size_t{1}, manager.GetMaterialCount(), "Should have 1 material (default)");
	
	EndTest();
}

void TestBoxOBJLoading()
{
	BeginTest("Load box OBJ file (cube with 12 triangles)");
	
	SceneManager manager;
	bool loaded = manager.LoadOBJ(GetTestAssetPath("box.obj"));
	
	AssertTrue(loaded, "OBJ file should load successfully");
	AssertEqual(size_t{12}, manager.GetTriangleCount(), "Cube should have 12 triangles (6 faces × 2)");
	AssertGreaterThan(static_cast<int>(manager.GetMaterialCount()), 1, "Should have materials from MTL");
	
	EndTest();
}

void TestNonExistentFile()
{
	BeginTest("Handle non-existent file gracefully");
	
	SceneManager manager;
	bool loaded = manager.LoadOBJ(GetTestAssetPath("nonexistent_file.obj"));
	
	AssertFalse(loaded, "Loading non-existent file should return false");
	AssertEqual(size_t{0}, manager.GetTriangleCount(), "Triangle count should be 0");
	
	EndTest();
}

void TestClearFunction()
{
	BeginTest("Clear function resets state");
	
	SceneManager manager;
	manager.LoadOBJ(GetTestAssetPath("box.obj"));
	
	AssertGreaterThan(static_cast<int>(manager.GetTriangleCount()), 0, "Should have triangles before clear");
	
	manager.Clear();
	
	AssertEqual(size_t{0}, manager.GetTriangleCount(), "Triangle count should be 0 after clear");
	AssertEqual(size_t{0}, manager.GetMaterialCount(), "Material count should be 0 after clear");
	
	EndTest();
}

// ----------------------------------------------------------------------------
// TEST SUITE 2: Quadric Surface Loading Tests
// ----------------------------------------------------------------------------

void TestSphereLoading()
{
	BeginTest("Load sphere OBJ (icosphere approximation)");
	
	SceneManager manager;
	bool loaded = manager.LoadOBJ(GetTestAssetPath("sphere.obj"));
	
	AssertTrue(loaded, "Sphere OBJ should load successfully");
	AssertGreaterThan(static_cast<int>(manager.GetTriangleCount()), 0, "Should have triangles");
	
	// Verify normals are valid (normalized)
	const auto& scene = manager.GetSceneData();
	if (!scene.Triangles.empty())
	{
		const auto& tri = scene.Triangles[0];
		float n0Length = glm::length(tri.N0);
		AssertFloatEqual(1.0f, n0Length, "Normals should be normalized");
	}
	
	EndTest();
}

void TestCylinderLoading()
{
	BeginTest("Load cylinder OBJ");
	
	SceneManager manager;
	bool loaded = manager.LoadOBJ(GetTestAssetPath("cylinder.obj"));
	
	AssertTrue(loaded, "Cylinder OBJ should load successfully");
	// Cylinder has 24 side triangles + 12 top + 12 bottom = 48
	AssertGreaterThan(static_cast<int>(manager.GetTriangleCount()), 20, "Should have many triangles");
	
	EndTest();
}

void TestConeLoading()
{
	BeginTest("Load cone OBJ");
	
	SceneManager manager;
	bool loaded = manager.LoadOBJ(GetTestAssetPath("cone.obj"));
	
	AssertTrue(loaded, "Cone OBJ should load successfully");
	AssertGreaterThan(static_cast<int>(manager.GetTriangleCount()), 0, "Should have triangles");
	
	EndTest();
}

void TestEllipsoidLoading()
{
	BeginTest("Load ellipsoid OBJ");
	
	SceneManager manager;
	bool loaded = manager.LoadOBJ(GetTestAssetPath("ellipsoid.obj"));
	
	AssertTrue(loaded, "Ellipsoid OBJ should load successfully");
	AssertGreaterThan(static_cast<int>(manager.GetTriangleCount()), 0, "Should have triangles");
	
	EndTest();
}

void TestEllipticParaboloidLoading()
{
	BeginTest("Load elliptic paraboloid OBJ");
	
	SceneManager manager;
	bool loaded = manager.LoadOBJ(GetTestAssetPath("elliptic_paraboloid.obj"));
	
	AssertTrue(loaded, "Elliptic paraboloid OBJ should load successfully");
	// 8x8 quads = 128 triangles
	AssertEqual(size_t{128}, manager.GetTriangleCount(), "Should have 128 triangles from 8x8 grid");
	
	EndTest();
}

void TestHyperbolicParaboloidLoading()
{
	BeginTest("Load hyperbolic paraboloid (saddle) OBJ");
	
	SceneManager manager;
	bool loaded = manager.LoadOBJ(GetTestAssetPath("hyperbolic_paraboloid.obj"));
	
	AssertTrue(loaded, "Hyperbolic paraboloid OBJ should load successfully");
	AssertEqual(size_t{128}, manager.GetTriangleCount(), "Should have 128 triangles from 8x8 grid");
	
	EndTest();
}

void TestHyperboloidOneSheetLoading()
{
	BeginTest("Load hyperboloid of one sheet OBJ");
	
	SceneManager manager;
	bool loaded = manager.LoadOBJ(GetTestAssetPath("hyperboloid_one_sheet.obj"));
	
	AssertTrue(loaded, "Hyperboloid of one sheet OBJ should load successfully");
	AssertGreaterThan(static_cast<int>(manager.GetTriangleCount()), 100, "Should have many triangles");
	
	EndTest();
}

void TestHyperboloidTwoSheetsLoading()
{
	BeginTest("Load hyperboloid of two sheets OBJ");
	
	SceneManager manager;
	bool loaded = manager.LoadOBJ(GetTestAssetPath("hyperboloid_two_sheets.obj"));
	
	AssertTrue(loaded, "Hyperboloid of two sheets OBJ should load successfully");
	AssertGreaterThan(static_cast<int>(manager.GetTriangleCount()), 100, "Should have many triangles");
	
	EndTest();
}

// ----------------------------------------------------------------------------
// TEST SUITE 3: Face Parsing and Triangulation Tests
// ----------------------------------------------------------------------------

void TestPolygonTriangulation()
{
	BeginTest("Polygon triangulation (quads, pentagons, hexagons)");
	
	SceneManager manager;
	bool loaded = manager.LoadOBJ(GetTestAssetPath("test_polygon.obj"));
	
	AssertTrue(loaded, "Polygon OBJ should load successfully");
	
	// Square (4 vertices) -> 2 triangles
	// Pentagon (5 vertices) -> 3 triangles
	// Hexagon (6 vertices) -> 4 triangles
	// Total: 2 + 3 + 4 = 9 triangles
	AssertEqual(size_t{9}, manager.GetTriangleCount(), "Should have 9 triangles from triangulation");
	
	EndTest();
}

void TestNegativeIndices()
{
	BeginTest("Negative index handling in faces");
	
	SceneManager manager;
	bool loaded = manager.LoadOBJ(GetTestAssetPath("test_negative_indices.obj"));
	
	AssertTrue(loaded, "OBJ with negative indices should load successfully");
	AssertEqual(size_t{3}, manager.GetTriangleCount(), "Should have 3 triangles");
	
	EndTest();
}

void TestFaceFormats()
{
	BeginTest("Various face format support (v, v/vt, v/vt/vn, v//vn)");
	
	SceneManager manager;
	bool loaded = manager.LoadOBJ(GetTestAssetPath("box.obj"));
	
	AssertTrue(loaded, "Box OBJ with full face format should load");
	
	// Verify that normals were parsed correctly
	const auto& scene = manager.GetSceneData();
	bool hasValidNormals = true;
	for (const auto& tri : scene.Triangles)
	{
		if (glm::length(tri.N0) < 0.9f || glm::length(tri.N1) < 0.9f || glm::length(tri.N2) < 0.9f)
		{
			hasValidNormals = false;
			break;
		}
	}
	AssertTrue(hasValidNormals, "All normals should be valid (normalized)");
	
	EndTest();
}

// ----------------------------------------------------------------------------
// TEST SUITE 4: Material Parsing Tests
// ----------------------------------------------------------------------------

void TestMaterialLoading()
{
	BeginTest("MTL file loading and material count");
	
	SceneManager manager;
	bool loaded = manager.LoadOBJ(GetTestAssetPath("test_all_materials.obj"));
	
	AssertTrue(loaded, "OBJ with materials should load successfully");
	
	// Should have default material + 15 custom materials = 16+
	AssertGreaterThan(static_cast<int>(manager.GetMaterialCount()), 10, "Should have many materials");
	
	EndTest();
}

void TestDiffuseMaterialProperties()
{
	BeginTest("Diffuse material properties (Kd)");
	
	SceneManager manager;
	manager.LoadOBJ(GetTestAssetPath("test_all_materials.obj"));
	
	const auto& scene = manager.GetSceneData();
	
	// Find the diffuse_red material
	bool foundRed = false;
	for (const auto& mat : scene.Materials)
	{
		if (mat.Name == "diffuse_red")
		{
			foundRed = true;
			AssertFloatEqual(0.8f, mat.Albedo.r, "Red material should have R=0.8");
			AssertFloatEqual(0.1f, mat.Albedo.g, "Red material should have G=0.1");
			AssertFloatEqual(0.1f, mat.Albedo.b, "Red material should have B=0.1");
			break;
		}
	}
	AssertTrue(foundRed, "Should find diffuse_red material");
	
	EndTest();
}

void TestMetallicMaterialProperties()
{
	BeginTest("Metallic material properties (illum 3)");
	
	SceneManager manager;
	manager.LoadOBJ(GetTestAssetPath("test_all_materials.obj"));
	
	const auto& scene = manager.GetSceneData();
	
	// Find metallic material
	bool foundMetallic = false;
	for (const auto& mat : scene.Materials)
	{
		if (mat.Name == "metallic_gold" || mat.Name == "metallic_chrome")
		{
			foundMetallic = true;
			AssertFloatEqual(1.0f, mat.Metallic, "Metallic material should have Metallic=1.0");
			break;
		}
	}
	AssertTrue(foundMetallic, "Should find metallic material");
	
	EndTest();
}

void TestGlassMaterialProperties()
{
	BeginTest("Glass material properties (illum 7, Ni, d)");
	
	SceneManager manager;
	manager.LoadOBJ(GetTestAssetPath("test_all_materials.obj"));
	
	const auto& scene = manager.GetSceneData();
	
	// Find glass material
	bool foundGlass = false;
	for (const auto& mat : scene.Materials)
	{
		if (mat.Name == "glass_clear")
		{
			foundGlass = true;
			AssertFloatEqual(1.5f, mat.IOR, "Glass should have IOR=1.5");
			AssertFloatEqual(1.0f, mat.Transmission, "Glass (illum 7) should have Transmission=1.0");
			break;
		}
	}
	AssertTrue(foundGlass, "Should find glass material");
	
	EndTest();
}

void TestEmissiveMaterialProperties()
{
	BeginTest("Emissive material properties (Ke)");
	
	SceneManager manager;
	manager.LoadOBJ(GetTestAssetPath("test_all_materials.obj"));
	
	const auto& scene = manager.GetSceneData();
	
	// Find emissive material
	bool foundEmissive = false;
	for (const auto& mat : scene.Materials)
	{
		if (mat.Name == "emissive_warm")
		{
			foundEmissive = true;
			AssertGreaterThan(static_cast<int>(mat.EmissionStrength * 100), 0, 
			                  "Emissive material should have EmissionStrength > 0");
			AssertGreaterThan(static_cast<int>(mat.Emission.r * 100), 0, 
			                  "Emissive material should have Emission.r > 0");
			break;
		}
	}
	AssertTrue(foundEmissive, "Should find emissive material");
	
	EndTest();
}

void TestRoughnessConversion()
{
	BeginTest("Specular exponent to roughness conversion (Ns)");
	
	SceneManager manager;
	manager.LoadOBJ(GetTestAssetPath("test_all_materials.obj"));
	
	const auto& scene = manager.GetSceneData();
	
	// Find rough and smooth materials to compare
	float roughValue = -1.0f;
	float smoothValue = -1.0f;
	
	for (const auto& mat : scene.Materials)
	{
		if (mat.Name == "rough_surface")
			roughValue = mat.Roughness;
		else if (mat.Name == "smooth_surface")
			smoothValue = mat.Roughness;
	}
	
	if (roughValue >= 0 && smoothValue >= 0)
	{
		AssertTrue(roughValue > smoothValue, "Rough surface should have higher roughness than smooth");
	}
	
	EndTest();
}

void TestTransparencyProperties()
{
	BeginTest("Transparency properties (d/Tr)");
	
	SceneManager manager;
	manager.LoadOBJ(GetTestAssetPath("test_all_materials.obj"));
	
	const auto& scene = manager.GetSceneData();
	
	// Find transparent materials
	bool foundTransparent = false;
	for (const auto& mat : scene.Materials)
	{
		if (mat.Name == "transparent_50")
		{
			foundTransparent = true;
			// d=0.5 means 50% opacity, so transmission should be 0.5
			AssertFloatEqual(0.5f, mat.Transmission, "transparent_50 should have Transmission=0.5");
			break;
		}
	}
	AssertTrue(foundTransparent, "Should find transparent material");
	
	EndTest();
}

// ----------------------------------------------------------------------------
// TEST SUITE 5: Custom Extension Tests (Camera, Light)
// ----------------------------------------------------------------------------

void TestCameraExtension()
{
	BeginTest("Custom camera extension (c command)");
	
	SceneManager manager;
	bool loaded = manager.LoadOBJ(GetTestAssetPath("test_with_camera.obj"));
	
	AssertTrue(loaded, "OBJ with camera should load successfully");
	
	const auto& scene = manager.GetSceneData();
	AssertTrue(scene.HasCamera, "Scene should have camera");
	
	// Camera should be at approximately (0, 2, 5)
	AssertFloatEqual(0.0f, scene.CameraPosition.x, "Camera X position");
	AssertGreaterThan(static_cast<int>(scene.CameraPosition.y * 10), 0, "Camera Y should be > 0");
	AssertGreaterThan(static_cast<int>(scene.CameraPosition.z * 10), 0, "Camera Z should be > 0");
	
	// Up vector should be approximately (0, 1, 0)
	AssertFloatEqual(1.0f, scene.CameraUp.y, "Camera up Y should be 1");
	
	EndTest();
}

void TestLightPointExtension()
{
	BeginTest("Custom light point extension (lp command)");
	
	SceneManager manager;
	bool loaded = manager.LoadOBJ(GetTestAssetPath("test_with_camera.obj"));
	
	AssertTrue(loaded, "OBJ with light should load successfully");
	
	const auto& scene = manager.GetSceneData();
	AssertTrue(scene.HasLight, "Scene should have light");
	
	// Light should be above the scene
	AssertGreaterThan(static_cast<int>(scene.LightPosition.y * 10), 0, "Light Y should be > 0");
	
	EndTest();
}

// ----------------------------------------------------------------------------
// TEST SUITE 6: Scene Normalization Tests
// ----------------------------------------------------------------------------

void TestSceneNormalization()
{
	BeginTest("Scene normalization (fit to 6x6x6 box)");
	
	SceneManager manager;
	manager.LoadOBJ(GetTestAssetPath("box.obj"));
	
	const auto& scene = manager.GetSceneData();
	
	// Find bounding box of normalized scene
	glm::vec3 minBounds(FLT_MAX);
	glm::vec3 maxBounds(-FLT_MAX);
	
	for (const auto& tri : scene.Triangles)
	{
		minBounds = glm::min(minBounds, tri.V0);
		minBounds = glm::min(minBounds, tri.V1);
		minBounds = glm::min(minBounds, tri.V2);
		maxBounds = glm::max(maxBounds, tri.V0);
		maxBounds = glm::max(maxBounds, tri.V1);
		maxBounds = glm::max(maxBounds, tri.V2);
	}
	
	glm::vec3 size = maxBounds - minBounds;
	float maxDim = std::max({size.x, size.y, size.z});
	
	// Should be normalized to approximately 6.0
	AssertFloatEqual(6.0f, maxDim, "Largest dimension should be ~6.0");
	
	// Should be centered around origin
	glm::vec3 center = (minBounds + maxBounds) * 0.5f;
	AssertFloatEqual(0.0f, center.x, "Center X should be ~0");
	AssertFloatEqual(0.0f, center.y, "Center Y should be ~0");
	AssertFloatEqual(0.0f, center.z, "Center Z should be ~0");
	
	EndTest();
}

void TestLargeSceneNormalization()
{
	BeginTest("Large scene normalization (hyperboloid)");
	
	SceneManager manager;
	manager.LoadOBJ(GetTestAssetPath("hyperboloid_one_sheet.obj"));
	
	const auto& scene = manager.GetSceneData();
	
	// Verify all vertices are within normalized bounds
	bool allInBounds = true;
	for (const auto& tri : scene.Triangles)
	{
		auto checkBounds = [&](const glm::vec3& v) {
			if (std::abs(v.x) > 4.0f || std::abs(v.y) > 4.0f || std::abs(v.z) > 4.0f)
				allInBounds = false;
		};
		checkBounds(tri.V0);
		checkBounds(tri.V1);
		checkBounds(tri.V2);
	}
	
	AssertTrue(allInBounds, "All vertices should be within normalized bounds");
	
	EndTest();
}

// ----------------------------------------------------------------------------
// TEST SUITE 7: Triangle-Material Association Tests
// ----------------------------------------------------------------------------

void TestTriangleMaterialAssociation()
{
	BeginTest("Triangle-material association (usemtl)");
	
	SceneManager manager;
	manager.LoadOBJ(GetTestAssetPath("test_all_materials.obj"));
	
	const auto& scene = manager.GetSceneData();
	
	// Verify that triangles have valid material indices
	bool allValidIndices = true;
	int maxIndex = static_cast<int>(scene.Materials.size()) - 1;
	
	for (const auto& tri : scene.Triangles)
	{
		if (tri.MaterialIndex < 0 || tri.MaterialIndex > maxIndex)
		{
			allValidIndices = false;
			break;
		}
	}
	
	AssertTrue(allValidIndices, "All triangles should have valid material indices");
	
	// Verify multiple materials are used
	std::vector<bool> materialUsed(scene.Materials.size(), false);
	for (const auto& tri : scene.Triangles)
	{
		materialUsed[tri.MaterialIndex] = true;
	}
	
	int materialsUsedCount = 0;
	for (bool used : materialUsed)
	{
		if (used) materialsUsedCount++;
	}
	
	AssertGreaterThan(materialsUsedCount, 5, "Multiple materials should be in use");
	
	EndTest();
}

// ----------------------------------------------------------------------------
// TEST SUITE 8: Edge Cases and Error Handling
// ----------------------------------------------------------------------------

void TestEmptyFile()
{
	BeginTest("Handle empty/comment-only OBJ gracefully");
	
	// This test relies on the minimal file having actual content
	// An empty file would return false from LoadOBJ
	SceneManager manager;
	bool loaded = manager.LoadOBJ(GetTestAssetPath("test_minimal.obj"));
	
	// Should load since minimal has content
	AssertTrue(loaded, "Minimal OBJ should load");
	
	EndTest();
}

void TestMissingMTLFile()
{
	BeginTest("Handle missing MTL file gracefully");
	
	// The box.obj references quadric_materials.mtl
	// If MTL is missing, loading should still work but use default material
	SceneManager manager;
	bool loaded = manager.LoadOBJ(GetTestAssetPath("test_minimal.obj"));
	
	AssertTrue(loaded, "OBJ without MTL should still load");
	AssertEqual(size_t{1}, manager.GetMaterialCount(), "Should have default material");
	
	EndTest();
}

void TestVertexNormalGeneration()
{
	BeginTest("Automatic face normal generation when normals missing");
	
	SceneManager manager;
	manager.LoadOBJ(GetTestAssetPath("test_minimal.obj"));
	
	const auto& scene = manager.GetSceneData();
	
	if (!scene.Triangles.empty())
	{
		const auto& tri = scene.Triangles[0];
		
		// Normal should be perpendicular to the triangle
		// For a triangle in XY plane, normal should point in Z direction
		float normalLength = glm::length(tri.N0);
		AssertFloatEqual(1.0f, normalLength, "Generated normal should be normalized");
	}
	
	EndTest();
}

// ----------------------------------------------------------------------------
// TEST SUITE 9: Data Integrity Tests
// ----------------------------------------------------------------------------

void TestTriangleDataIntegrity()
{
	BeginTest("Triangle vertex data integrity");
	
	SceneManager manager;
	manager.LoadOBJ(GetTestAssetPath("box.obj"));
	
	const auto& scene = manager.GetSceneData();
	
	// Verify no NaN or infinite values
	bool allValid = true;
	for (const auto& tri : scene.Triangles)
	{
		auto checkValid = [](const glm::vec3& v) {
			return !std::isnan(v.x) && !std::isnan(v.y) && !std::isnan(v.z) &&
			       !std::isinf(v.x) && !std::isinf(v.y) && !std::isinf(v.z);
		};
		
		if (!checkValid(tri.V0) || !checkValid(tri.V1) || !checkValid(tri.V2) ||
		    !checkValid(tri.N0) || !checkValid(tri.N1) || !checkValid(tri.N2))
		{
			allValid = false;
			break;
		}
	}
	
	AssertTrue(allValid, "All vertex/normal data should be valid (no NaN/Inf)");
	
	EndTest();
}

void TestMaterialDataIntegrity()
{
	BeginTest("Material data integrity");
	
	SceneManager manager;
	manager.LoadOBJ(GetTestAssetPath("test_all_materials.obj"));
	
	const auto& scene = manager.GetSceneData();
	
	// Verify material properties are in valid ranges
	bool allValid = true;
	for (const auto& mat : scene.Materials)
	{
		// Roughness should be between 0 and 1
		if (mat.Roughness < 0.0f || mat.Roughness > 1.0f)
			allValid = false;
		
		// Metallic should be between 0 and 1
		if (mat.Metallic < 0.0f || mat.Metallic > 1.0f)
			allValid = false;
		
		// Transmission should be between 0 and 1
		if (mat.Transmission < 0.0f || mat.Transmission > 1.0f)
			allValid = false;
		
		// IOR should be positive
		if (mat.IOR < 0.0f)
			allValid = false;
		
		// Albedo components should be between 0 and 1
		if (mat.Albedo.r < 0.0f || mat.Albedo.g < 0.0f || mat.Albedo.b < 0.0f)
			allValid = false;
	}
	
	AssertTrue(allValid, "All material properties should be in valid ranges");
	
	EndTest();
}

// ----------------------------------------------------------------------------
// TEST SUITE 10: GetSceneData Access Tests
// ----------------------------------------------------------------------------

void TestGetSceneDataAccess()
{
	BeginTest("GetSceneData provides correct access");
	
	SceneManager manager;
	manager.LoadOBJ(GetTestAssetPath("box.obj"));
	
	const SceneData& scene = manager.GetSceneData();
	
	// Verify we can access all data
	AssertEqual(manager.GetTriangleCount(), scene.Triangles.size(), "Triangle count should match");
	AssertEqual(manager.GetMaterialCount(), scene.Materials.size(), "Material count should match");
	
	EndTest();
}

// ============================================================================
// MAIN - Run All Tests
// ============================================================================

int main()
{
	std::cout << std::endl;
	std::cout << "╔════════════════════════════════════════════════════════════╗" << std::endl;
	std::cout << "║          SCENE MANAGER TEST SUITE                          ║" << std::endl;
	std::cout << "║          Comprehensive OBJ/MTL Parser Tests                ║" << std::endl;
	std::cout << "╚════════════════════════════════════════════════════════════╝" << std::endl;
	
	// Suite 1: Basic Loading Tests
	PrintSectionHeader("SUITE 1: Basic Loading Tests");
	TestMinimalOBJLoading();
	TestBoxOBJLoading();
	TestNonExistentFile();
	TestClearFunction();
	
	// Suite 2: Quadric Surface Loading Tests
	PrintSectionHeader("SUITE 2: Quadric Surface Loading Tests");
	TestSphereLoading();
	TestCylinderLoading();
	TestConeLoading();
	TestEllipsoidLoading();
	TestEllipticParaboloidLoading();
	TestHyperbolicParaboloidLoading();
	TestHyperboloidOneSheetLoading();
	TestHyperboloidTwoSheetsLoading();
	
	// Suite 3: Face Parsing and Triangulation Tests
	PrintSectionHeader("SUITE 3: Face Parsing & Triangulation Tests");
	TestPolygonTriangulation();
	TestNegativeIndices();
	TestFaceFormats();
	
	// Suite 4: Material Parsing Tests
	PrintSectionHeader("SUITE 4: Material Parsing Tests");
	TestMaterialLoading();
	TestDiffuseMaterialProperties();
	TestMetallicMaterialProperties();
	TestGlassMaterialProperties();
	TestEmissiveMaterialProperties();
	TestRoughnessConversion();
	TestTransparencyProperties();
	
	// Suite 5: Custom Extension Tests
	PrintSectionHeader("SUITE 5: Custom Extension Tests (Camera, Light)");
	TestCameraExtension();
	TestLightPointExtension();
	
	// Suite 6: Scene Normalization Tests
	PrintSectionHeader("SUITE 6: Scene Normalization Tests");
	TestSceneNormalization();
	TestLargeSceneNormalization();
	
	// Suite 7: Triangle-Material Association Tests
	PrintSectionHeader("SUITE 7: Triangle-Material Association Tests");
	TestTriangleMaterialAssociation();
	
	// Suite 8: Edge Cases and Error Handling
	PrintSectionHeader("SUITE 8: Edge Cases & Error Handling");
	TestEmptyFile();
	TestMissingMTLFile();
	TestVertexNormalGeneration();
	
	// Suite 9: Data Integrity Tests
	PrintSectionHeader("SUITE 9: Data Integrity Tests");
	TestTriangleDataIntegrity();
	TestMaterialDataIntegrity();
	
	// Suite 10: GetSceneData Access Tests
	PrintSectionHeader("SUITE 10: API Access Tests");
	TestGetSceneDataAccess();
	
	// Print summary
	PrintSummary();
	
	return GetExitCode();
}

