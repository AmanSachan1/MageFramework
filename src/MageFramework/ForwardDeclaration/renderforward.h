#pragma once
#include <global.h>
#include <Vulkan/Utilities/vPipelineUtil.h>

enum class RENDER_API { VULKAN, DX12 };
enum class RENDER_TYPE { RASTERIZATION, RAYTRACE, HYBRID };
enum class PIPELINE_TYPE { COMPUTE, RASTER, RAYTRACE, POST_PROCESS };
enum class POST_PROCESS_TYPE { HIGH_RESOLUTION, TONEMAP, LOW_RESOLUTION };
enum class DSL_TYPE {
	COMPUTE, MODEL, TIME, LIGHTS,
	POST_PROCESS, BEFOREPOST_FRAME, POST_HRFRAME1, POST_HRFRAME2, POST_LRFRAME1, POST_LRFRAME2
};

// If these change we need to inform the renderer and recreate everything.
struct RendererOptions
{
	RENDER_API renderAPI;
	RENDER_TYPE renderType;
	bool MSAA;  // Geometry Anti-Aliasing
	bool FXAA;  // Fast-Approximate Anti-Aliasing
	bool TXAA;	// Temporal Anti-Aliasing
	bool enableSampleRateShading; // Shading Anti-Aliasing (enables processing more than one sample per fragment)
	float minSampleShading; // value between 0.0f and 1.0f --> closer to one is smoother
	bool enableAnisotropy; // Anisotropic filtering -- image sampling will use anisotropic filter
	float anisotropy; //controls level of anisotropic filtering
};

struct Vertex
{
	glm::vec4 position; // Not using the last float 
	glm::vec4 normal;	// Not using the last float
	glm::vec4 uv;		// Not using the last 2 floats 

	bool operator==(const Vertex& other) const 
	{
		return position == other.position && normal == other.normal && uv == other.uv;
	}

	static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions()
	{
		std::array<VkVertexInputAttributeDescription, 3> vertexInputAttributes;
		vertexInputAttributes[0] = VulkanPipelineStructures::vertexInputAttributeDesc(0, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex, position));
		vertexInputAttributes[1] = VulkanPipelineStructures::vertexInputAttributeDesc(1, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex, normal));
		vertexInputAttributes[2] = VulkanPipelineStructures::vertexInputAttributeDesc(2, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex, uv));
		//vertexInputAttributes[3] = VulkanPipelineStructures::vertexInputAttributeDesc(3, 0, VK_FORMAT_R8G8B8A8_SINT, offsetof(Vertex, textureArrayIndices));
		return vertexInputAttributes;
	}

	static VkFormat getVertexPositionFormat()
	{
		return VK_FORMAT_R32G32B32A32_SFLOAT;
	}
	static VkFormat getVertexPositionFormat_RayTracing()
	{
		return VK_FORMAT_R32G32B32_SFLOAT;
	}
};

namespace std 
{
	template<> struct hash<Vertex> 
	{
		size_t operator()(Vertex const& vertex) const 
		{
			size_t const h1 ( hash<glm::vec4>()(vertex.position) );
			size_t const h2 ( hash<glm::vec4>()(vertex.normal) );
			size_t const h3 ( hash<glm::vec4>()(vertex.uv) );

			size_t h = ((h1 ^ (h2 << 1)) >> 1) ^ 
					   (h3 >> 1); //((h3 ^ (h4 << 1)) >> 1);
			return h;
		}
	};
}