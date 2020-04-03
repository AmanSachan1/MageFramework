#include "Scene.h"

Scene::Scene(std::shared_ptr<VulkanManager> vulkanManager, JSONItem::Scene& scene, uint32_t numSwapChainImages, VkExtent2D windowExtents,
	VkQueue& graphicsQueue, VkCommandPool& graphicsCommandPool,	VkQueue& computeQueue, VkCommandPool& computeCommandPool )
	:  m_vulkanManager(vulkanManager), m_logicalDevice(vulkanManager->getLogicalDevice()), m_physicalDevice(vulkanManager->getPhysicalDevice()),
	m_numSwapChainImages(numSwapChainImages),
	m_graphicsQueue(graphicsQueue),	m_graphicsCommandPool(graphicsCommandPool),
	m_computeQueue(computeQueue), m_computeCommandPool(computeCommandPool)
{
	m_prevtime = std::chrono::high_resolution_clock::now();

	m_timeUniform.resize(m_numSwapChainImages);
	for (uint32_t i = 0; i < numSwapChainImages; i++)
	{
		UniformBufferObject& timeUBO = m_timeUniform[i].ubo;
		timeUBO.bufferSize = sizeof(TimeUniform);
		BufferUtil::createUniformBuffer(m_logicalDevice, m_physicalDevice, timeUBO.buffer, timeUBO.memory, timeUBO.bufferSize);
		initializeTimeUBO(i);
	}

	createScene(scene);
}
Scene::~Scene()
{
	vkDeviceWaitIdle(m_logicalDevice);

	vkDestroyDescriptorSetLayout(m_logicalDevice, m_DSL_model, nullptr);
	vkDestroyDescriptorSetLayout(m_logicalDevice, m_DSL_compute, nullptr);
	vkDestroyDescriptorSetLayout(m_logicalDevice, m_DSL_time, nullptr);

	for (uint32_t i = 0; i < m_numSwapChainImages; i++)
	{
		vkUnmapMemory(m_logicalDevice, m_timeUniform[i].ubo.memory);
		vkDestroyBuffer(m_logicalDevice, m_timeUniform[i].ubo.buffer, nullptr);
		vkFreeMemory(m_logicalDevice, m_timeUniform[i].ubo.memory, nullptr);
	}

	m_modelMap.clear();
	m_textureMap.clear();
}

