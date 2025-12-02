// ============================================================================
// QUADRIC SURFACES - Implementation
// ============================================================================

#include "Quadric.h"
#include <cmath>
#include <algorithm>
#include <limits>

namespace Quadric
{
	// ============================================================================
	// BOUNDING BOX IMPLEMENTATION
	// ============================================================================
	
	bool BoundingBox::Contains(const glm::vec3& point) const
	{
		return point.x >= Min.x && point.x <= Max.x &&
		       point.y >= Min.y && point.y <= Max.y &&
		       point.z >= Min.z && point.z <= Max.z;
	}
	
	bool BoundingBox::Intersect(const glm::vec3& origin, const glm::vec3& direction,
	                           float& tMin, float& tMax) const
	{
		// AABB ray intersection using slab method
		float t1 = (Min.x - origin.x) / direction.x;
		float t2 = (Max.x - origin.x) / direction.x;
		
		tMin = std::min(t1, t2);
		tMax = std::max(t1, t2);
		
		t1 = (Min.y - origin.y) / direction.y;
		t2 = (Max.y - origin.y) / direction.y;
		
		tMin = std::max(tMin, std::min(t1, t2));
		tMax = std::min(tMax, std::max(t1, t2));
		
		t1 = (Min.z - origin.z) / direction.z;
		t2 = (Max.z - origin.z) / direction.z;
		
		tMin = std::max(tMin, std::min(t1, t2));
		tMax = std::min(tMax, std::max(t1, t2));
		
		return tMax >= tMin && tMax >= 0.0f;
	}
	
	// ============================================================================
	// QUADRIC SURFACE IMPLEMENTATION
	// ============================================================================
	
	QuadricSurface::QuadricSurface()
		: m_Coefficients()
		, m_BoundingBox()
		, m_UseBoundingBox(false)
	{
	}
	
	QuadricSurface::QuadricSurface(const QuadricCoefficients& coeffs)
		: m_Coefficients(coeffs)
		, m_BoundingBox()
		, m_UseBoundingBox(false)
	{
	}
	
	QuadricSurface::QuadricSurface(const QuadricCoefficients& coeffs, const BoundingBox& bbox)
		: m_Coefficients(coeffs)
		, m_BoundingBox(bbox)
		, m_UseBoundingBox(true)
	{
	}
	
	void QuadricSurface::SetCoefficients(const QuadricCoefficients& coeffs)
	{
		m_Coefficients = coeffs;
	}
	
	void QuadricSurface::SetBoundingBox(const BoundingBox& bbox)
	{
		m_BoundingBox = bbox;
	}
	
	bool QuadricSurface::SolveQuadratic(float A, float B, float C, float& t0, float& t1) const
	{
		// Handle linear case (A ≈ 0)
		if (std::abs(A) < 1e-6f)
		{
			if (std::abs(B) < 1e-6f)
				return false;
			
			t0 = t1 = -C / B;
			return true;
		}
		
		// Quadratic formula with numerical stability
		float discriminant = B * B - 4.0f * A * C;
		
		if (discriminant < 0.0f)
			return false;
		
		float sqrtDiscriminant = std::sqrt(discriminant);
		float q = (B < 0.0f) ? (-B - sqrtDiscriminant) / 2.0f : (-B + sqrtDiscriminant) / 2.0f;
		
		t0 = q / A;
		t1 = C / q;
		
		if (t0 > t1)
			std::swap(t0, t1);
		
		return true;
	}
	
