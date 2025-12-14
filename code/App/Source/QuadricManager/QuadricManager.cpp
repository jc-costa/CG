#include "QuadricManager.h"

#include <iostream>
#include <string>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>

// ============================================================================
// CONSTRUCTOR
// ============================================================================
QuadricManager::QuadricManager()
{
	// Zero-initialize all quadrics
	for (int i = 0; i < MAX_QUADRICS; i++)
	{
		m_Quadrics[i] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
		                 glm::vec3(0.0f), glm::vec3(0.0f), 0};
	}
}

// ============================================================================
// INITIALIZE DEFAULT QUADRICS
// ============================================================================
void QuadricManager::InitializeDefaults()
{
	// Esfera teste usando quádrica (x² + y² + z² = r²)
	// Posição: lado direito, material dourado
	// Centro em (2.0, -2.0, 0.0), raio 0.6
	// Transladar: substituir x→(x-2), y→(y+2), z→z
	// (x-2)² + (y+2)² + z² = 0.36
	// x² - 4x + 4 + y² + 4y + 4 + z² = 0.36
	// x² + y² + z² - 4x + 4y + 7.64 = 0
	m_Quadrics[0] = {1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, -4.0f, 4.0f, 0.0f, 7.64f,
	                 glm::vec3(1.4f, -2.6f, -0.6f), glm::vec3(2.6f, -1.4f, 0.6f), 4};
	
	// Elipsoide - Esquerda atrás (x²/a² + y²/b² + z²/c² = 1)
	// a=0.5, b=0.8, c=0.4, centro em (-2.0, -2.0, -2.0)
	// Expandir: x²/0.25 + y²/0.64 + z²/0.16 = 1
	// Multiplicar por 0.16: 0.64x² + 0.25y² + z² = 0.16
	// Transladar (x→x+2, y→y+2, z→z+2):
	// 0.64(x+2)² + 0.25(y+2)² + (z+2)² = 0.16
	// 0.64x² + 2.56x + 2.56 + 0.25y² + y + 1 + z² + 4z + 4 = 0.16
	// 0.64x² + 0.25y² + z² + 2.56x + y + 4z + 7.4 = 0
	m_Quadrics[1] = {0.64f, 0.25f, 1.0f, 0.0f, 0.0f, 0.0f, 2.56f, 1.0f, 4.0f, 7.4f,
	                 glm::vec3(-2.5f, -2.8f, -2.4f), glm::vec3(-1.5f, -1.2f, -1.6f), 8};
	
	m_NumQuadrics = 2;
	
	std::cout << "\n========================================" << std::endl;
	std::cout << "QUADRICS LOADED:" << std::endl;
	std::cout << "1. Sphere (gold) - right side" << std::endl;
	std::cout << "2. Ellipsoid (white) - back left" << std::endl;
	std::cout << "Press G to open editor and modify!" << std::endl;
	std::cout << "========================================\n" << std::endl;
}

