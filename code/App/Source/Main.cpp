// ============================================================================
// CINEMATIC PATH TRACER - Production Quality GPU Implementation
// ============================================================================

#include <iostream>
#include <filesystem>
#include <vector>
#include <cstdlib>
#include <chrono>

#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Shader.h"
#include "Renderer.h"

// ============================================================================
// CONFIGURATION
// ============================================================================
static constexpr int INITIAL_WIDTH = 1920;
static constexpr int INITIAL_HEIGHT = 1080;
static constexpr int MAX_BOUNCES = 8;
static constexpr float CAMERA_SPEED = 3.0f;
static constexpr float MOUSE_SENSITIVITY = 0.002f;

// ============================================================================
// CAMERA SYSTEM
// ============================================================================
struct Camera
{
	glm::vec3 Position = glm::vec3(0.0f, 0.0f, 8.0f);
	glm::vec3 Forward = glm::vec3(0.0f, 0.0f, -1.0f);
	glm::vec3 Up = glm::vec3(0.0f, 1.0f, 0.0f);
	
	float VerticalFOV = 45.0f;
	float NearClip = 0.1f;
	float FarClip = 100.0f;
	
	// Depth of field
	float FocusDistance = 8.0f;
	float Aperture = 0.0f;  // 0 = no DOF, 0.1 = subtle
	
	// Post-processing
	float Exposure = 1.0f;
	float Gamma = 2.2f;
	int Tonemapper = 2;  // ACES
	
	glm::mat4 Projection = glm::mat4(1.0f);
	glm::mat4 View = glm::mat4(1.0f);
	glm::mat4 InverseProjection = glm::mat4(1.0f);
	glm::mat4 InverseView = glm::mat4(1.0f);
	
	glm::vec2 LastMousePos = glm::vec2(0.0f);
	bool FirstMouse = true;
	
	void RecalculateProjection(int width, int height)
	{
		float aspect = (float)width / (float)height;
		Projection = glm::perspective(glm::radians(VerticalFOV), aspect, NearClip, FarClip);
		InverseProjection = glm::inverse(Projection);
	}
	
	void RecalculateView()
	{
		View = glm::lookAt(Position, Position + Forward, Up);
		InverseView = glm::inverse(View);
	}
	
	bool Update(float deltaTime, GLFWwindow* window)
	{
		double mouseX, mouseY;
		glfwGetCursorPos(window, &mouseX, &mouseY);
		glm::vec2 mousePos((float)mouseX, (float)mouseY);
		
		if (FirstMouse)
		{
			LastMousePos = mousePos;
			FirstMouse = false;
		}
		
		glm::vec2 delta = (mousePos - LastMousePos) * MOUSE_SENSITIVITY;
		LastMousePos = mousePos;
		
		// Only move camera when right mouse button is pressed
		if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) != GLFW_PRESS)
		{
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			return false;
		}
		
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		
		bool moved = false;
		glm::vec3 right = glm::normalize(glm::cross(Forward, Up));
		
		float speed = CAMERA_SPEED;
		if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
			speed *= 3.0f;
		
		// WASD movement
		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		{
			Position += Forward * speed * deltaTime;
			moved = true;
		}
		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		{
			Position -= Forward * speed * deltaTime;
			moved = true;
		}
		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		{
			Position -= right * speed * deltaTime;
			moved = true;
		}
		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		{
			Position += right * speed * deltaTime;
			moved = true;
		}
		if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
		{
			Position -= Up * speed * deltaTime;
			moved = true;
		}
		if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
		{
			Position += Up * speed * deltaTime;
			moved = true;
		}
		
		// Mouse look rotation
		if (delta.x != 0.0f || delta.y != 0.0f)
		{
			float yaw = -delta.x;
			float pitch = -delta.y;
			
			// Create rotation quaternions
			glm::mat4 yawRot = glm::rotate(glm::mat4(1.0f), yaw, Up);
			glm::mat4 pitchRot = glm::rotate(glm::mat4(1.0f), pitch, right);
			
			Forward = glm::normalize(glm::vec3(pitchRot * yawRot * glm::vec4(Forward, 0.0f)));
			
			moved = true;
		}
		
		if (moved)
		{
			RecalculateView();
		}
		
		return moved;
	}
};

