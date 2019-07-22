#include "Scene.h"

Scene::Scene(VulkanDevices* devices, uint32_t numSwapChainImages, VkExtent2D windowExtents,
	VkQueue& graphicsQueue, VkCommandPool& graphicsCommandPool,	VkQueue& computeQueue, VkCommandPool& computeCommandPool )
	: m_devices(devices), m_logicalDevice(devices->getLogicalDevice()), m_physicalDevice(devices->getPhysicalDevice()),
	m_numSwapChainImages(numSwapChainImages), m_windowExtents(windowExtents),
	m_graphicsQueue(graphicsQueue),	m_graphicsCommandPool(graphicsCommandPool),
	m_computeQueue(computeQueue), m_computeCommandPool(computeCommandPool)
{
	m_prevtime = std::chrono::high_resolution_clock::now();

	m_timeBufferSize = sizeof(TimeUBO);
	m_timeBuffers.resize(m_numSwapChainImages);
	m_timeBufferMemories.resize(m_numSwapChainImages);
	m_timeUBOs.resize(m_numSwapChainImages);
	m_mappedDataTimeBuffers.resize(m_numSwapChainImages);

	BufferUtil::createUniformBuffers(
		m_physicalDevice, m_logicalDevice, m_numSwapChainImages,
		m_timeBuffers, m_timeBufferMemories, m_timeBufferSize);

	for (uint32_t i = 0; i < numSwapChainImages; i++)
	{
		initializeTimeUBO(i);
	}

	createScene();
}
Scene::~Scene()
{
	for (uint32_t i = 0; i < m_numSwapChainImages; i++)
	{
		vkUnmapMemory(m_logicalDevice, m_timeBufferMemories[i]);
		vkDestroyBuffer(m_logicalDevice, m_timeBuffers[i], nullptr);
		vkFreeMemory(m_logicalDevice, m_timeBufferMemories[i], nullptr);
	}

	for (auto it : m_modelMap) { delete it.second; }
	m_modelMap.clear();

	for (auto it : m_textureMap) { delete it.second; }
	m_textureMap.clear();
}

void Scene::cleanup()
{
	vkDestroyDescriptorSetLayout(m_logicalDevice, m_DSL_model, nullptr);
	vkDestroyDescriptorSetLayout(m_logicalDevice, m_DSL_compute, nullptr);
	vkDestroyDescriptorSetLayout(m_logicalDevice, m_DSL_time, nullptr);
}

void Scene::createScene()
{
	Model* model = nullptr;
#ifdef DEBUG
	model = new Model(m_devices, m_graphicsQueue, m_graphicsCommandPool, m_numSwapChainImages, "thinCube.obj", "statue.jpg", false, true);
#else
	model = new Model(m_devices, m_graphicsQueue, m_graphicsCommandPool, m_numSwapChainImages, "chalet.obj", "chalet.jpg", true, true);
#endif
	m_modelMap.insert({ "house", model });
	
	for (uint32_t i = 0; i < m_numSwapChainImages; i++)
	{
		std::string name = "compute" + std::to_string(i);
		Texture* texture = new Texture(m_devices, m_graphicsQueue, m_graphicsCommandPool, VK_FORMAT_R8G8B8A8_UNORM);
		texture->createEmpty2DTexture(m_windowExtents.width, m_windowExtents.height, 1, false,
			VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, 
			VK_IMAGE_TILING_OPTIMAL, 
			VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT);
		m_textureMap.insert({ name, texture });
	}
}
void Scene::updateSceneInfrequent(VkExtent2D windowExtents)
{
	m_windowExtents = windowExtents;
}
void Scene::updateUniforms(uint32_t currentImageIndex)
{
	updateTimeUBO(currentImageIndex);

	for (auto& model : m_modelMap)
	{
		model.second->updateUniformBuffer(currentImageIndex);
	}
}

