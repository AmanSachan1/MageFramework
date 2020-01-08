#pragma once
#include <global.h>
#include <Vulkan/Utilities/vPipelineUtil.h>

struct Vertex
{
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 texCoord;
	glm::vec4 color;

	// The functions below allow us to access texture coordinates as input in the vertex shader.
	// That is necessary to be able to pass them to the fragment shader for interpolation across the surface of the square
	bool operator==(const Vertex& other) const 
	{
		return position == other.position && normal == other.normal && texCoord == other.texCoord;
	}

	static std::array<VkVertexInputAttributeDescription, 4> getAttributeDescriptions()
	{
		std::array<VkVertexInputAttributeDescription, 4> vertexInputAttributes;
		vertexInputAttributes[0] = VulkanPipelineStructures::vertexInputAttributeDesc(0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position));
		vertexInputAttributes[1] = VulkanPipelineStructures::vertexInputAttributeDesc(1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal));
		vertexInputAttributes[2] = VulkanPipelineStructures::vertexInputAttributeDesc(2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, texCoord));
		vertexInputAttributes[3] = VulkanPipelineStructures::vertexInputAttributeDesc(3, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex, color));
		return vertexInputAttributes;
	}
};

namespace std 
{
	template<> struct hash<Vertex> 
	{
		size_t operator()(Vertex const& vertex) const 
		{
			size_t const h1 ( hash<glm::vec3>()(vertex.position) );
			size_t const h2 ( hash<glm::vec3>()(vertex.normal) );
			size_t const h3 ( hash<glm::vec2>()(vertex.texCoord) );
			size_t const h4 ( hash<glm::vec4>()(vertex.color) );
			size_t h = ((h1 ^ (h2 << 1)) >> 1) ^ 
					   ((h3 ^ (h4 << 1)) >> 1);
			return h;
		}
	};
}

struct SwapChainSupportDetails 
{
	VkSurfaceCapabilitiesKHR surfaceCapabilities;
	std::vector<VkSurfaceFormatKHR> surfaceFormats;
	std::vector<VkPresentModeKHR> presentModes;
};