	IntersectionResult QuadricSurface::Intersect(const glm::vec3& rayOrigin,
	                                            const glm::vec3& rayDirection,
	                                            float tMin, float tMax) const
	{
		IntersectionResult result;
		result.Hit = false;
		
		// Check bounding box first if enabled
		if (m_UseBoundingBox)
		{
			float boxTMin, boxTMax;
			if (!m_BoundingBox.Intersect(rayOrigin, rayDirection, boxTMin, boxTMax))
				return result;
			
			// Restrict search range to bounding box
			tMin = std::max(tMin, boxTMin);
			tMax = std::min(tMax, boxTMax);
		}
		
		// Ray equation: P(t) = O + tD, where O = origin, D = direction
		// Substitute into quadric equation and solve for t
		
		const float& A = m_Coefficients.A;
		const float& B = m_Coefficients.B;
		const float& C = m_Coefficients.C;
		const float& D = m_Coefficients.D;
		const float& E = m_Coefficients.E;
		const float& F = m_Coefficients.F;
		const float& G = m_Coefficients.G;
		const float& H = m_Coefficients.H;
		const float& I = m_Coefficients.I;
		const float& J = m_Coefficients.J;
		
		const glm::vec3& O = rayOrigin;
		const glm::vec3& d = rayDirection;
		
		// Compute coefficients for the quadratic equation: at² + bt + c = 0
		
		// Coefficient of t²
		float a = A * d.x * d.x + 
		         B * d.y * d.y + 
		         C * d.z * d.z +
		         D * d.x * d.y + 
		         E * d.x * d.z + 
		         F * d.y * d.z;
		
		// Coefficient of t
		float b = 2.0f * A * O.x * d.x + 
		         2.0f * B * O.y * d.y + 
		         2.0f * C * O.z * d.z +
		         D * (O.x * d.y + O.y * d.x) + 
		         E * (O.x * d.z + O.z * d.x) + 
		         F * (O.y * d.z + O.z * d.y) +
		         G * d.x + 
		         H * d.y + 
		         I * d.z;
		
		// Constant term
		float c = A * O.x * O.x + 
		         B * O.y * O.y + 
		         C * O.z * O.z +
		         D * O.x * O.y + 
		         E * O.x * O.z + 
		         F * O.y * O.z +
		         G * O.x + 
		         H * O.y + 
		         I * O.z + 
		         J;
		
		// Solve quadratic equation
		float t0, t1;
		if (!SolveQuadratic(a, b, c, t0, t1))
			return result;
		
		// Find the closest valid intersection
		float t = -1.0f;
		
		if (t0 >= tMin && t0 <= tMax)
		{
			t = t0;
		}
		else if (t1 >= tMin && t1 <= tMax)
		{
			t = t1;
		}
		else
		{
			return result;
		}
		
		// Calculate intersection point
		glm::vec3 hitPoint = rayOrigin + t * rayDirection;
		
		// If bounding box is enabled, verify point is inside
		if (m_UseBoundingBox && !m_BoundingBox.Contains(hitPoint))
		{
			// Try the other intersection point
			if (t == t0 && t1 >= tMin && t1 <= tMax)
			{
				t = t1;
				hitPoint = rayOrigin + t * rayDirection;
				if (!m_BoundingBox.Contains(hitPoint))
					return result;
			}
			else
			{
				return result;
			}
		}
		
		// Calculate normal using gradient
		glm::vec3 normal = CalculateNormal(hitPoint);
		
		// Ensure normal points towards ray origin
		if (glm::dot(normal, rayDirection) > 0.0f)
			normal = -normal;
		
		result.Hit = true;
		result.Distance = t;
		result.Point = hitPoint;
		result.Normal = glm::normalize(normal);
		
		return result;
	}
	
	float QuadricSurface::Evaluate(const glm::vec3& point) const
	{
		const float& A = m_Coefficients.A;
		const float& B = m_Coefficients.B;
		const float& C = m_Coefficients.C;
		const float& D = m_Coefficients.D;
		const float& E = m_Coefficients.E;
		const float& F = m_Coefficients.F;
		const float& G = m_Coefficients.G;
		const float& H = m_Coefficients.H;
		const float& I = m_Coefficients.I;
		const float& J = m_Coefficients.J;
		
		const float& x = point.x;
		const float& y = point.y;
		const float& z = point.z;
		
		return A * x * x + B * y * y + C * z * z +
		       D * x * y + E * x * z + F * y * z +
		       G * x + H * y + I * z + J;
	}
	
	glm::vec3 QuadricSurface::CalculateNormal(const glm::vec3& point) const
	{
		// Normal is the gradient: ∇f(x,y,z) = (∂f/∂x, ∂f/∂y, ∂f/∂z)
		//
		// ∂f/∂x = 2Ax + Dy + Ez + G
		// ∂f/∂y = 2By + Dx + Fz + H
		// ∂f/∂z = 2Cz + Ex + Fy + I
		
		const float& A = m_Coefficients.A;
		const float& B = m_Coefficients.B;
		const float& C = m_Coefficients.C;
		const float& D = m_Coefficients.D;
		const float& E = m_Coefficients.E;
		const float& F = m_Coefficients.F;
		const float& G = m_Coefficients.G;
		const float& H = m_Coefficients.H;
		const float& I = m_Coefficients.I;
		
		const float& x = point.x;
		const float& y = point.y;
		const float& z = point.z;
		
		glm::vec3 gradient;
		gradient.x = 2.0f * A * x + D * y + E * z + G;
		gradient.y = 2.0f * B * y + D * x + F * z + H;
		gradient.z = 2.0f * C * z + E * x + F * y + I;
		
		return gradient;
	}
	
