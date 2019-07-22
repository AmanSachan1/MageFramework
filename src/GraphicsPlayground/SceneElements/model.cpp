#include "model.h"
#include <Utilities/loadingUtility.h>

Model::Model(VulkanDevices* devices, VkQueue& graphicsQueue, VkCommandPool& commandPool, unsigned int numSwapChainImages,
	std::vector<Vertex> &vertices, std::vector<uint32_t> &indices, bool isMipMapped, bool yAxisIsUp)
	: m_devices(devices), m_logicalDevice(devices->getLogicalDevice()), m_physicalDevice(devices->getPhysicalDevice()),
	m_vertices(vertices), m_indices(indices), m_numSwapChainImages(numSwapChainImages), m_yAxisIsUp(yAxisIsUp)
{
	m_vertexBufferSize = vertices.size() * sizeof(vertices[0]);
	m_indexBufferSize = indices.size() * sizeof(indices[0]);

	m_uniformBufferSize = sizeof(ModelUBO);
	m_uniformBuffers.resize(m_numSwapChainImages);
	m_uniformBufferMemories.resize(m_numSwapChainImages);
	m_modelUBOs.resize(m_numSwapChainImages);
	m_mappedDataUniformBuffers.resize(m_numSwapChainImages);

	BufferUtil::createVertexBuffer(m_physicalDevice, m_logicalDevice, graphicsQueue, commandPool,
		m_vertexBuffer, m_vertexBufferMemory, m_vertexBufferSize, m_mappedDataVertexBuffer, vertices.data());

	BufferUtil::createIndexBuffer(m_physicalDevice, m_logicalDevice, graphicsQueue, commandPool,
		m_indexBuffer, m_indexBufferMemory, m_indexBufferSize, m_mappedDataIndexBuffer, indices.data());

	BufferUtil::createUniformBuffers(m_physicalDevice, m_logicalDevice, m_numSwapChainImages,
		m_uniformBuffers, m_uniformBufferMemories, m_uniformBufferSize);

	for (unsigned int i = 0; i < numSwapChainImages; i++)
	{
		vkMapMemory(m_logicalDevice, m_uniformBufferMemories[i], 0, m_uniformBufferSize, 0, &m_mappedDataUniformBuffers[i]);
		memcpy(m_mappedDataUniformBuffers[i], &m_modelUBOs[i], (size_t)m_uniformBufferSize);
	}
}
Model::Model(VulkanDevices* devices, VkQueue& graphicsQueue, VkCommandPool& commandPool, unsigned int numSwapChainImages,
	const std::string model_path, const std::string texture_path, bool isMipMapped, bool yAxisIsUp) :
	m_devices(devices), m_logicalDevice(devices->getLogicalDevice()), m_physicalDevice(devices->getPhysicalDevice()),
	m_numSwapChainImages(numSwapChainImages), m_yAxisIsUp(yAxisIsUp)
{
	loadingUtil::loadObj(m_vertices, m_indices, model_path);
	m_texture = new Texture(m_devices, graphicsQueue, commandPool, VK_FORMAT_R8G8B8A8_UNORM);
	m_texture->create2DTexture(texture_path, isMipMapped);

	m_vertexBufferSize = m_vertices.size() * sizeof(m_vertices[0]);
	m_indexBufferSize = m_indices.size() * sizeof(m_indices[0]);

	m_uniformBufferSize = sizeof(ModelUBO);
	m_uniformBuffers.resize(m_numSwapChainImages);
	m_uniformBufferMemories.resize(m_numSwapChainImages);
	m_modelUBOs.resize(m_numSwapChainImages);
	m_mappedDataUniformBuffers.resize(m_numSwapChainImages);

	BufferUtil::createVertexBuffer(m_physicalDevice, m_logicalDevice, graphicsQueue, commandPool,
		m_vertexBuffer, m_vertexBufferMemory, m_vertexBufferSize, m_mappedDataVertexBuffer, m_vertices.data());

	BufferUtil::createIndexBuffer(m_physicalDevice, m_logicalDevice, graphicsQueue, commandPool,
		m_indexBuffer, m_indexBufferMemory, m_indexBufferSize, m_mappedDataIndexBuffer, m_indices.data());

	BufferUtil::createUniformBuffers(m_physicalDevice, m_logicalDevice, m_numSwapChainImages,
		m_uniformBuffers, m_uniformBufferMemories, m_uniformBufferSize);

	for (unsigned int i = 0; i < numSwapChainImages; i++)
	{
		vkMapMemory(m_logicalDevice, m_uniformBufferMemories[i], 0, m_uniformBufferSize, 0, &m_mappedDataUniformBuffers[i]);
		memcpy(m_mappedDataUniformBuffers[i], &m_modelUBOs[i], (size_t)m_uniformBufferSize);
	}
}
Model::~Model()
{
	// Destroy Descriptor Stuff
	m_DS_model.clear();

	delete m_texture;

	vkDestroyBuffer(m_logicalDevice, m_indexBuffer, nullptr);
	vkFreeMemory(m_logicalDevice, m_indexBufferMemory, nullptr);

	vkDestroyBuffer(m_logicalDevice, m_vertexBuffer, nullptr);
	vkFreeMemory(m_logicalDevice, m_vertexBufferMemory, nullptr);

	for (unsigned int i = 0; i < m_numSwapChainImages; i++)
	{
		vkUnmapMemory(m_logicalDevice, m_uniformBufferMemories[i]);
		vkDestroyBuffer(m_logicalDevice, m_uniformBuffers[i], nullptr);
		vkFreeMemory(m_logicalDevice, m_uniformBufferMemories[i], nullptr);		
	}
}

