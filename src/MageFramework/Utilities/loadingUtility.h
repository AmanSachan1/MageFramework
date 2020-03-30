#pragma once
#include <stdexcept>
#include <iostream>
#include <unordered_map>

#include <forward.h>

namespace loadingUtil
{
	unsigned char* loadImage(std::string imagePath, int* width, int* height, int* numChannels);
	void freeImage(unsigned char* pixels);

	bool loadObj(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices, const std::string modelPath);
	bool loadGLTF();
};