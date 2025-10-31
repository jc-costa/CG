#include "Renderer.h"

#include <iostream>

#include "stb_image.h"

Texture CreateTexture(int width, int height)
{
	Texture result;
	result.Width = width;
	result.Height = height;

	glGenTextures(1, &result.Handle);
	glBindTexture(GL_TEXTURE_2D, result.Handle);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glBindTexture(GL_TEXTURE_2D, 0);

	return result;
}

Texture LoadTexture(const std::filesystem::path& path)
{
	int width, height, channels;
	std::string filepath = path.string();
	unsigned char* data = stbi_load(filepath.c_str(), &width, &height, &channels, 0);

	if (!data)
	{
		std::cerr << "Failed to load texture: " << filepath << "\n";
		return {};
	}

	GLenum format = channels == 4 ? GL_RGBA :
		channels == 3 ? GL_RGB :
		channels == 1 ? GL_RED : 0;

	Texture result;
	result.Width = width;
	result.Height = height;

	glGenTextures(1, &result.Handle);
	glBindTexture(GL_TEXTURE_2D, result.Handle);

	GLenum internalFormat = format == GL_RGBA ? GL_RGBA8 : GL_RGB8;
	glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, data);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glGenerateMipmap(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);
	
	stbi_image_free(data);

	return result;
}

Framebuffer CreateFramebufferWithTexture(const Texture texture)
{
	Framebuffer result;

	glGenFramebuffers(1, &result.Handle);

	if (!AttachTextureToFramebuffer(result, texture))
	{
		glDeleteFramebuffers(1, &result.Handle);
		return {};
	}

	return result;
}

bool AttachTextureToFramebuffer(Framebuffer& framebuffer, const Texture texture)
{
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.Handle);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture.Handle, 0);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		std::cerr << "Framebuffer is not complete!" << std::endl;
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		return false;
	}

	framebuffer.ColorAttachment = texture;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	return true;
}

void BlitFramebufferToSwapchain(const Framebuffer framebuffer)
{
	glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer.Handle);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0); // swapchain

	int width = framebuffer.ColorAttachment.Width;
	int height = framebuffer.ColorAttachment.Height;
	
	glBlitFramebuffer(0, 0, width, height, // Source rect
		0, 0, width, height,               // Destination rect
		GL_COLOR_BUFFER_BIT, GL_NEAREST);
	
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}