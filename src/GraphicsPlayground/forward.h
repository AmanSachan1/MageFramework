#pragma once
#include <global.h>
#include <Utilities/pipelineUtility.h>

struct Vertex
{
	glm::vec4 position;
	glm::vec4 color;
	glm::vec2 texCoord;

	// The functions below allow us to access texture coordinates as input in the vertex shader.
	// That is necessary to be able to pass them to the fragment shader for interpolation across the surface of the square
	bool operator==(const Vertex& other) const 
	{
		return position == other.position && color == other.color && texCoord == other.texCoord;
	}

	static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions()
	{
		std::array<VkVertexInputAttributeDescription, 3> vertexInputAttributes;
		vertexInputAttributes[0] = VulkanPipelineStructures::vertexInputAttributeDesc(0, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex, position));
		vertexInputAttributes[1] = VulkanPipelineStructures::vertexInputAttributeDesc(1, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex, color));
		vertexInputAttributes[2] = VulkanPipelineStructures::vertexInputAttributeDesc(2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, texCoord));
		return vertexInputAttributes;
	}
};

struct SwapChainSupportDetails 
{
	VkSurfaceCapabilitiesKHR surfaceCapabilities;
	std::vector<VkSurfaceFormatKHR> surfaceFormats;
	std::vector<VkPresentModeKHR> presentModes;
};