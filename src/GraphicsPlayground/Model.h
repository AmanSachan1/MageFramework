#pragma once

#include <unordered_map>
#include <global.h>
#include <Utilities/bufferUtility.h>
#include <Utilities/imageUtility.h>

#include "vulkanDevices.h"

class Model
{
public:
	Model() = delete;
	Model(VulkanDevices* devices, VkCommandPool commandPool, const std::vector<Vertex> &vertices, const std::vector<uint32_t> &indices);
	Model(VulkanDevices* devices, VkCommandPool commandPool, const std::string model_path, const std::string texture_path);
	~Model();

	//Getters
	const std::vector<Vertex>& getVertices() const;
	VkBuffer& getVertexBuffer();
	uint32_t getVertexBufferSize() const;

	const std::vector<uint32_t>& getIndices() const;
	VkBuffer& getIndexBuffer();
	uint32_t getIndexBufferSize() const;

private:
	VkDevice m_logicalDevice;
	VkPhysicalDevice m_physicalDevice;
	
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
};