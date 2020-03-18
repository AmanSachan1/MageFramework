#pragma once

#include <unordered_map>
#include <global.h>
#include "Vulkan/vulkanManager.h"
#include <Vulkan\Utilities\vBufferUtil.h>
#include <Vulkan\Utilities\vRenderUtil.h>
#include <Vulkan\Utilities\vDescriptorUtil.h>

#include "texture.h"

struct ModelUBO
{
	glm::mat4 modelMat;
};

class Model
{
public:
	Model() = delete;
	Model(std::shared_ptr<VulkanManager> vulkanObj, VkQueue& graphicsQueue, VkCommandPool& commandPool, unsigned int numSwapChainImages, std::vector<Vertex> &vertices, std::vector<uint32_t> &indices, bool isMipMapped = false, bool yAxisIsUp = true);
	Model(std::shared_ptr<VulkanManager> vulkanObj, VkQueue& graphicsQueue, VkCommandPool& commandPool, unsigned int numSwapChainImages, const std::string model_path, const std::string texture_path, bool isMipMapped = false, bool yAxisIsUp = true);
	~Model();

	void updateUniformBuffer(uint32_t currentImageIndex);

	// Descriptor Sets
	void addToDescriptorPoolSize(std::vector<VkDescriptorPoolSize>& poolSizes);
	void createDescriptorSetLayout(VkDescriptorSetLayout& DSL_model);
	void createDescriptorSets(VkDescriptorPool descriptorPool, VkDescriptorSetLayout& DSL_model, uint32_t index);
	void writeToAndUpdateDescriptorSets(std::shared_ptr<Texture> computeTexture, uint32_t index);

	//Getters
	const std::vector<Vertex>& getVertices() const;
	VkBuffer& getVertexBuffer();
	uint32_t getVertexBufferSize() const;

	const std::vector<uint32_t>& getIndices() const;
	const uint32_t getNumIndices() const;
	VkBuffer& getIndexBuffer();
	uint32_t getIndexBufferSize() const;

	std::shared_ptr<Texture> getTexture() const;

	VkBuffer& getUniformBuffer(unsigned int bufferIndex);
	uint32_t getUniformBufferSize() const;

private:
	VkDevice m_logicalDevice;
	VkPhysicalDevice m_physicalDevice;
	uint32_t m_numSwapChainImages;
	bool m_yAxisIsUp;
	
	std::vector<Vertex> m_vertices;
	VkBuffer m_vertexBuffer;
	VkDeviceMemory m_vertexBufferMemory;
	VkDeviceSize m_vertexBufferSize;
	void* m_mappedDataVertexBuffer;

	std::vector<uint32_t> m_indices;
	VkBuffer m_indexBuffer;
	VkDeviceMemory m_indexBufferMemory;
	VkDeviceSize m_indexBufferSize;
	void* m_mappedDataIndexBuffer;

	std::shared_ptr<Texture> m_texture;

	// Multiple buffers for UBO because multiple frames may be in flight at the same time and this is data that could potentially be updated every frame
	// This is also why it wouldnt make sense to use the staging buffer, the overhead of that may lead to worse performance
	std::vector<ModelUBO> m_modelUBOs;
	std::vector<VkBuffer> m_uniformBuffers;
	std::vector<VkDeviceMemory> m_uniformBufferMemories;
	VkDeviceSize m_uniformBufferSize;
	std::vector<void*> m_mappedDataUniformBuffers;

public:
	//Descriptor Stuff
	std::vector<VkDescriptorSet> m_DS_model;
};