// ============================================================================
// GLOBALS
// ============================================================================
static Camera s_Camera;
static GLuint s_PathTraceShader = 0;
static GLuint s_AccumulateShader = 0;
static GLuint s_DisplayShader = 0;
static GLuint s_VAO = 0;

// We need 3 framebuffers:
// - pathTraceFB: temporary storage for new sample each frame
// - accumFB[0], accumFB[1]: ping-pong buffers for accumulated result
static Texture s_PathTraceTexture;
static Framebuffer s_PathTraceFB;
static Texture s_AccumTextures[2];
static Framebuffer s_AccumFB[2];

static int s_FrameIndex = 0;
static bool s_ResetAccumulation = true;
static int s_Width = INITIAL_WIDTH;
static int s_Height = INITIAL_HEIGHT;
static int s_MaxBounces = MAX_BOUNCES;

static std::filesystem::path s_ShaderDir;

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================
static std::filesystem::path GetExecutableDirectory()
{
#ifdef __APPLE__
	uint32_t size = 0;
	_NSGetExecutablePath(nullptr, &size);
	std::vector<char> path(size);
	if (_NSGetExecutablePath(path.data(), &size) == 0)
	{
		return std::filesystem::path(path.data()).parent_path();
	}
#endif
	return std::filesystem::current_path();
}

static std::filesystem::path GetShaderPath(const std::filesystem::path& shaderName)
{
	std::filesystem::path exeDir = GetExecutableDirectory();
	std::filesystem::path shaderPath = exeDir / shaderName;
	
	if (!std::filesystem::exists(shaderPath))
	{
		shaderPath = shaderName;
	}
	
	return shaderPath;
}

// ============================================================================
// CALLBACKS
// ============================================================================
static void ErrorCallback(int error, const char* description)
{
	std::cerr << "GLFW Error " << error << ": " << description << std::endl;
}

static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GLFW_TRUE);
	
	// Reload shaders
	if (key == GLFW_KEY_R && action == GLFW_PRESS)
	{
		std::cout << "Reloading shaders..." << std::endl;
		
		uint32_t newPathTrace = ReloadGraphicsShader(s_PathTraceShader, 
			GetShaderPath("Shaders/PathTrace/Vertex.glsl"),
			GetShaderPath("Shaders/PathTrace/PathTrace.glsl"));
		
		uint32_t newAccumulate = ReloadGraphicsShader(s_AccumulateShader,
			GetShaderPath("Shaders/PathTrace/Vertex.glsl"),
			GetShaderPath("Shaders/PathTrace/Accumulate.glsl"));
		
		uint32_t newDisplay = ReloadGraphicsShader(s_DisplayShader,
			GetShaderPath("Shaders/PathTrace/Vertex.glsl"),
			GetShaderPath("Shaders/PathTrace/Display.glsl"));
		
		if (newPathTrace != (uint32_t)-1 && newAccumulate != (uint32_t)-1 && newDisplay != (uint32_t)-1)
		{
			s_PathTraceShader = newPathTrace;
			s_AccumulateShader = newAccumulate;
			s_DisplayShader = newDisplay;
			s_ResetAccumulation = true;
			std::cout << "Shaders reloaded successfully!" << std::endl;
		}
		else
		{
			std::cerr << "Shader reload failed!" << std::endl;
		}
	}
	
	// Tonemapper cycling
	if (key == GLFW_KEY_T && action == GLFW_PRESS)
	{
		s_Camera.Tonemapper = (s_Camera.Tonemapper + 1) % 4;
		const char* names[] = {"None", "Reinhard", "ACES", "Uncharted2"};
		std::cout << "Tonemapper: " << names[s_Camera.Tonemapper] << std::endl;
	}
	
	// Exposure controls
	if (key == GLFW_KEY_EQUAL && action == GLFW_PRESS)
	{
		s_Camera.Exposure *= 1.2f;
		std::cout << "Exposure: " << s_Camera.Exposure << std::endl;
	}
	if (key == GLFW_KEY_MINUS && action == GLFW_PRESS)
	{
		s_Camera.Exposure /= 1.2f;
		std::cout << "Exposure: " << s_Camera.Exposure << std::endl;
	}
	
	// Bounce count
	if (key == GLFW_KEY_UP && action == GLFW_PRESS)
	{
		s_MaxBounces = std::min(s_MaxBounces + 1, 16);
		s_ResetAccumulation = true;
		std::cout << "Max bounces: " << s_MaxBounces << std::endl;
	}
	if (key == GLFW_KEY_DOWN && action == GLFW_PRESS)
	{
		s_MaxBounces = std::max(s_MaxBounces - 1, 1);
		s_ResetAccumulation = true;
		std::cout << "Max bounces: " << s_MaxBounces << std::endl;
	}
	
	// DOF controls
	if (key == GLFW_KEY_F && action == GLFW_PRESS)
	{
		s_Camera.Aperture = (s_Camera.Aperture > 0.0f) ? 0.0f : 0.05f;
		s_ResetAccumulation = true;
		std::cout << "DOF: " << (s_Camera.Aperture > 0 ? "ON" : "OFF") << std::endl;
	}
}

