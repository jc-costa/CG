#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include <optional>

// ============================================================================
// FILE MANAGER - Handles file I/O operations
// ============================================================================
//
// A utility class providing static methods for common file operations used
// throughout the application. All methods are static, so no instantiation
// is required.
//
// USAGE EXAMPLE:
// --------------
//   // Read entire file as string
//   auto content = FileManager::ReadTextFile("path/to/file.txt");
//   if (content.has_value()) {
//       std::cout << content.value() << std::endl;
//   }
//
//   // Read file line by line
//   auto lines = FileManager::ReadLines("path/to/file.obj");
//   if (lines.has_value()) {
//       for (const auto& line : lines.value()) {
//           // process each line
//       }
//   }
//
//   // Resolve relative path from OBJ file to MTL file
//   std::filesystem::path mtlPath = FileManager::ResolvePath(
//       "models/scene.obj",   // base path
//       "materials.mtl"       // relative path from OBJ
//   );
//   // Result: "models/materials.mtl"
//
// ============================================================================
class FileManager
{
public:
	// ========================================================================
	// ReadTextFile
	// ========================================================================
	// Reads the entire contents of a text file into a string.
	//
	// Parameters:
	//   path - Path to the file (absolute or relative to working directory)
	//
	// Returns:
	//   std::optional<std::string> - File contents if successful, std::nullopt
	//                                if file cannot be opened
	//
	// Notes:
	//   - Uses std::ifstream with rdbuf() for efficient reading
	//   - Suitable for small to medium files (shaders, config files)
	//   - For very large files, consider ReadLines() for streaming
	// ========================================================================
	static std::optional<std::string> ReadTextFile(const std::filesystem::path& path);
	
	// ========================================================================
	// ReadLines
	// ========================================================================
	// Reads a file line by line into a vector of strings.
	//
	// Parameters:
	//   path - Path to the file (absolute or relative to working directory)
	//
	// Returns:
	//   std::optional<std::vector<std::string>> - Vector of lines if successful,
	//                                             std::nullopt if file cannot be opened
	//
	// Notes:
	//   - Each line is stored without the newline character
	//   - Preserves empty lines
	//   - Ideal for parsing structured files (OBJ, MTL, CSV)
	// ========================================================================
	static std::optional<std::vector<std::string>> ReadLines(const std::filesystem::path& path);
	
	// ========================================================================
	// FileExists
	// ========================================================================
	// Checks if a file exists at the given path.
	//
	// Parameters:
	//   path - Path to check
	//
	// Returns:
	//   bool - true if file exists, false otherwise
	// ========================================================================
	static bool FileExists(const std::filesystem::path& path);
	
	// ========================================================================
	// GetExtension
	// ========================================================================
	// Gets the file extension in lowercase.
	//
	// Parameters:
	//   path - Path to extract extension from
	//
	// Returns:
	//   std::string - Lowercase extension including the dot (e.g., ".obj")
	//
	// Example:
	//   GetExtension("Model.OBJ") returns ".obj"
	// ========================================================================
	static std::string GetExtension(const std::filesystem::path& path);
	
	// ========================================================================
	// GetDirectory
	// ========================================================================
	// Gets the parent directory of a file path.
	//
	// Parameters:
	//   path - Full file path
	//
	// Returns:
	//   std::filesystem::path - Parent directory path
	//
	// Example:
	//   GetDirectory("/models/scene/box.obj") returns "/models/scene"
	// ========================================================================
	static std::filesystem::path GetDirectory(const std::filesystem::path& path);
	
	// ========================================================================
	// ResolvePath
	// ========================================================================
	// Resolves a relative path from a base file's location.
	//
	// This is useful for resolving references within files, such as when an
	// OBJ file references an MTL file with a relative path.
	//
	// Parameters:
	//   basePath     - Path to the file containing the reference
	//   relativePath - The relative path to resolve
	//
	// Returns:
	//   std::filesystem::path - Resolved absolute/full path
	//
	// Example:
	//   ResolvePath("/models/scene.obj", "./textures/wood.png")
	//   returns "/models/textures/wood.png"
	// ========================================================================
	static std::filesystem::path ResolvePath(const std::filesystem::path& basePath, 
											  const std::filesystem::path& relativePath);
};