void Scene::createScene(JSONItem::Scene& scene)
{
	for (JSONItem::Model& jsonModel : scene.modelList)
	{
		std::shared_ptr<Model> model = std::make_shared<Model>(m_vulkanManager, m_graphicsQueue, m_graphicsCommandPool, m_numSwapChainImages, jsonModel, true);
		m_modelMap.insert({ jsonModel.name, model });
	}
	
	const VkExtent2D windowExtents = m_vulkanManager->getSwapChainVkExtent();
	for (uint32_t i = 0; i < m_numSwapChainImages; i++)
	{
		std::string name = "compute" + std::to_string(i);
		std::shared_ptr<Texture2D> texture = std::make_shared<Texture2D>(m_vulkanManager, m_graphicsQueue, m_graphicsCommandPool, VK_FORMAT_R8G8B8A8_UNORM);
		texture->createEmptyTexture(windowExtents.width, windowExtents.height, 1, 1,
			m_graphicsQueue, m_graphicsCommandPool, false,
			VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, 
			VK_IMAGE_TILING_OPTIMAL, 
			VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT);
		m_textureMap.insert({ name, texture });
	}
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
	UniformBufferObject& timeUBO = m_timeUniform[currentImageIndex].ubo;
	TimeUniformBlock& timeUniformBlock = m_timeUniform[currentImageIndex].uniformBlock;
	vkMapMemory(m_logicalDevice, timeUBO.memory, 0, timeUBO.bufferSize, 0, &timeUBO.mappedData);

	timeUniformBlock.time.x = 0.0f;
	timeUniformBlock.time.y += 0.0f;

	//generate 8 numbers from the halton sequence for TXAA
	timeUniformBlock.haltonSeq1.x = TimerUtil::haltonSequenceAt(1, 3);
	timeUniformBlock.haltonSeq1.y = TimerUtil::haltonSequenceAt(2, 3);
	timeUniformBlock.haltonSeq1.z = TimerUtil::haltonSequenceAt(3, 3);
	timeUniformBlock.haltonSeq1.w = TimerUtil::haltonSequenceAt(4, 3);

	timeUniformBlock.haltonSeq2.x = TimerUtil::haltonSequenceAt(5, 3);
	timeUniformBlock.haltonSeq2.y = TimerUtil::haltonSequenceAt(6, 3);
	timeUniformBlock.haltonSeq2.z = TimerUtil::haltonSequenceAt(7, 3);
	timeUniformBlock.haltonSeq2.w = TimerUtil::haltonSequenceAt(8, 3);

	timeUniformBlock.haltonSeq3.x = TimerUtil::haltonSequenceAt(9, 3);
	timeUniformBlock.haltonSeq3.y = TimerUtil::haltonSequenceAt(10, 3);
	timeUniformBlock.haltonSeq3.z = TimerUtil::haltonSequenceAt(11, 3);
	timeUniformBlock.haltonSeq3.w = TimerUtil::haltonSequenceAt(12, 3);

	timeUniformBlock.haltonSeq4.x = TimerUtil::haltonSequenceAt(13, 3);
	timeUniformBlock.haltonSeq4.y = TimerUtil::haltonSequenceAt(14, 3);
	timeUniformBlock.haltonSeq4.z = TimerUtil::haltonSequenceAt(15, 3);
	timeUniformBlock.haltonSeq4.w = TimerUtil::haltonSequenceAt(16, 3);

	timeUniformBlock.frameCount = 0;

	memcpy(timeUBO.mappedData, &timeUniformBlock, sizeof(TimeUniform));
}
void Scene::updateTimeUBO(uint32_t currentImageIndex)
{
	UniformBufferObject& timeUBO = m_timeUniform[currentImageIndex].ubo;
	TimeUniformBlock& timeUniformBlock = m_timeUniform[currentImageIndex].uniformBlock;
	const float deltaTime = static_cast<float>(TimerUtil::getTimeElapsedSinceStart(m_prevtime));

	timeUniformBlock.time.x = deltaTime;
	timeUniformBlock.time.y += timeUniformBlock.time.x;

	timeUniformBlock.frameCount += 1;
	timeUniformBlock.frameCount = timeUniformBlock.frameCount % 16;

	memcpy(timeUBO.mappedData, &timeUniformBlock, sizeof(TimeUniform));
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
void Scene::createDescriptors(VkDescriptorPool descriptorPool)
{
	// Descriptor Set Layouts
	{
		// Descriptor set layouts are specified in the pipeline layout object., i.e. during pipeline creation to tell Vulkan 
		// which descriptors the shaders will be using.
		// The numbers are bindingCount, binding, and descriptorCount respectively

		// MODEL
		// One Descriptor Set Layout for all the models we create
		m_modelMap.begin()->second->createDescriptorSetLayout(m_DSL_model);

		// COMPUTE
		VkDescriptorSetLayoutBinding computeLayoutBinding = { 0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr };
		DescriptorUtil::createDescriptorSetLayout(m_logicalDevice, m_DSL_compute, 1, &computeLayoutBinding);
		
		// TIME
		VkDescriptorSetLayoutBinding timeSetLayoutBinding = { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_ALL, nullptr };
		DescriptorUtil::createDescriptorSetLayout(m_logicalDevice, m_DSL_time, 1, &timeSetLayoutBinding);
	}

	// Descriptor Sets
	{
		// Model
		for (auto& model : m_modelMap)
		{
			for (uint32_t i = 0; i < static_cast<uint32_t>(m_numSwapChainImages); i++)
			{
				model.second->createDescriptorSets(descriptorPool, m_DSL_model, i);
			}
		}

		m_DS_time.resize(m_numSwapChainImages);
		m_DS_compute.resize(m_numSwapChainImages);

		for (uint32_t i = 0; i < m_numSwapChainImages; i++)
		{
			// Compute
			DescriptorUtil::createDescriptorSets(m_logicalDevice, descriptorPool, 1, &m_DSL_compute, &m_DS_compute[i]);

			// Time
			DescriptorUtil::createDescriptorSets(m_logicalDevice, descriptorPool, 1, &m_DSL_time, &m_DS_time[i]);
		}
	}
}
void Scene::writeToAndUpdateDescriptorSets()
{
	for (uint32_t i = 0; i < m_numSwapChainImages; i++)
	{
		// Models
		for (auto& model : m_modelMap) { model.second->writeToAndUpdateDescriptorSets(i); }

		// Compute
		{
			std::shared_ptr<Texture2D> computeTex = getTexture("compute", i);
			VkDescriptorImageInfo computeSetInfo = DescriptorUtil::createDescriptorImageInfo(computeTex->m_sampler, computeTex->m_imageView, computeTex->m_imageLayout);
			VkWriteDescriptorSet writeComputeSetInfo = DescriptorUtil::writeDescriptorSet(m_DS_compute[i], 0, 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, &computeSetInfo);
			vkUpdateDescriptorSets(m_logicalDevice, 1, &writeComputeSetInfo, 0, nullptr);
		}

		// Time
		{
			m_timeUniform[i].descriptorInfo = DescriptorUtil::createDescriptorBufferInfo(m_timeUniform[i].ubo.buffer, 0, m_timeUniform[i].ubo.bufferSize);
			VkWriteDescriptorSet writeTimeSetInfo =	DescriptorUtil::writeDescriptorSet(m_DS_time[i], 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &m_timeUniform[i].descriptorInfo);
			vkUpdateDescriptorSets(m_logicalDevice, 1, &writeTimeSetInfo, 0, nullptr);
		}
	}
}

VkDescriptorSet Scene::getDescriptorSet(DSL_TYPE type, int index, std::string key)
{
	switch (type)
	{
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

	return nullptr;
}