static void FramebufferSizeCallback(GLFWwindow* window, int width, int height)
{
	// Just mark for reset - let main loop handle the actual resize
	// to ensure InitializeFramebuffers() is called
	if (width > 0 && height > 0)
	{
		s_ResetAccumulation = true;
	}
}

// ============================================================================
// INITIALIZATION
// ============================================================================
static bool InitializeFramebuffers()
{
	// Cleanup old resources
	if (s_PathTraceTexture.Handle)
		glDeleteTextures(1, &s_PathTraceTexture.Handle);
	if (s_PathTraceFB.Handle)
		glDeleteFramebuffers(1, &s_PathTraceFB.Handle);
	
	for (int i = 0; i < 2; i++)
	{
		if (s_AccumTextures[i].Handle)
			glDeleteTextures(1, &s_AccumTextures[i].Handle);
		if (s_AccumFB[i].Handle)
			glDeleteFramebuffers(1, &s_AccumFB[i].Handle);
	}
	
	// Create path trace framebuffer (temporary for each frame's sample)
	s_PathTraceTexture = CreateTexture(s_Width, s_Height);
	s_PathTraceFB = CreateFramebufferWithTexture(s_PathTraceTexture);
	if (!s_PathTraceFB.Handle)
	{
		std::cerr << "Failed to create path trace framebuffer" << std::endl;
		return false;
	}
	
	// Create accumulation ping-pong framebuffers
	for (int i = 0; i < 2; i++)
	{
		s_AccumTextures[i] = CreateTexture(s_Width, s_Height);
		s_AccumFB[i] = CreateFramebufferWithTexture(s_AccumTextures[i]);
		
		if (!s_AccumFB[i].Handle)
		{
			std::cerr << "Failed to create accumulation framebuffer " << i << std::endl;
			return false;
		}
	}
	return true;
}

static bool InitializeShaders()
{
	s_PathTraceShader = CreateGraphicsShader(
		GetShaderPath("Shaders/PathTrace/Vertex.glsl"),
		GetShaderPath("Shaders/PathTrace/PathTrace.glsl"));
	
	s_AccumulateShader = CreateGraphicsShader(
		GetShaderPath("Shaders/PathTrace/Vertex.glsl"),
		GetShaderPath("Shaders/PathTrace/Accumulate.glsl"));
	
	s_DisplayShader = CreateGraphicsShader(
		GetShaderPath("Shaders/PathTrace/Vertex.glsl"),
		GetShaderPath("Shaders/PathTrace/Display.glsl"));
	
	if (s_PathTraceShader == (uint32_t)-1 || 
		s_AccumulateShader == (uint32_t)-1 || 
		s_DisplayShader == (uint32_t)-1)
	{
		std::cerr << "Failed to compile shaders!" << std::endl;
		return false;
	}
	
	return true;
}

