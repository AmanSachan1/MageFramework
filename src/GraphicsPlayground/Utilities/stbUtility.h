#pragma once
#include <stdexcept>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

namespace stbUtil
{
	inline unsigned char* loadImage(std::string imagePath, int* width, int* height, int* numChannels)
	{
		std::string str = "../../src/Assets/Textures/";
		str.append(imagePath);
		const char* image_file_path = str.c_str();

		// The pointer that is returned is the first element in an array of pixel values. 
		// The pixels are laid out row by row with 4 bytes per pixel in the case of STBI_rgba_alpha for a total of texWidth * texHeight * 4 values.
		unsigned char* pixels = stbi_load(image_file_path, width, height, numChannels, STBI_rgb_alpha);
		if (!pixels)
		{
			throw std::runtime_error("failed to load image!");
		}

		return pixels;
	}

	inline void freeImage(unsigned char* pixels)
	{
		stbi_image_free(pixels);
	}
}