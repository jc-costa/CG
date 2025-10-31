#include <iostream>
#include <filesystem>
#include <vector>
#include <cstdlib>

#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

#include <glm/glm.hpp>

#include "Shader.h"
#include "Renderer.h"

static uint32_t s_GraphicsShader = -1;
static GLuint s_VAO = 0;
static GLuint s_VBO = 0;

static std::filesystem::path GetExecutableDirectory()
{
#ifdef __APPLE__
	// On macOS, get the path to the executable's directory
	uint32_t size = 0;
	_NSGetExecutablePath(nullptr, &size); // Get required buffer size
	std::vector<char> path(size);
	if (_NSGetExecutablePath(path.data(), &size) == 0)
	{
		return std::filesystem::path(path.data()).parent_path();
	}
#endif
	// Fallback: use current directory (works when running from build/App/)
	return std::filesystem::current_path();
}

static std::filesystem::path GetShaderPath(const std::filesystem::path& shaderName)
{
	std::filesystem::path exeDir = GetExecutableDirectory();
	std::filesystem::path shaderPath = exeDir / shaderName;
	
	// If shader not found relative to executable, try relative to current directory
	if (!std::filesystem::exists(shaderPath))
	{
		shaderPath = shaderName;
	}
	
	return shaderPath;
}

static std::filesystem::path s_VertexShaderPath;
static std::filesystem::path s_FragmentShaderPath;

static void ErrorCallback(int error, const char* description)
{
	std::cerr << "Error: " << description << std::endl;
}

static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GLFW_TRUE);

	if (key == GLFW_KEY_R)
		s_GraphicsShader = ReloadGraphicsShader(s_GraphicsShader, s_VertexShaderPath, s_FragmentShaderPath);
}

int main()
{
	glfwSetErrorCallback(ErrorCallback);

	if (!glfwInit())
		exit(EXIT_FAILURE);

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	int width = 1280;
	int height = 720;

	GLFWwindow* window = glfwCreateWindow(width, height, "Spheres", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	glfwSetKeyCallback(window, KeyCallback);

	glfwMakeContextCurrent(window);
	int version = gladLoadGL(glfwGetProcAddress);
	if (version == 0)
	{
		std::cerr << "Failed to initialize OpenGL context\n";
		glfwTerminate();
		return -1;
	}

	int major = GLAD_VERSION_MAJOR(version);
	int minor = GLAD_VERSION_MINOR(version);
	std::cout << "Loaded OpenGL " << major << "." << minor << std::endl;

	glfwSwapInterval(1);

	// Create full-screen quad for rendering
	float quadVertices[] = {
		-1.0f, -1.0f,
		 1.0f, -1.0f,
		-1.0f,  1.0f,
		 1.0f,  1.0f,
	};

	glGenVertexArrays(1, &s_VAO);
	glGenBuffers(1, &s_VBO);

	glBindVertexArray(s_VAO);
	glBindBuffer(GL_ARRAY_BUFFER, s_VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);

	glBindVertexArray(0);

	// Load graphics shaders
	s_VertexShaderPath = GetShaderPath("Shaders/Spheres/Vertex.glsl");
	s_FragmentShaderPath = GetShaderPath("Shaders/Spheres/Fragment.glsl");
	s_GraphicsShader = CreateGraphicsShader(s_VertexShaderPath, s_FragmentShaderPath);
	if (s_GraphicsShader == -1)
	{
		std::cerr << "Graphics shader failed\n";
		glfwTerminate();
		return -1;
	}

	Texture renderTexture = CreateTexture(width, height);
	Framebuffer fb = CreateFramebufferWithTexture(renderTexture);
	
	while (!glfwWindowShouldClose(window))
	{
		glfwGetFramebufferSize(window, &width, &height);

		// Resize texture and framebuffer if window size changed
		if (width != renderTexture.Width || height != renderTexture.Height)
		{
			glDeleteTextures(1, &renderTexture.Handle);
			renderTexture = CreateTexture(width, height);
			AttachTextureToFramebuffer(fb, renderTexture);
		}

		// Render to framebuffer
		glBindFramebuffer(GL_FRAMEBUFFER, fb.Handle);
		glViewport(0, 0, width, height);

		glUseProgram(s_GraphicsShader);
		glBindVertexArray(s_VAO);
		
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		glBindVertexArray(0);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		// Blit framebuffer to screen
		BlitFramebufferToSwapchain(fb);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// Cleanup
	glDeleteVertexArrays(1, &s_VAO);
	glDeleteBuffers(1, &s_VBO);
	glDeleteTextures(1, &renderTexture.Handle);
	glDeleteFramebuffers(1, &fb.Handle);
	glDeleteProgram(s_GraphicsShader);

	glfwDestroyWindow(window);
	glfwTerminate();
}