// ============================================================================
// RENDER PASSES
// ============================================================================
static void RenderPathTrace()
{
	// Render new sample to dedicated path trace framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, s_PathTraceFB.Handle);
	glViewport(0, 0, s_Width, s_Height);
	
	glUseProgram(s_PathTraceShader);
	
	// Set uniforms
	glUniform1i(glGetUniformLocation(s_PathTraceShader, "uFrame"), s_FrameIndex);
	glUniform1i(glGetUniformLocation(s_PathTraceShader, "uBounces"), s_MaxBounces);
	glUniform2f(glGetUniformLocation(s_PathTraceShader, "uResolution"), (float)s_Width, (float)s_Height);
	glUniform3fv(glGetUniformLocation(s_PathTraceShader, "uCameraPosition"), 1, glm::value_ptr(s_Camera.Position));
	glUniformMatrix4fv(glGetUniformLocation(s_PathTraceShader, "uInverseProjection"), 1, GL_FALSE, glm::value_ptr(s_Camera.InverseProjection));
	glUniformMatrix4fv(glGetUniformLocation(s_PathTraceShader, "uInverseView"), 1, GL_FALSE, glm::value_ptr(s_Camera.InverseView));
	glUniform1f(glGetUniformLocation(s_PathTraceShader, "uTime"), (float)glfwGetTime());
	glUniform1f(glGetUniformLocation(s_PathTraceShader, "uAperture"), s_Camera.Aperture);
	glUniform1f(glGetUniformLocation(s_PathTraceShader, "uFocusDistance"), s_Camera.FocusDistance);
	
	glBindVertexArray(s_VAO);
	glDrawArrays(GL_TRIANGLES, 0, 3);
}

static void RenderAccumulate(int srcAccumIndex, int dstAccumIndex)
{
	// Accumulate: blend new sample with previous accumulated result
	// Read from: s_PathTraceTexture (new sample) + s_AccumTextures[srcAccumIndex] (previous)
	// Write to: s_AccumFB[dstAccumIndex]
	glBindFramebuffer(GL_FRAMEBUFFER, s_AccumFB[dstAccumIndex].Handle);
	glViewport(0, 0, s_Width, s_Height);
	
	glUseProgram(s_AccumulateShader);
	
	// New sample from path trace pass
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, s_PathTraceTexture.Handle);
	glUniform1i(glGetUniformLocation(s_AccumulateShader, "uNewSample"), 0);
	
	// Previous accumulated result
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, s_AccumTextures[srcAccumIndex].Handle);
	glUniform1i(glGetUniformLocation(s_AccumulateShader, "uAccumulated"), 1);
	
	glUniform1i(glGetUniformLocation(s_AccumulateShader, "uFrame"), s_FrameIndex);
	
	glBindVertexArray(s_VAO);
	glDrawArrays(GL_TRIANGLES, 0, 3);
}

static void RenderDisplay(int accumIndex)
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, s_Width, s_Height);
	glClear(GL_COLOR_BUFFER_BIT);
	
	glUseProgram(s_DisplayShader);
	
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, s_AccumTextures[accumIndex].Handle);
	glUniform1i(glGetUniformLocation(s_DisplayShader, "uTexture"), 0);
	
	glUniform1f(glGetUniformLocation(s_DisplayShader, "uExposure"), s_Camera.Exposure);
	glUniform1f(glGetUniformLocation(s_DisplayShader, "uGamma"), s_Camera.Gamma);
	glUniform1i(glGetUniformLocation(s_DisplayShader, "uTonemapper"), s_Camera.Tonemapper);
	
	glBindVertexArray(s_VAO);
	glDrawArrays(GL_TRIANGLES, 0, 3);
}