// ============================================================================
// RENDER IMGUI EDITOR
// ============================================================================
bool QuadricManager::RenderEditor()
{
	if (!m_ShowEditor)
		return false;

	bool changed = false;

	ImGui::SetNextWindowPos(ImVec2(10, 220), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(450, 600), ImGuiCond_FirstUseEver);
	ImGui::Begin("Quadric Surface Editor", &m_ShowEditor);
	
	ImGui::Text("Equation: Ax² + By² + Cz² + Dxy + Exz + Fyz + Gx + Hy + Iz + J = 0");
	ImGui::Separator();
	
	// Quadric selector
	ImGui::Text("Select Quadric:");
	ImGui::PushItemWidth(100);
	if (ImGui::InputInt("##quadric_index", &m_SelectedQuadric))
	{
		m_SelectedQuadric = glm::clamp(m_SelectedQuadric, 0, MAX_QUADRICS - 1);
		if (m_SelectedQuadric >= m_NumQuadrics)
			m_NumQuadrics = m_SelectedQuadric + 1;
	}
	ImGui::PopItemWidth();
	ImGui::SameLine();
	ImGui::Text("(0-%d) | Active: %d", MAX_QUADRICS - 1, m_NumQuadrics);
	
	ImGui::Separator();
	
	Quadric& q = m_Quadrics[m_SelectedQuadric];
	
	// Quadratic coefficients
	ImGui::Text("Quadratic Terms:");
	ImGui::PushItemWidth(120);
	changed |= ImGui::DragFloat("A (x²)", &q.A, 0.01f, -10.0f, 10.0f, "%.3f");
	ImGui::SameLine();
	changed |= ImGui::DragFloat("B (y²)", &q.B, 0.01f, -10.0f, 10.0f, "%.3f");
	ImGui::SameLine();
	changed |= ImGui::DragFloat("C (z²)", &q.C, 0.01f, -10.0f, 10.0f, "%.3f");
	ImGui::PopItemWidth();
	
	// Cross terms
	ImGui::Text("Cross Terms:");
	ImGui::PushItemWidth(120);
	changed |= ImGui::DragFloat("D (xy)", &q.D, 0.01f, -10.0f, 10.0f, "%.3f");
	ImGui::SameLine();
	changed |= ImGui::DragFloat("E (xz)", &q.E, 0.01f, -10.0f, 10.0f, "%.3f");
	ImGui::SameLine();
	changed |= ImGui::DragFloat("F (yz)", &q.F, 0.01f, -10.0f, 10.0f, "%.3f");
	ImGui::PopItemWidth();
	
	// Linear terms
	ImGui::Text("Linear Terms:");
	ImGui::PushItemWidth(120);
	changed |= ImGui::DragFloat("G (x)", &q.G, 0.01f, -10.0f, 10.0f, "%.3f");
	ImGui::SameLine();
	changed |= ImGui::DragFloat("H (y)", &q.H, 0.01f, -10.0f, 10.0f, "%.3f");
	ImGui::SameLine();
	changed |= ImGui::DragFloat("I (z)", &q.I, 0.01f, -10.0f, 10.0f, "%.3f");
	ImGui::PopItemWidth();
	
	// Constant
	ImGui::Text("Constant:");
	ImGui::PushItemWidth(120);
	changed |= ImGui::DragFloat("J", &q.J, 0.01f, -10.0f, 10.0f, "%.3f");
	ImGui::PopItemWidth();
	
	ImGui::Separator();
	
	// Bounding box
	ImGui::Text("Bounding Box:");
	ImGui::PushItemWidth(100);
	changed |= ImGui::DragFloat3("Min", &q.bboxMin.x, 0.1f, -20.0f, 20.0f, "%.2f");
	changed |= ImGui::DragFloat3("Max", &q.bboxMax.x, 0.1f, -20.0f, 20.0f, "%.2f");
	ImGui::PopItemWidth();
	
	ImGui::Separator();
	
	// Material
	ImGui::Text("Material:");
	const char* materials[] = {
		"0: White Diffuse", "1: Red Diffuse", "2: Green Diffuse",
		"3: Chrome", "4: Gold", "5: Light", "6: Glass",
		"7: Blue Glossy", "8: Rough White", "9: Bronze"
	};
	ImGui::PushItemWidth(200);
	changed |= ImGui::Combo("##material", &q.materialIndex, materials, 10);
	ImGui::PopItemWidth();
	
	ImGui::Separator();
	
	// Presets
	ImGui::Text("Quick Presets:");
	
	// BIG VISIBLE TEST SPHERE
	if (ImGui::Button("TEST SPHERE (EMISSIVE)"))
	{
		q = {1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 4.0f, 0.0f,
		     glm::vec3(-3.0f, -3.0f, -5.0f), glm::vec3(3.0f, 3.0f, 1.0f), 5};
		changed = true;
	}
	ImGui::SameLine();
	
	if (ImGui::Button("Sphere (r=1)"))
	{
		q = {1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f,
		     glm::vec3(-1.0f), glm::vec3(1.0f), 6};
		changed = true;
	}
	ImGui::SameLine();
	if (ImGui::Button("Cylinder (r=0.6)"))
	{
		q = {1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, -0.36f,
		     glm::vec3(-0.6f, -0.6f, -2.0f), glm::vec3(0.6f, 0.6f, 2.0f), 3};
		changed = true;
	}
	if (ImGui::Button("Cone"))
	{
		q = {1.0f, 1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
		     glm::vec3(-2.0f, -2.0f, -3.0f), glm::vec3(2.0f, 2.0f, 3.0f), 3};
		changed = true;
	}
	ImGui::SameLine();
	if (ImGui::Button("Paraboloid"))
	{
		q = {1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f,
		     glm::vec3(-1.5f, -1.5f, 0.0f), glm::vec3(1.5f, 1.5f, 4.5f), 4};
		changed = true;
	}
	if (ImGui::Button("Ellipsoid"))
	{
		q = {1.5625f, 0.6944f, 2.7778f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f,
		     glm::vec3(-0.8f, -1.2f, -0.6f), glm::vec3(0.8f, 1.2f, 0.6f), 8};
		changed = true;
	}
	ImGui::SameLine();
	if (ImGui::Button("Hyperboloid"))
	{
		q = {4.0f, 4.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f,
		     glm::vec3(-1.0f, -1.0f, -2.0f), glm::vec3(1.0f, 1.0f, 2.0f), 7};
		changed = true;
	}
	
	ImGui::Separator();
	
	// Copy equation
	ImGui::Text("Equation:");
	ImGui::TextWrapped("%.3fx² + %.3fy² + %.3fz² + %.3fxy + %.3fxz + %.3fyz + %.3fx + %.3fy + %.3fz + %.3f = 0",
	                   q.A, q.B, q.C, q.D, q.E, q.F, q.G, q.H, q.I, q.J);
	
	ImGui::End();

	return changed;
}