	// ============================================================================
	// FACTORY METHODS
	// ============================================================================
	
	QuadricSurface QuadricSurface::CreateSphere(float radius)
	{
		// x² + y² + z² - r² = 0
		QuadricCoefficients coeffs;
		coeffs.A = 1.0f;
		coeffs.B = 1.0f;
		coeffs.C = 1.0f;
		coeffs.J = -radius * radius;
		
		return QuadricSurface(coeffs);
	}
	
	QuadricSurface QuadricSurface::CreateEllipsoid(float a, float b, float c)
	{
		// x²/a² + y²/b² + z²/c² - 1 = 0
		QuadricCoefficients coeffs;
		coeffs.A = 1.0f / (a * a);
		coeffs.B = 1.0f / (b * b);
		coeffs.C = 1.0f / (c * c);
		coeffs.J = -1.0f;
		
		return QuadricSurface(coeffs);
	}
	
	QuadricSurface QuadricSurface::CreateCylinder(float radius, float height)
	{
		// x² + y² - r² = 0, bounded by z ∈ [-height/2, height/2]
		QuadricCoefficients coeffs;
		coeffs.A = 1.0f;
		coeffs.B = 1.0f;
		coeffs.J = -radius * radius;
		
		BoundingBox bbox;
		bbox.Min = glm::vec3(-radius - 1.0f, -radius - 1.0f, -height / 2.0f);
		bbox.Max = glm::vec3(radius + 1.0f, radius + 1.0f, height / 2.0f);
		
		return QuadricSurface(coeffs, bbox);
	}
	
	QuadricSurface QuadricSurface::CreateEllipticCylinder(float a, float b, float height)
	{
		// x²/a² + y²/b² - 1 = 0
		QuadricCoefficients coeffs;
		coeffs.A = 1.0f / (a * a);
		coeffs.B = 1.0f / (b * b);
		coeffs.J = -1.0f;
		
		float maxRadius = std::max(a, b);
		BoundingBox bbox;
		bbox.Min = glm::vec3(-maxRadius - 1.0f, -maxRadius - 1.0f, -height / 2.0f);
		bbox.Max = glm::vec3(maxRadius + 1.0f, maxRadius + 1.0f, height / 2.0f);
		
		return QuadricSurface(coeffs, bbox);
	}
	
	QuadricSurface QuadricSurface::CreateCone(float angle, float height)
	{
		// x² + y² - (z·tan(θ))² = 0
		float tanAngle = std::tan(angle);
		float k = tanAngle * tanAngle;
		
		QuadricCoefficients coeffs;
		coeffs.A = 1.0f;
		coeffs.B = 1.0f;
		coeffs.C = -k;
		
		float radius = height * tanAngle;
		BoundingBox bbox;
		bbox.Min = glm::vec3(-radius - 1.0f, -radius - 1.0f, 0.0f);
		bbox.Max = glm::vec3(radius + 1.0f, radius + 1.0f, height);
		
		return QuadricSurface(coeffs, bbox);
	}
	
	QuadricSurface QuadricSurface::CreateHyperboloidOneSheet(float a, float b, float c, float height)
	{
		// x²/a² + y²/b² - z²/c² - 1 = 0
		QuadricCoefficients coeffs;
		coeffs.A = 1.0f / (a * a);
		coeffs.B = 1.0f / (b * b);
		coeffs.C = -1.0f / (c * c);
		coeffs.J = -1.0f;
		
		float maxRadius = std::max(a, b);
		BoundingBox bbox;
		bbox.Min = glm::vec3(-maxRadius * 2.0f, -maxRadius * 2.0f, -height / 2.0f);
		bbox.Max = glm::vec3(maxRadius * 2.0f, maxRadius * 2.0f, height / 2.0f);
		
		return QuadricSurface(coeffs, bbox);
	}
	
