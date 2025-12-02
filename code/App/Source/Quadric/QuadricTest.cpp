// ============================================================================
// QUADRIC TEST - Standalone Test Program
// ============================================================================

#include "Quadric.h"
#include <iostream>
#include <iomanip>

using namespace Quadric;

// Helper function to print equation
void PrintQuadricEquation(const QuadricCoefficients& coeffs)
{
	std::cout << "Equation: ";
	if (coeffs.A != 0) std::cout << coeffs.A << "x² ";
	if (coeffs.B != 0) std::cout << (coeffs.B > 0 ? "+ " : "") << coeffs.B << "y² ";
	if (coeffs.C != 0) std::cout << (coeffs.C > 0 ? "+ " : "") << coeffs.C << "z² ";
	if (coeffs.D != 0) std::cout << (coeffs.D > 0 ? "+ " : "") << coeffs.D << "xy ";
	if (coeffs.E != 0) std::cout << (coeffs.E > 0 ? "+ " : "") << coeffs.E << "xz ";
	if (coeffs.F != 0) std::cout << (coeffs.F > 0 ? "+ " : "") << coeffs.F << "yz ";
	if (coeffs.G != 0) std::cout << (coeffs.G > 0 ? "+ " : "") << coeffs.G << "x ";
	if (coeffs.H != 0) std::cout << (coeffs.H > 0 ? "+ " : "") << coeffs.H << "y ";
	if (coeffs.I != 0) std::cout << (coeffs.I > 0 ? "+ " : "") << coeffs.I << "z ";
	if (coeffs.J != 0) std::cout << (coeffs.J > 0 ? "+ " : "") << coeffs.J;
	std::cout << " = 0" << std::endl;
	std::cout << "Type: " << GetQuadricTypeName(coeffs) << std::endl;
}

QuadricSurface CreateQuadricFromUserInput()
{
	std::cout << "\nEnter coefficients for: Ax² + By² + Cz² + Dxy + Exz + Fyz + Gx + Hy + Iz + J = 0\n";
	QuadricCoefficients coeffs;
	std::cout << "A: "; std::cin >> coeffs.A;
	std::cout << "B: "; std::cin >> coeffs.B;
	std::cout << "C: "; std::cin >> coeffs.C;
	std::cout << "D: "; std::cin >> coeffs.D;
	std::cout << "E: "; std::cin >> coeffs.E;
	std::cout << "F: "; std::cin >> coeffs.F;
	std::cout << "G: "; std::cin >> coeffs.G;
	std::cout << "H: "; std::cin >> coeffs.H;
	std::cout << "I: "; std::cin >> coeffs.I;
	std::cout << "J: "; std::cin >> coeffs.J;
	return QuadricSurface(coeffs);
}

QuadricSurface GetPresetQuadric(const std::string& name)
{
	if (name == "sphere") return QuadricSurface::CreateSphere(1.0f);
	if (name == "ellipsoid") return QuadricSurface::CreateEllipsoid(2.0f, 1.5f, 1.0f);
	if (name == "cylinder") return QuadricSurface::CreateCylinder(1.0f, 10.0f);
	if (name == "cone") return QuadricSurface::CreateCone(0.785f, 10.0f);
	if (name == "paraboloid") return QuadricSurface::CreateEllipticParaboloid(1.0f, 1.0f, 5.0f);
	if (name == "saddle") return QuadricSurface::CreateHyperbolicParaboloid(1.0f, 1.0f, 5.0f);
	if (name == "hyperboloid1") return QuadricSurface::CreateHyperboloidOneSheet(1.0f, 1.0f, 1.0f, 10.0f);
	if (name == "hyperboloid2") return QuadricSurface::CreateHyperboloidTwoSheets(1.0f, 1.0f, 1.0f, 10.0f);
	return QuadricSurface::CreateSphere(1.0f);
}