void Model::updateUniformBuffer(uint32_t currentImageIndex)
{
	const float deltaTime = TimerUtil::getTimeElapsedSinceStart();

	glm::mat4 m = glm::mat4(1.0f);

	// Usually, y is the up axis for the model, if that's not true then we assume z is the up axis
	if(!m_yAxisIsUp)
	{
		m = glm::rotate(m,glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	}

	m_modelUBOs[currentImageIndex].modelMat = m;
	memcpy(m_mappedDataUniformBuffers[currentImageIndex], &m_modelUBOs[currentImageIndex], (size_t)m_uniformBufferSize);
}

// Descriptor Setup
void Model::addToDescriptorPoolSize(std::vector<VkDescriptorPoolSize>& poolSizes)
{
	poolSizes.push_back({ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, m_numSwapChainImages });			// Model Matrix
	poolSizes.push_back({ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, m_numSwapChainImages });	// Model Texture
	//poolSizes.push_back({ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, m_numSwapChainImages });	// Compute Texture used to color the model somehow
}
void Model::createDescriptorSetLayout(VkDescriptorSetLayout& DSL_model)
{
	//We pass in a descriptor Set layout because it will remain common to all models that we create. So no point keep multiple copies of it.
	VkDescriptorSetLayoutBinding modelLayoutBinding =		{ 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,			1, VK_SHADER_STAGE_ALL,			 nullptr };
	VkDescriptorSetLayoutBinding samplerLayoutBinding =		{ 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr };
	//VkDescriptorSetLayoutBinding readComputeLayoutBinding = { 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr };
	std::array<VkDescriptorSetLayoutBinding, 2> graphicsBindings = { modelLayoutBinding, samplerLayoutBinding };// , readComputeLayoutBinding

	DescriptorUtil::createDescriptorSetLayout(m_logicalDevice, DSL_model, static_cast<uint32_t>(graphicsBindings.size()), graphicsBindings.data());
}
void Model::createAndWriteDescriptorSets(VkDescriptorPool descriptorPool, VkDescriptorSetLayout& DSL_model, Texture* computeTexture, uint32_t index)
{
	DescriptorUtil::createDescriptorSets(m_logicalDevice, descriptorPool, 1, &DSL_model, &m_DS_model[index]);

	std::array<VkWriteDescriptorSet, 2> writeGraphicsSetInfo = {};
	VkDescriptorBufferInfo modelBufferSetInfo;
	VkDescriptorImageInfo samplerImageSetInfo;// , readComputeSamplerImageSetInfo;

	// Model
	modelBufferSetInfo = DescriptorUtil::createDescriptorBufferInfo(getUniformBuffer(index), 0, getUniformBufferSize());
	samplerImageSetInfo = DescriptorUtil::createDescriptorImageInfo(
		m_texture->getSampler(),
		m_texture->getImageView(),
		m_texture->getImageLayout());
	//readComputeSamplerImageSetInfo = DescriptorUtil::createDescriptorImageInfo(
	//	computeTexture->getSampler(),
	//	computeTexture->getImageView(),
	//	computeTexture->getImageLayout());;

	writeGraphicsSetInfo[0] = DescriptorUtil::writeDescriptorSet(m_DS_model[index], 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &modelBufferSetInfo);
	writeGraphicsSetInfo[1] = DescriptorUtil::writeDescriptorSet(m_DS_model[index], 1, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &samplerImageSetInfo);
	//writeGraphicsSetInfo[2] = DescriptorUtil::writeDescriptorSet(m_DS_model[index], 2, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &readComputeSamplerImageSetInfo);

	vkUpdateDescriptorSets(m_logicalDevice, static_cast<uint32_t>(writeGraphicsSetInfo.size()), writeGraphicsSetInfo.data(), 0, nullptr);
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
const uint32_t Model::getNumIndices() const
{
	return static_cast<uint32_t>(m_indices.size());
}
VkBuffer& Model::getIndexBuffer()
{
	return m_indexBuffer;
}
uint32_t Model::getIndexBufferSize() const
{
	return static_cast<uint32_t>(m_indexBufferSize);
}

Texture* Model::getTexture() const
{
	return m_texture;
}

VkBuffer& Model::getUniformBuffer(unsigned int bufferIndex)
{
	return m_uniformBuffers[bufferIndex];
}
uint32_t Model::getUniformBufferSize() const
{
	return static_cast<uint32_t>(m_uniformBufferSize);
}