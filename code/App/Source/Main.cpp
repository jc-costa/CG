#include <iostream>

#include <glm/glm.hpp>

#include "Shader.h"
#include "Renderer.h"

static uint32_t s_ComputeShader = -1;
static const std::filesystem::path s_ComputeShaderPath = "Shaders/Compute.glsl";

static void ErrorCallback(int error, const char* description)
{
	std::cerr << "Error: " << description << std::endl;
}

static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GLFW_TRUE);

	if (key == GLFW_KEY_R)
		s_ComputeShader = ReloadComputeShader(s_ComputeShader, s_ComputeShaderPath);
}

int main()
{
	glfwSetErrorCallback(ErrorCallback);

	if (!glfwInit())
		exit(EXIT_FAILURE);

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);

	int width = 1280;
	int height = 720;

	GLFWwindow* window = glfwCreateWindow(width, height, "Compute", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	glfwSetKeyCallback(window, KeyCallback);

	glfwMakeContextCurrent(window);
	gladLoadGL(glfwGetProcAddress);
	glfwSwapInterval(1);

	s_ComputeShader = CreateComputeShader(s_ComputeShaderPath);
	if (s_ComputeShader == -1)
	{
		std::cerr << "Compute shader failed\n";
		return -1;
	}

	Texture computeShaderTexture = CreateTexture(width, height);
	Framebuffer fb = CreateFramebufferWithTexture(computeShaderTexture);
	
	while (!glfwWindowShouldClose(window))
	{
		glfwGetFramebufferSize(window, &width, &height);

		// Resize texture
		if (width != computeShaderTexture.Width || height != computeShaderTexture.Height)
		{
			glDeleteTextures(1, &computeShaderTexture.Handle);
			computeShaderTexture = CreateTexture(width, height);
			AttachTextureToFramebuffer(fb, computeShaderTexture);
		}

		// Compute
		{
			glUseProgram(s_ComputeShader);
			glBindImageTexture(0, fb.ColorAttachment.Handle, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

			const GLuint workGroupSizeX = 16;
			const GLuint workGroupSizeY = 16;

			GLuint numGroupsX = (width + workGroupSizeX - 1) / workGroupSizeX;
			GLuint numGroupsY = (height + workGroupSizeY - 1) / workGroupSizeY;

			glDispatchCompute(numGroupsX, numGroupsY, 1);

			// Ensure all writes to the image are complete
			glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		}

		// Blit
		{
			BlitFramebufferToSwapchain(fb);
		}

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwDestroyWindow(window);

	glfwTerminate();
}
