#include "model.h"

Model::Model(VulkanDevices* devices, VkCommandPool commandPool, const std::vector<Vertex> &vertices, const std::vector<uint32_t> &indices)
	: m_logicalDevice(devices->getLogicalDevice()), m_physicalDevice(devices->getPhysicalDevice()),
	m_vertices(vertices), m_indices(indices)
{
	m_vertexBufferSize = vertices.size() * sizeof(vertices[0]);
	m_indexBufferSize = indices.size() * sizeof(indices[0]);

	BufferUtil::createBuffer(m_physicalDevice, m_logicalDevice, 
		m_vertexBuffer, m_vertexBufferMemory, m_vertexBufferSize,
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 
		VK_SHARING_MODE_EXCLUSIVE, 
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	vkMapMemory(m_logicalDevice, m_vertexBufferMemory, 0, m_vertexBufferSize, 0, &m_mappedDataVertexBuffer);
	memcpy(m_mappedDataVertexBuffer, vertices.data(), static_cast<size_t>(m_vertexBufferSize));
}
Model::Model(VulkanDevices* devices, VkCommandPool commandPool, const std::string model_path, const std::string texture_path)
{

}
Model::~Model()
{
	vkUnmapMemory(m_logicalDevice, m_vertexBufferMemory);
	vkDestroyBuffer(m_logicalDevice, m_vertexBuffer, nullptr);
	vkFreeMemory(m_logicalDevice, m_vertexBufferMemory, nullptr);
}

//Getters
const std::vector<Vertex>& Model::getVertices() const
{
	return m_vertices;
}
VkBuffer& Model::getVertexBuffer()
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
VkBuffer& Model::getIndexBuffer()
{
	return m_indexBuffer;
}
uint32_t Model::getIndexBufferSize() const
{
	return static_cast<uint32_t>(m_indexBufferSize);
}