void Scene::initializeTimeUBO(uint32_t currentImageIndex)
{
	vkMapMemory(m_logicalDevice, m_timeBufferMemories[currentImageIndex], 0, m_timeBufferSize, 0, &m_mappedDataTimeBuffers[currentImageIndex]);

	m_timeUBOs[currentImageIndex].time.x = 0.0f;
	m_timeUBOs[currentImageIndex].time.y += 0.0f;

	//generate 8 numbers from the halton sequence for TXAA
	m_timeUBOs[currentImageIndex].haltonSeq1.x = TimerUtil::haltonSequenceAt(1, 3);
	m_timeUBOs[currentImageIndex].haltonSeq1.y = TimerUtil::haltonSequenceAt(2, 3);
	m_timeUBOs[currentImageIndex].haltonSeq1.z = TimerUtil::haltonSequenceAt(3, 3);
	m_timeUBOs[currentImageIndex].haltonSeq1.w = TimerUtil::haltonSequenceAt(4, 3);

	m_timeUBOs[currentImageIndex].haltonSeq2.x = TimerUtil::haltonSequenceAt(5, 3);
	m_timeUBOs[currentImageIndex].haltonSeq2.y = TimerUtil::haltonSequenceAt(6, 3);
	m_timeUBOs[currentImageIndex].haltonSeq2.z = TimerUtil::haltonSequenceAt(7, 3);
	m_timeUBOs[currentImageIndex].haltonSeq2.w = TimerUtil::haltonSequenceAt(8, 3);

	m_timeUBOs[currentImageIndex].haltonSeq3.x = TimerUtil::haltonSequenceAt(9, 3);
	m_timeUBOs[currentImageIndex].haltonSeq3.y = TimerUtil::haltonSequenceAt(10, 3);
	m_timeUBOs[currentImageIndex].haltonSeq3.z = TimerUtil::haltonSequenceAt(11, 3);
	m_timeUBOs[currentImageIndex].haltonSeq3.w = TimerUtil::haltonSequenceAt(12, 3);

	m_timeUBOs[currentImageIndex].haltonSeq4.x = TimerUtil::haltonSequenceAt(13, 3);
	m_timeUBOs[currentImageIndex].haltonSeq4.y = TimerUtil::haltonSequenceAt(14, 3);
	m_timeUBOs[currentImageIndex].haltonSeq4.z = TimerUtil::haltonSequenceAt(15, 3);
	m_timeUBOs[currentImageIndex].haltonSeq4.w = TimerUtil::haltonSequenceAt(16, 3);

	m_timeUBOs[currentImageIndex].frameCount = 0;

	memcpy(m_mappedDataTimeBuffers[currentImageIndex], &m_timeUBOs[currentImageIndex], sizeof(TimeUBO));
}
void Scene::updateTimeUBO(uint32_t currentImageIndex)
{
	const float deltaTime = TimerUtil::getTimeElapsedSinceStart(m_prevtime);

	m_timeUBOs[currentImageIndex].time.x = deltaTime;
	m_timeUBOs[currentImageIndex].time.y += m_timeUBOs[currentImageIndex].time.x;

	m_timeUBOs[currentImageIndex].frameCount += 1;
	m_timeUBOs[currentImageIndex].frameCount = m_timeUBOs[currentImageIndex].frameCount % 16;

	memcpy(m_mappedDataTimeBuffers[currentImageIndex], &m_timeUBOs[currentImageIndex], sizeof(TimeUBO));
}

