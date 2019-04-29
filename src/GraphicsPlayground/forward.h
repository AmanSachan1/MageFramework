#pragma once
#include <global.h>

struct Vertex
{
	glm::vec4 position;
	glm::vec4 color;
};

struct SwapChainSupportDetails 
{
	VkSurfaceCapabilitiesKHR surfaceCapabilities;
	std::vector<VkSurfaceFormatKHR> surfaceFormats;
	std::vector<VkPresentModeKHR> presentModes;
};