// ============================================================================
// UPLOAD QUADRICS TO SHADER
// ============================================================================
void QuadricManager::UploadToShader(GLuint shaderProgram) const
{
	GLint uNumQuadricsLoc = glGetUniformLocation(shaderProgram, "uNumQuadrics");
	glUniform1i(uNumQuadricsLoc, m_NumQuadrics);
	
	// Send quadric data as separate uniform arrays
	for (int i = 0; i < m_NumQuadrics && i < MAX_QUADRICS; i++)
	{
		const Quadric& q = m_Quadrics[i];
		
		glUniform1f(glGetUniformLocation(shaderProgram, ("uQuadrics_A[" + std::to_string(i) + "]").c_str()), q.A);
		glUniform1f(glGetUniformLocation(shaderProgram, ("uQuadrics_B[" + std::to_string(i) + "]").c_str()), q.B);
		glUniform1f(glGetUniformLocation(shaderProgram, ("uQuadrics_C[" + std::to_string(i) + "]").c_str()), q.C);
		glUniform1f(glGetUniformLocation(shaderProgram, ("uQuadrics_D[" + std::to_string(i) + "]").c_str()), q.D);
		glUniform1f(glGetUniformLocation(shaderProgram, ("uQuadrics_E[" + std::to_string(i) + "]").c_str()), q.E);
		glUniform1f(glGetUniformLocation(shaderProgram, ("uQuadrics_F[" + std::to_string(i) + "]").c_str()), q.F);
		glUniform1f(glGetUniformLocation(shaderProgram, ("uQuadrics_G[" + std::to_string(i) + "]").c_str()), q.G);
		glUniform1f(glGetUniformLocation(shaderProgram, ("uQuadrics_H[" + std::to_string(i) + "]").c_str()), q.H);
		glUniform1f(glGetUniformLocation(shaderProgram, ("uQuadrics_I[" + std::to_string(i) + "]").c_str()), q.I);
		glUniform1f(glGetUniformLocation(shaderProgram, ("uQuadrics_J[" + std::to_string(i) + "]").c_str()), q.J);
		glUniform3fv(glGetUniformLocation(shaderProgram, ("uQuadrics_bboxMin[" + std::to_string(i) + "]").c_str()), 1, glm::value_ptr(q.bboxMin));
		glUniform3fv(glGetUniformLocation(shaderProgram, ("uQuadrics_bboxMax[" + std::to_string(i) + "]").c_str()), 1, glm::value_ptr(q.bboxMax));
		glUniform1i(glGetUniformLocation(shaderProgram, ("uQuadrics_materialIndex[" + std::to_string(i) + "]").c_str()), q.materialIndex);
	}
}

// ============================================================================
// ACCESSORS
// ============================================================================
const Quadric& QuadricManager::GetQuadric(int index) const
{
	if (index < 0 || index >= MAX_QUADRICS)
	{
		static Quadric empty = {};
		return empty;
	}
	return m_Quadrics[index];
}

Quadric& QuadricManager::GetQuadric(int index)
{
	if (index < 0 || index >= MAX_QUADRICS)
	{
		static Quadric empty = {};
		return empty;
	}
	return m_Quadrics[index];
}