	QuadricSurface QuadricSurface::CreateHyperboloidTwoSheets(float a, float b, float c, float height)
	{
		// -x²/a² - y²/b² + z²/c² - 1 = 0
		QuadricCoefficients coeffs;
		coeffs.A = -1.0f / (a * a);
		coeffs.B = -1.0f / (b * b);
		coeffs.C = 1.0f / (c * c);
		coeffs.J = -1.0f;
		
		float maxRadius = std::max(a, b);
		BoundingBox bbox;
		bbox.Min = glm::vec3(-maxRadius * 2.0f, -maxRadius * 2.0f, -height / 2.0f);
		bbox.Max = glm::vec3(maxRadius * 2.0f, maxRadius * 2.0f, height / 2.0f);
		
		return QuadricSurface(coeffs, bbox);
	}
	
	QuadricSurface QuadricSurface::CreateEllipticParaboloid(float a, float b, float height)
	{
		// z - x²/a² - y²/b² = 0  =>  -x²/a² - y²/b² + z = 0
		QuadricCoefficients coeffs;
		coeffs.A = -1.0f / (a * a);
		coeffs.B = -1.0f / (b * b);
		coeffs.I = 1.0f;
		
		float maxRadius = std::max(a, b) * std::sqrt(height);
		BoundingBox bbox;
		bbox.Min = glm::vec3(-maxRadius, -maxRadius, 0.0f);
		bbox.Max = glm::vec3(maxRadius, maxRadius, height);
		
		return QuadricSurface(coeffs, bbox);
	}
	
	QuadricSurface QuadricSurface::CreateHyperbolicParaboloid(float a, float b, float height)
	{
		// z - x²/a² + y²/b² = 0  =>  -x²/a² + y²/b² + z = 0
		QuadricCoefficients coeffs;
		coeffs.A = -1.0f / (a * a);
		coeffs.B = 1.0f / (b * b);
		coeffs.I = 1.0f;
		
		float maxRadius = std::max(a, b) * std::sqrt(std::abs(height));
		BoundingBox bbox;
		bbox.Min = glm::vec3(-maxRadius, -maxRadius, -height);
		bbox.Max = glm::vec3(maxRadius, maxRadius, height);
		
		return QuadricSurface(coeffs, bbox);
	}
	
	// ============================================================================
	// UTILITY FUNCTIONS
	// ============================================================================
	
	bool IsQuadricBounded(const QuadricCoefficients& coeffs)
	{
		// A quadric is bounded if all second-degree terms have the same sign
		// and none are zero (ellipsoid case)
		
		int positiveCount = 0;
		int negativeCount = 0;
		int zeroCount = 0;
		
		if (coeffs.A > 1e-6f) positiveCount++;
		else if (coeffs.A < -1e-6f) negativeCount++;
		else zeroCount++;
		
		if (coeffs.B > 1e-6f) positiveCount++;
		else if (coeffs.B < -1e-6f) negativeCount++;
		else zeroCount++;
		
		if (coeffs.C > 1e-6f) positiveCount++;
		else if (coeffs.C < -1e-6f) negativeCount++;
		else zeroCount++;
		
		// Bounded if all three are positive (ellipsoid)
		return (positiveCount == 3 && negativeCount == 0);
	}
	
	const char* GetQuadricTypeName(const QuadricCoefficients& coeffs)
	{
		// Simple heuristic classification
		bool hasA = std::abs(coeffs.A) > 1e-6f;
		bool hasB = std::abs(coeffs.B) > 1e-6f;
		bool hasC = std::abs(coeffs.C) > 1e-6f;
		bool hasCross = std::abs(coeffs.D) > 1e-6f || std::abs(coeffs.E) > 1e-6f || std::abs(coeffs.F) > 1e-6f;
		bool hasLinear = std::abs(coeffs.G) > 1e-6f || std::abs(coeffs.H) > 1e-6f || std::abs(coeffs.I) > 1e-6f;
		
		if (hasCross)
			return "General Quadric";
		
		if (hasA && hasB && hasC)
		{
			if (coeffs.A > 0 && coeffs.B > 0 && coeffs.C > 0)
				return "Ellipsoid";
			else if (coeffs.A > 0 && coeffs.B > 0 && coeffs.C < 0)
				return "Hyperboloid";
			else
				return "General Quadric";
		}
		
		if (hasA && hasB && !hasC)
		{
			if (hasLinear)
				return "Paraboloid";
			else
				return "Cylinder";
		}
		
		if (hasA && hasB && hasC && (coeffs.C * coeffs.A < 0))
			return "Cone";
		
		if (hasLinear && !hasA && !hasB && !hasC)
			return "Plane";
		
		return "Quadric Surface";
	}
	
} // namespace Quadric
