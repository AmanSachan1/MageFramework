#pragma once
#include <global.h>
#include <unordered_map>
#include <Vulkan/Utilities/vBufferUtil.h>

// Disable Warnings: 
#pragma warning( disable : 28020 ) // C28020: The expression <expr> is not true at this call

#include <json.hpp>
using json = nlohmann::json;

#include <Utilities/loadingUtilityForward.h>
#include <SceneElements/modelForward.h>
#include <SceneElements/texture.h>
#include <SceneElements/model.h>


namespace glm
{
	inline void to_json(json& j, const vec3& v)
	{
		j = json{ {"x", v.x}, {"y", v.y}, {"z", v.z} };
	}

	inline void from_json(const json& j, vec3& v)
	{
		j.at("x").get_to(v.x);
		j.at("y").get_to(v.y);
		j.at("z").get_to(v.z);
	}
} // namespace glm

namespace loadingUtil
{
	// Use STB library to load image into a staging buffer
	void loadImageUsingSTB(const std::string filename, ImageLoaderOutput& out, VkDevice& logicalDevice, VkPhysicalDevice& pDevice);
	void loadArrayOfImageUsingSTB(std::vector<std::string>& texturePaths, ImageArrayLoaderOutput& out, VkDevice& logicalDevice, VkPhysicalDevice& pDevice);
	
	bool loadObj(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices, std::vector<std::shared_ptr<Texture2D>>& textures,
		const std::string meshFilePath, const std::vector<std::string>& textureFilePaths, bool areTexturesMipMapped,
		VkDevice& logicalDevice, VkPhysicalDevice& pDevice, VkQueue& graphicsQueue, VkCommandPool& commandPool);
	bool loadGLTF(std::vector<Vertex>& vertexBuffer, std::vector<uint32_t>& indexBuffer,
		std::vector<std::shared_ptr<Texture2D>>& textures, std::vector<vkMaterial*>& materials,
		std::vector<vkNode*>& nodes, std::vector<vkNode*>& linearNodes,
		const std::string filename, glm::mat4& transform, uint32_t& primitiveCount, uint32_t& materialCount, unsigned int numFrames,
		VkDevice& logicalDevice, VkPhysicalDevice& pDevice, VkQueue& graphicsQueue, VkCommandPool& commandPool);

	void convertObjToNodeStructure(Vertices& vertices, Indices& indices,
		std::vector<std::shared_ptr<Texture2D>>& textures, std::vector<vkMaterial*>& materials,
		std::vector<vkNode*>& nodes, std::vector<vkNode*>& linearNodes,
		const std::string filename, glm::mat4& transform, uint32_t& primitiveCount, uint32_t& materialCount, unsigned int numFrames,
		VkDevice& logicalDevice, VkPhysicalDevice& pDevice, VkQueue& graphicsQueue, VkCommandPool& commandPool);
		
	JSONContents loadJSON(const std::string jsonFilePath);
};