void TestBasicIntersection()
{
	std::cout << "\n========================================" << std::endl;
	std::cout << "TEST 1: Basic Sphere Intersection" << std::endl;
	std::cout << "========================================" << std::endl;
	
	// Create a sphere at origin with radius 2
	QuadricSurface sphere = QuadricSurface::CreateSphere(2.0f);
	PrintQuadricEquation(sphere.GetCoefficients());
	
	// Test ray from outside pointing at center
	glm::vec3 rayOrigin(0, 0, 5);
	glm::vec3 rayDirection(0, 0, -1);
	
	std::cout << "\nRay Origin: (" << rayOrigin.x << ", " << rayOrigin.y << ", " << rayOrigin.z << ")" << std::endl;
	std::cout << "Ray Direction: (" << rayDirection.x << ", " << rayDirection.y << ", " << rayDirection.z << ")" << std::endl;
	
	IntersectionResult result = sphere.Intersect(rayOrigin, rayDirection);
	
	if (result.Hit)
	{
		std::cout << "✓ HIT!" << std::endl;
		std::cout << "  Distance: " << result.Distance << std::endl;
		std::cout << "  Point: (" << result.Point.x << ", " << result.Point.y << ", " << result.Point.z << ")" << std::endl;
		std::cout << "  Normal: (" << result.Normal.x << ", " << result.Normal.y << ", " << result.Normal.z << ")" << std::endl;
	}
	else
	{
		std::cout << "✗ No intersection" << std::endl;
	}
}

void TestCustomQuadric()
{
	std::cout << "\n========================================" << std::endl;
	std::cout << "TEST 2: Custom Quadric (Ellipsoid)" << std::endl;
	std::cout << "========================================" << std::endl;
	
	// Create ellipsoid: x²/4 + y²/9 + z²/1 = 1
	QuadricCoefficients coeffs;
	coeffs.A = 1.0f / 4.0f;   // x²/4
	coeffs.B = 1.0f / 9.0f;   // y²/9
	coeffs.C = 1.0f;          // z²
	coeffs.J = -1.0f;
	
	QuadricSurface ellipsoid(coeffs);
	PrintQuadricEquation(coeffs);
	
	// Test intersection
	glm::vec3 rayOrigin(0, 0, 5);
	glm::vec3 rayDirection(0, 0, -1);
	
	IntersectionResult result = ellipsoid.Intersect(rayOrigin, rayDirection);
	
	if (result.Hit)
	{
		std::cout << "✓ HIT!" << std::endl;
		std::cout << "  Distance: " << result.Distance << std::endl;
		std::cout << "  Point: (" << std::fixed << std::setprecision(3) 
		          << result.Point.x << ", " << result.Point.y << ", " << result.Point.z << ")" << std::endl;
		std::cout << "  Normal: (" << result.Normal.x << ", " << result.Normal.y << ", " << result.Normal.z << ")" << std::endl;
	}
	else
	{
		std::cout << "✗ No intersection" << std::endl;
	}
}

void TestCylinder()
{
	std::cout << "\n========================================" << std::endl;
	std::cout << "TEST 3: Cylinder with Bounding Box" << std::endl;
	std::cout << "========================================" << std::endl;
	
	QuadricSurface cylinder = QuadricSurface::CreateCylinder(1.5f, 10.0f);
	PrintQuadricEquation(cylinder.GetCoefficients());
	std::cout << "Bounding box enabled: " << (cylinder.IsBoundingBoxEnabled() ? "YES" : "NO") << std::endl;
	
	// Test rays at different heights
	glm::vec3 origins[] = {
		glm::vec3(0, 0, 3),    // Inside height range
		glm::vec3(0, 0, 0),    // Center
		glm::vec3(0, 0, -3),   // Inside height range (bottom)
		glm::vec3(0, 0, 10)    // Outside height range
	};
	
	glm::vec3 direction(1, 0, 0);  // Ray pointing right
	
	for (int i = 0; i < 4; i++)
	{
		std::cout << "\nRay " << (i+1) << " from height z=" << origins[i].z << std::endl;
		IntersectionResult result = cylinder.Intersect(origins[i], direction);
		
		if (result.Hit)
		{
			std::cout << "  ✓ HIT at distance " << result.Distance << std::endl;
			std::cout << "  Point: (" << std::fixed << std::setprecision(2) 
			          << result.Point.x << ", " << result.Point.y << ", " << result.Point.z << ")" << std::endl;
		}
		else
		{
			std::cout << "  ✗ No hit (outside bounding box)" << std::endl;
		}
	}
}

void TestCone()
{
	std::cout << "\n========================================" << std::endl;
	std::cout << "TEST 4: Cone" << std::endl;
	std::cout << "========================================" << std::endl;
	
	// Create cone with 45 degree angle
	QuadricSurface cone = QuadricSurface::CreateCone(0.785398f, 8.0f);  // pi/4 radians
	PrintQuadricEquation(cone.GetCoefficients());
	
	// Test ray from side
	glm::vec3 rayOrigin(5, 0, 4);
	glm::vec3 rayDirection = glm::normalize(glm::vec3(-1, 0, 0));
	
	IntersectionResult result = cone.Intersect(rayOrigin, rayDirection);
	
	if (result.Hit)
	{
		std::cout << "✓ HIT!" << std::endl;
		std::cout << "  Point: (" << std::fixed << std::setprecision(2) 
		          << result.Point.x << ", " << result.Point.y << ", " << result.Point.z << ")" << std::endl;
		std::cout << "  Normal: (" << result.Normal.x << ", " << result.Normal.y << ", " << result.Normal.z << ")" << std::endl;
	}
	else
	{
		std::cout << "✗ No intersection" << std::endl;
	}
}

