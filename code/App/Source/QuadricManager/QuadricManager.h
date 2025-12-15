#pragma once

#include <glm/glm.hpp>
#include <glad/gl.h>
#include <string>
#include <vector>

// ============================================================================
// QUADRIC STRUCTURE
// ============================================================================
struct Quadric
{
	float A, B, C;      // x², y², z² coefficients
	float D, E, F;      // xy, xz, yz cross terms
	float G, H, I;      // x, y, z linear terms
	float J;            // constant term
	glm::vec3 bboxMin;  // Bounding box minimum
	glm::vec3 bboxMax;  // Bounding box maximum
	int materialIndex;
};

// ============================================================================
// QUADRIC MANAGER CLASS
// ============================================================================
class QuadricManager
{
public:
	static constexpr int MAX_QUADRICS = 8;

	QuadricManager();
	~QuadricManager() = default;

	// Initialize with default quadrics
	void InitializeDefaults();

	// Render the ImGui editor window
	// Returns true if any quadric was modified
	bool RenderEditor();

	// Upload quadric uniforms to shader
	void UploadToShader(GLuint shaderProgram) const;

	// Accessors
	int GetNumQuadrics() const { return m_NumQuadrics; }
	bool IsEditorVisible() const { return m_ShowEditor; }
	void SetEditorVisible(bool visible) { m_ShowEditor = visible; }
	void ToggleEditor() { m_ShowEditor = !m_ShowEditor; }

	// Get quadric for external access if needed
	const Quadric& GetQuadric(int index) const;
	Quadric& GetQuadric(int index);

private:
	Quadric m_Quadrics[MAX_QUADRICS];
	int m_NumQuadrics = 0;
	int m_SelectedQuadric = 0;
	bool m_ShowEditor = false;
};

