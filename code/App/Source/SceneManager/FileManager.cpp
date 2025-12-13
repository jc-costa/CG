// ============================================================================
// FILE MANAGER - Implementation
// ============================================================================
//
// Provides file I/O utilities for the path tracer application.
// See FileManager.h for detailed documentation of each method.
//
// ============================================================================

#include "FileManager.h"

#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

// ----------------------------------------------------------------------------
// ReadTextFile
// ----------------------------------------------------------------------------
// Uses std::ostringstream with rdbuf() for efficient single-pass reading.
// This avoids multiple allocations that would occur with repeated string
// concatenation.
// ----------------------------------------------------------------------------
std::optional<std::string> FileManager::ReadTextFile(const std::filesystem::path& path)
{
	std::ifstream file(path);
	
	if (!file.is_open())
	{
		std::cerr << "[FileManager] Failed to open file: " << path.string() << std::endl;
		return std::nullopt;
	}
	
	std::ostringstream contentStream;
	contentStream << file.rdbuf();
	return contentStream.str();
}

// ----------------------------------------------------------------------------
// ReadLines
// ----------------------------------------------------------------------------
// Reads file line by line using std::getline(). Each line is stored without
// the newline character. Empty lines are preserved in the output.
// ----------------------------------------------------------------------------
std::optional<std::vector<std::string>> FileManager::ReadLines(const std::filesystem::path& path)
{
	std::ifstream file(path);
	
	if (!file.is_open())
	{
		std::cerr << "[FileManager] Failed to open file: " << path.string() << std::endl;
		return std::nullopt;
	}
	
	std::vector<std::string> lines;
	std::string line;
	
	while (std::getline(file, line))
	{
		lines.push_back(line);
	}
	
	return lines;
}

// ----------------------------------------------------------------------------
// FileExists
// ----------------------------------------------------------------------------
// Simple wrapper around std::filesystem::exists().
// ----------------------------------------------------------------------------
bool FileManager::FileExists(const std::filesystem::path& path)
{
	return std::filesystem::exists(path);
}

// ----------------------------------------------------------------------------
// GetExtension
// ----------------------------------------------------------------------------
// Returns the extension in lowercase for case-insensitive comparison.
// The dot is included in the result (e.g., ".obj" not "obj").
// ----------------------------------------------------------------------------
std::string FileManager::GetExtension(const std::filesystem::path& path)
{
	std::string ext = path.extension().string();
	std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
	return ext;
}

// ----------------------------------------------------------------------------
// GetDirectory
// ----------------------------------------------------------------------------
// Returns the parent path. For "/a/b/c.txt" returns "/a/b".
// ----------------------------------------------------------------------------
std::filesystem::path FileManager::GetDirectory(const std::filesystem::path& path)
{
	return path.parent_path();
}

// ----------------------------------------------------------------------------
// ResolvePath
// ----------------------------------------------------------------------------
// Combines the directory of the base path with the relative path.
// Used primarily for resolving MTL file references from OBJ files.
// ----------------------------------------------------------------------------
std::filesystem::path FileManager::ResolvePath(const std::filesystem::path& basePath,
											   const std::filesystem::path& relativePath)
{
	return GetDirectory(basePath) / relativePath;
}