void Scene::expandDescriptorPool(std::vector<VkDescriptorPoolSize>& poolSizes)
{
	// Models
	for (auto& model : m_modelMap)
	{
		model.second->addToDescriptorPoolSize(poolSizes);
	}

	// Compute
	poolSizes.push_back({ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, m_numSwapChainImages });

	// Time
	poolSizes.push_back({ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, m_numSwapChainImages });
}
void Scene::createDescriptorSetLayouts()
{
	// COMPUTE
	VkDescriptorSetLayoutBinding computeLayoutBinding = { 0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr };
	DescriptorUtil::createDescriptorSetLayout(m_logicalDevice, m_DSL_compute, 1, &computeLayoutBinding);

	// MODEL
	// One Descriptor Set Layout for all the models we create
	m_modelMap.begin()->second->createDescriptorSetLayout(m_DSL_model);
	
	// TIME
	VkDescriptorSetLayoutBinding timeSetLayoutBinding = { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_ALL, nullptr };
	DescriptorUtil::createDescriptorSetLayout(m_logicalDevice, m_DSL_time, 1, &timeSetLayoutBinding);
}
void Scene::createAndWriteDescriptorSets(VkDescriptorPool descriptorPool)
{
	// Model
	for (auto& model : m_modelMap)
	{
		model.second->m_DS_model.resize(m_numSwapChainImages);

		for (uint32_t i = 0; i < static_cast<uint32_t>(m_numSwapChainImages); i++)
		{
			std::string name = "compute" + std::to_string(i);
			Texture* computeTexture = getTexture(name);
			model.second->createAndWriteDescriptorSets(descriptorPool, m_DSL_model, computeTexture, i);
		}
	}

	m_DS_time.resize(m_numSwapChainImages);
	m_DS_compute.resize(m_numSwapChainImages);

	for (uint32_t i = 0; i < m_numSwapChainImages; i++)
	{
		// Compute
		{
			Texture* computeTex = getTexture("compute", i);
			DescriptorUtil::createDescriptorSets(m_logicalDevice, descriptorPool, 1, &m_DSL_compute, &m_DS_compute[i]);
						
			VkDescriptorImageInfo computeSetInfo = 
				DescriptorUtil::createDescriptorImageInfo(computeTex->getSampler(), computeTex->getImageView(), computeTex->getImageLayout());
			VkWriteDescriptorSet writeComputeSetInfo = 
				DescriptorUtil::writeDescriptorSet(m_DS_compute[i], 0, 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, &computeSetInfo);

			vkUpdateDescriptorSets(m_logicalDevice, 1, &writeComputeSetInfo, 0, nullptr);
		}

		// Time
		{
			DescriptorUtil::createDescriptorSets(m_logicalDevice, descriptorPool, 1, &m_DSL_time, &m_DS_time[i]);

			VkDescriptorBufferInfo timeBufferSetInfo = DescriptorUtil::createDescriptorBufferInfo(getTimeBuffer(i), 0, getTimeBufferSize());
			VkWriteDescriptorSet writeTimeSetInfo =
				DescriptorUtil::writeDescriptorSet(m_DS_time[i], 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &timeBufferSetInfo);

			vkUpdateDescriptorSets(m_logicalDevice, 1, &writeTimeSetInfo, 0, nullptr);
		}
	}
}

Model* Scene::getModel(std::string key)
{
	std::unordered_map<std::string, Model*>::const_iterator found = m_modelMap.find(key);

	if (found == m_modelMap.end())
	{
		throw std::runtime_error("failed to find the model specified");
	}

	return found->second;
}

Texture* Scene::getTexture(std::string key)
{
	std::unordered_map<std::string, Texture*>::const_iterator found = m_textureMap.find(key);

	if (found == m_textureMap.end())
	{
		throw std::runtime_error("failed to find the texture specified");
	}

	return found->second;
}
Texture* Scene::getTexture(std::string name, unsigned int index)
{
	std::string key = name + std::to_string(index);
	return getTexture(key);
}

VkBuffer Scene::getTimeBuffer(unsigned int index) const
{
	return m_timeBuffers[index];
}
uint32_t Scene::getTimeBufferSize() const
{
	return static_cast<uint32_t>(m_timeBufferSize);
}

VkDescriptorSet Scene::getDescriptorSet(DSL_TYPE type, int index, std::string key)
{
	switch (type)
	{
	case DSL_TYPE::MODEL:
		return getModel(key)->m_DS_model[index];
		break;
	case DSL_TYPE::COMPUTE:
		return m_DS_compute[index];
		break;
	case DSL_TYPE::TIME:
		return m_DS_time[index];
		break;
	default:
		throw std::runtime_error("no such Descriptor Set Layout Type (DSL_TYPE) exists");
	}

	return nullptr;
}
VkDescriptorSetLayout Scene::getDescriptorSetLayout(DSL_TYPE key)
{
	switch (key)
	{
	case DSL_TYPE::MODEL:
		return m_DSL_model;
		break;
	case DSL_TYPE::COMPUTE:
		return m_DSL_compute;
		break;
	case DSL_TYPE::TIME:
		return m_DSL_time;
		break;
	default:
		throw std::runtime_error("no such Descriptor Set Layout Type (DSL_TYPE) exists");
	}

	assert(("Did not find a Descriptor Set Layout to return", true));
	return nullptr;
}