void TestParaboloid()
{
	std::cout << "\n========================================" << std::endl;
	std::cout << "TEST 5: Elliptic Paraboloid" << std::endl;
	std::cout << "========================================" << std::endl;
	
	QuadricSurface paraboloid = QuadricSurface::CreateEllipticParaboloid(1.0f, 1.0f, 5.0f);
	PrintQuadricEquation(paraboloid.GetCoefficients());
	
	// Test ray from above
	glm::vec3 rayOrigin(0, 0, 6);
	glm::vec3 rayDirection(0, 0, -1);
	
	IntersectionResult result = paraboloid.Intersect(rayOrigin, rayDirection);
	
	if (result.Hit)
	{
		std::cout << "✓ HIT!" << std::endl;
		std::cout << "  Point: (" << std::fixed << std::setprecision(3) 
		          << result.Point.x << ", " << result.Point.y << ", " << result.Point.z << ")" << std::endl;
		std::cout << "  Normal: (" << result.Normal.x << ", " << result.Normal.y << ", " << result.Normal.z << ")" << std::endl;
	}
	else
	{
		std::cout << "✗ No intersection" << std::endl;
	}
}

void TestUserInput()
{
	std::cout << "\n========================================" << std::endl;
	std::cout << "TEST 6: User-Defined Coefficients" << std::endl;
	std::cout << "========================================" << std::endl;
	
	char choice;
	std::cout << "Would you like to enter custom coefficients? (y/n): ";
	std::cin >> choice;
	
	if (choice == 'y' || choice == 'Y')
	{
		QuadricSurface quadric = CreateQuadricFromUserInput();
		
		std::cout << "\nYour quadric:" << std::endl;
		PrintQuadricEquation(quadric.GetCoefficients());
		
		// Test intersection
		glm::vec3 rayOrigin(0, 0, 5);
		glm::vec3 rayDirection(0, 0, -1);
		
		std::cout << "\nTesting ray from (0,0,5) pointing down..." << std::endl;
		IntersectionResult result = quadric.Intersect(rayOrigin, rayDirection);
		
		if (result.Hit)
		{
			std::cout << "✓ HIT!" << std::endl;
			std::cout << "  Distance: " << result.Distance << std::endl;
			std::cout << "  Point: (" << result.Point.x << ", " << result.Point.y << ", " << result.Point.z << ")" << std::endl;
			std::cout << "  Normal: (" << result.Normal.x << ", " << result.Normal.y << ", " << result.Normal.z << ")" << std::endl;
		}
		else
		{
			std::cout << "✗ No intersection with this ray" << std::endl;
		}
	}
	else
	{
		std::cout << "Skipped user input test." << std::endl;
	}
}

void TestAllPresets()
{
	std::cout << "\n========================================" << std::endl;
	std::cout << "TEST 7: All Preset Quadrics" << std::endl;
	std::cout << "========================================" << std::endl;
	
	const char* presets[] = {
		"sphere", "ellipsoid", "cylinder", "cone", 
		"paraboloid", "saddle", "hyperboloid1", "hyperboloid2"
	};
	
	for (const char* preset : presets)
	{
		std::cout << "\n--- " << preset << " ---" << std::endl;
		QuadricSurface quadric = GetPresetQuadric(preset);
		PrintQuadricEquation(quadric.GetCoefficients());
	}
}

int main()
{
	std::cout << "╔════════════════════════════════════════╗" << std::endl;
	std::cout << "║   QUADRIC SURFACES - TEST SUITE       ║" << std::endl;
	std::cout << "╚════════════════════════════════════════╝" << std::endl;
	
	// Run all tests
	TestBasicIntersection();
	TestCustomQuadric();
	TestCylinder();
	TestCone();
	TestParaboloid();
	TestAllPresets();
	TestUserInput();
	
	std::cout << "\n========================================" << std::endl;
	std::cout << "All tests completed!" << std::endl;
	std::cout << "========================================\n" << std::endl;
	
	return 0;
}
