// ============================================================================
// QUADRIC SURFACES - General Quadric Implementation
// ============================================================================
// Implements ray-quadric intersection for the general quadric equation:
// Ax² + By² + Cz² + Dxy + Exz + Fyz + Gx + Hy + Iz + J = 0
// ============================================================================

#pragma once

#include <glm/glm.hpp>
#include <optional>

namespace Quadric
{
	// ============================================================================
	// STRUCTURES
	// ============================================================================
	
	/// Coefficients for the general quadric equation
	/// Ax² + By² + Cz² + Dxy + Exz + Fyz + Gx + Hy + Iz + J = 0
	struct QuadricCoefficients
	{
		// Second-degree terms
		float A = 0.0f;  // x²
		float B = 0.0f;  // y²
		float C = 0.0f;  // z²
		float D = 0.0f;  // xy
		float E = 0.0f;  // xz
		float F = 0.0f;  // yz
		
		// First-degree terms
		float G = 0.0f;  // x
		float H = 0.0f;  // y
		float I = 0.0f;  // z
		
		// Constant term
		float J = 0.0f;
		
		QuadricCoefficients() = default;
		
		QuadricCoefficients(float a, float b, float c, float d, float e, float f,
		                   float g, float h, float i, float j)
			: A(a), B(b), C(c), D(d), E(e), F(f), G(g), H(h), I(i), J(j)
		{}
	};
	
	/// Axis-aligned bounding box for limiting unbounded quadrics
	struct BoundingBox
	{
		glm::vec3 Min = glm::vec3(-10.0f);
		glm::vec3 Max = glm::vec3(10.0f);
		
		BoundingBox() = default;
		BoundingBox(const glm::vec3& min, const glm::vec3& max)
			: Min(min), Max(max)
		{}
		
		/// Check if a point is inside the bounding box
		bool Contains(const glm::vec3& point) const;
		
		/// Intersect ray with bounding box (returns entry and exit distances)
		bool Intersect(const glm::vec3& origin, const glm::vec3& direction, 
		              float& tMin, float& tMax) const;
	};
	
	/// Result of ray-quadric intersection
	struct IntersectionResult
	{
		bool Hit = false;
		float Distance = 0.0f;
		glm::vec3 Point = glm::vec3(0.0f);
		glm::vec3 Normal = glm::vec3(0.0f, 1.0f, 0.0f);
	};
	
	// ============================================================================
	// QUADRIC CLASS
	// ============================================================================
	
	class QuadricSurface
	{
	public:
		QuadricSurface();
		QuadricSurface(const QuadricCoefficients& coeffs);
		QuadricSurface(const QuadricCoefficients& coeffs, const BoundingBox& bbox);
		
		/// Set quadric coefficients
		void SetCoefficients(const QuadricCoefficients& coeffs);
		
		/// Get current coefficients
		const QuadricCoefficients& GetCoefficients() const { return m_Coefficients; }
		
		/// Set bounding box (for unbounded surfaces)
		void SetBoundingBox(const BoundingBox& bbox);
		
		/// Enable/disable bounding box
		void SetBoundingBoxEnabled(bool enabled) { m_UseBoundingBox = enabled; }
		
		/// Check if bounding box is enabled
		bool IsBoundingBoxEnabled() const { return m_UseBoundingBox; }
		
		/// Intersect ray with quadric surface
		/// Returns the closest intersection point if hit
		IntersectionResult Intersect(const glm::vec3& rayOrigin, 
		                            const glm::vec3& rayDirection,
		                            float tMin = 0.001f, 
		                            float tMax = 1000.0f) const;
		
		/// Evaluate quadric function at a point
		/// f(x,y,z) = Ax² + By² + Cz² + Dxy + Exz + Fyz + Gx + Hy + Iz + J
		float Evaluate(const glm::vec3& point) const;
		
		/// Calculate normal at a point using gradient: n = ∇f(x,y,z)
		/// ∇f = (∂f/∂x, ∂f/∂y, ∂f/∂z)
		glm::vec3 CalculateNormal(const glm::vec3& point) const;
		
		// ============================================================================
		// FACTORY METHODS - Common Quadric Surfaces
		// ============================================================================
		
		/// Create a sphere: x² + y² + z² - r² = 0
		static QuadricSurface CreateSphere(float radius);
		
		/// Create an ellipsoid: x²/a² + y²/b² + z²/c² - 1 = 0
		static QuadricSurface CreateEllipsoid(float a, float b, float c);
		
		/// Create a circular cylinder (along Z axis): x² + y² - r² = 0
		static QuadricSurface CreateCylinder(float radius, float height);
		
		/// Create an elliptic cylinder: x²/a² + y²/b² - 1 = 0
		static QuadricSurface CreateEllipticCylinder(float a, float b, float height);
		
		/// Create a cone (along Z axis): x² + y² - (z·tan(θ))² = 0
		static QuadricSurface CreateCone(float angle, float height);
		
		/// Create a hyperboloid of one sheet: x²/a² + y²/b² - z²/c² - 1 = 0
		static QuadricSurface CreateHyperboloidOneSheet(float a, float b, float c, float height);
		
		/// Create a hyperboloid of two sheets: x²/a² + y²/b² - z²/c² + 1 = 0
		static QuadricSurface CreateHyperboloidTwoSheets(float a, float b, float c, float height);
		
		/// Create an elliptic paraboloid: z = x²/a² + y²/b²
		static QuadricSurface CreateEllipticParaboloid(float a, float b, float height);
		
		/// Create a hyperbolic paraboloid (saddle): z = x²/a² - y²/b²
		static QuadricSurface CreateHyperbolicParaboloid(float a, float b, float height);
		
	private:
		QuadricCoefficients m_Coefficients;
		BoundingBox m_BoundingBox;
		bool m_UseBoundingBox = false;
		
		/// Solve quadratic equation At² + Bt + C = 0
		/// Returns true if real solutions exist
		bool SolveQuadratic(float A, float B, float C, float& t0, float& t1) const;
	};
	
	// ============================================================================
	// UTILITY FUNCTIONS
	// ============================================================================
	
	/// Check if quadric represents a bounded surface
	bool IsQuadricBounded(const QuadricCoefficients& coeffs);
	
	/// Get a descriptive name for common quadric types
	const char* GetQuadricTypeName(const QuadricCoefficients& coeffs);
	
} // namespace Quadric
