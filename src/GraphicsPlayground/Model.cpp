#include "model.h"

Model::Model(VulkanDevices* devices, VkCommandPool& commandPool, std::vector<Vertex> &vertices, std::vector<uint32_t> &indices)
	: m_devices(devices), m_logicalDevice(devices->getLogicalDevice()), m_physicalDevice(devices->getPhysicalDevice()),
	m_vertices(vertices), m_indices(indices)
{
	m_vertexBufferSize = vertices.size() * sizeof(vertices[0]);
	m_indexBufferSize = indices.size() * sizeof(indices[0]);

	BufferUtil::createVertexBuffer(m_devices, m_physicalDevice, m_logicalDevice, commandPool, 
		m_vertexBuffer, m_vertexBufferMemory, m_vertexBufferSize,
		m_mappedDataVertexBuffer, vertices.data());
}
Model::Model(VulkanDevices* devices, VkCommandPool& commandPool, const std::string model_path, const std::string texture_path)
{

}
Model::~Model()
{
	vkDestroyBuffer(m_logicalDevice, m_vertexBuffer, nullptr);
	vkFreeMemory(m_logicalDevice, m_vertexBufferMemory, nullptr);
}

//Getters
const std::vector<Vertex>& Model::getVertices() const
{
	return m_vertices;
}
VkBuffer Model::getVertexBuffer()
{
	return m_vertexBuffer;
}
uint32_t Model::getVertexBufferSize() const 
{
	return static_cast<uint32_t>(m_vertexBufferSize);
}

const std::vector<uint32_t>& Model::getIndices() const
{
	return m_indices;
}
VkBuffer Model::getIndexBuffer()
{
	return m_indexBuffer;
}
uint32_t Model::getIndexBufferSize() const
{
	return static_cast<uint32_t>(m_indexBufferSize);
}