// ============================================================================
// MAIN
// ============================================================================
int main()
{
	glfwSetErrorCallback(ErrorCallback);
	
	if (!glfwInit())
	{
		std::cerr << "Failed to initialize GLFW" << std::endl;
		return EXIT_FAILURE;
	}
	
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_SAMPLES, 0);  // No MSAA, we do our own AA
	
	GLFWwindow* window = glfwCreateWindow(s_Width, s_Height, "Cinematic Path Tracer", NULL, NULL);
	if (!window)
	{
		std::cerr << "Failed to create window" << std::endl;
		glfwTerminate();
		return EXIT_FAILURE;
	}
	
	glfwSetKeyCallback(window, KeyCallback);
	glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);
	
	glfwMakeContextCurrent(window);
	int version = gladLoadGL(glfwGetProcAddress);
	if (version == 0)
	{
		std::cerr << "Failed to initialize OpenGL" << std::endl;
		glfwTerminate();
		return EXIT_FAILURE;
	}
	
	int major = GLAD_VERSION_MAJOR(version);
	int minor = GLAD_VERSION_MINOR(version);
	std::cout << "OpenGL " << major << "." << minor << std::endl;
	std::cout << "Renderer: " << glGetString(GL_RENDERER) << std::endl;
	
	glfwSwapInterval(1);  // VSync
	
	// Create empty VAO for fullscreen triangle
	glGenVertexArrays(1, &s_VAO);
	
	// Initialize camera
	s_Camera.RecalculateProjection(s_Width, s_Height);
	s_Camera.RecalculateView();
	
	// Initialize resources
	if (!InitializeShaders())
	{
		glfwTerminate();
		return EXIT_FAILURE;
	}
	
	if (!InitializeFramebuffers())
	{
		glfwTerminate();
		return EXIT_FAILURE;
	}
	
	std::cout << "\n=== CONTROLS ===" << std::endl;
	std::cout << "Right Mouse + WASD: Move camera" << std::endl;
	std::cout << "Q/E: Move up/down" << std::endl;
	std::cout << "Shift: Move faster" << std::endl;
	std::cout << "R: Reload shaders" << std::endl;
	std::cout << "T: Cycle tonemapper" << std::endl;
	std::cout << "+/-: Adjust exposure" << std::endl;
	std::cout << "Up/Down: Adjust bounces" << std::endl;
	std::cout << "F: Toggle depth of field" << std::endl;
	std::cout << "ESC: Quit" << std::endl;
	std::cout << "================\n" << std::endl;
	
	auto lastTime = std::chrono::high_resolution_clock::now();
	float fpsTimer = 0.0f;
	int fpsCounter = 0;
	
	// Main render loop
	while (!glfwWindowShouldClose(window))
	{
		auto currentTime = std::chrono::high_resolution_clock::now();
		float deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
		lastTime = currentTime;
		
		// FPS counter
		fpsTimer += deltaTime;
		fpsCounter++;
		if (fpsTimer >= 1.0f)
		{
			char title[256];
			snprintf(title, sizeof(title), "Cinematic Path Tracer | %d FPS | Frame %d | %d bounces", 
				fpsCounter, s_FrameIndex, s_MaxBounces);
			glfwSetWindowTitle(window, title);
			fpsTimer = 0.0f;
			fpsCounter = 0;
		}
		
		// Update camera
		if (s_Camera.Update(deltaTime, window))
		{
			s_ResetAccumulation = true;
		}
		
		// Handle resize
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);
		if (width != s_Width || height != s_Height)
		{
			s_Width = width;
			s_Height = height;
			s_Camera.RecalculateProjection(width, height);
			InitializeFramebuffers();
			s_ResetAccumulation = true;
		}
		
		// Reset accumulation
		if (s_ResetAccumulation)
		{
			s_FrameIndex = 0;
			s_ResetAccumulation = false;
		}
		
		// Ping-pong accumulation buffer indices
		// srcAccum: where previous accumulated result is stored
		// dstAccum: where new accumulated result will be written
		int srcAccum = s_FrameIndex % 2;
		int dstAccum = 1 - srcAccum;
		
		// Pass 1: Path trace new sample → s_PathTraceFB
		RenderPathTrace();
		
		// Pass 2: Accumulate (new sample + prev accum → dst accum)
		RenderAccumulate(srcAccum, dstAccum);
		
		// Pass 3: Display the accumulated result
		RenderDisplay(dstAccum);
		
		glfwSwapBuffers(window);
		glfwPollEvents();
		
		s_FrameIndex++;
	}
	
	// Cleanup
	glDeleteVertexArrays(1, &s_VAO);
	glDeleteTextures(1, &s_PathTraceTexture.Handle);
	glDeleteFramebuffers(1, &s_PathTraceFB.Handle);
	for (int i = 0; i < 2; i++)
	{
		glDeleteTextures(1, &s_AccumTextures[i].Handle);
		glDeleteFramebuffers(1, &s_AccumFB[i].Handle);
	}
	glDeleteProgram(s_PathTraceShader);
	glDeleteProgram(s_AccumulateShader);
	glDeleteProgram(s_DisplayShader);
	
	glfwDestroyWindow(window);
	glfwTerminate();
	
	return EXIT_SUCCESS;
}
