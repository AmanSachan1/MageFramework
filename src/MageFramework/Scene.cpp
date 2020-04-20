#include "Scene.h"

Scene::Scene(std::shared_ptr<VulkanManager> vulkanManager, JSONItem::Scene& scene, 
	RENDER_TYPE renderType, uint32_t numSwapChainImages, VkExtent2D windowExtents,
	VkQueue& graphicsQueue, VkCommandPool& graphicsCommandPool,	VkQueue& computeQueue, VkCommandPool& computeCommandPool)
	:  m_vulkanManager(vulkanManager), m_logicalDevice(vulkanManager->getLogicalDevice()), m_physicalDevice(vulkanManager->getPhysicalDevice()),
	m_numSwapChainImages(numSwapChainImages), m_renderType(renderType),
	m_graphicsQueue(graphicsQueue),	m_graphicsCmdPool(graphicsCommandPool),
	m_computeQueue(computeQueue), m_computeCmdPool(computeCommandPool)
{
	m_prevtime = std::chrono::high_resolution_clock::now();

	m_timeUniform.resize(m_numSwapChainImages);
	m_lightsUniform.resize(m_numSwapChainImages);
	for (uint32_t i = 0; i < numSwapChainImages; i++)
	{
		initializeTimeUBO(i);
		initializeLightsUBO(i);
	}

	createScene(scene);
}
Scene::~Scene()
{
	vkDeviceWaitIdle(m_logicalDevice);

	if (m_modelMap.size() > 0)
	{
		vkDestroyDescriptorSetLayout(m_logicalDevice, m_DSL_model, nullptr);
	}
	vkDestroyDescriptorSetLayout(m_logicalDevice, m_DSL_compute, nullptr);
	vkDestroyDescriptorSetLayout(m_logicalDevice, m_DSL_time, nullptr);
	vkDestroyDescriptorSetLayout(m_logicalDevice, m_DSL_lights, nullptr);

	for (uint32_t i = 0; i < m_numSwapChainImages; i++)
	{
		m_timeUniform[i].timeBuffer.unmap(m_logicalDevice);
		m_timeUniform[i].timeBuffer.destroy(m_logicalDevice);

		m_lightsUniform[i].lightBuffer.unmap(m_logicalDevice);
		m_lightsUniform[i].lightBuffer.destroy(m_logicalDevice);
	}

	m_modelMap.clear();
	m_textureMap.clear();
}

void Scene::createScene(JSONItem::Scene& scene)
{
	for (JSONItem::Model& jsonModel : scene.modelList)
	{
		std::shared_ptr<Model> model = std::make_shared<Model>(
			m_vulkanManager, m_graphicsQueue, m_graphicsCmdPool, m_numSwapChainImages, jsonModel, true, m_renderType);
		m_modelMap.insert({ jsonModel.name, model });
	}
	
	const VkExtent2D windowExtents = m_vulkanManager->getSwapChainVkExtent();
	for (uint32_t i = 0; i < m_numSwapChainImages; i++)
	{
		std::string name = "compute" + std::to_string(i);
		std::shared_ptr<Texture2D> texture = std::make_shared<Texture2D>(m_vulkanManager, m_graphicsQueue, m_graphicsCmdPool, VK_FORMAT_R8G8B8A8_UNORM);
		texture->createEmptyTexture(windowExtents.width, windowExtents.height, 1, 1,
			m_graphicsQueue, m_graphicsCmdPool, false,
			VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, 
			VK_IMAGE_TILING_OPTIMAL, 
			VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT);
		m_textureMap.insert({ name, texture });
	}
}

void Scene::updateUniforms(uint32_t currentImageIndex)
{
	for (auto& model : m_modelMap)
	{
		model.second->updateUniformBuffer(currentImageIndex);
	}

	updateTimeUBO(currentImageIndex);
	updateLightsUBO(currentImageIndex);
}

void Scene::initializeTimeUBO(uint32_t currentImageIndex)
{
	TimeUniformBlock& l_timeUniformBlock = m_timeUniform[currentImageIndex].uniformBlock;
	l_timeUniformBlock.time.x = 0.0f;
	l_timeUniformBlock.time.y += 0.0f;

	//generate 8 numbers from the halton sequence for TXAA
	l_timeUniformBlock.haltonSeq1.x = TimerUtil::haltonSequenceAt(1, 3);
	l_timeUniformBlock.haltonSeq1.y = TimerUtil::haltonSequenceAt(2, 3);
	l_timeUniformBlock.haltonSeq1.z = TimerUtil::haltonSequenceAt(3, 3);
	l_timeUniformBlock.haltonSeq1.w = TimerUtil::haltonSequenceAt(4, 3);

	l_timeUniformBlock.haltonSeq2.x = TimerUtil::haltonSequenceAt(5, 3);
	l_timeUniformBlock.haltonSeq2.y = TimerUtil::haltonSequenceAt(6, 3);
	l_timeUniformBlock.haltonSeq2.z = TimerUtil::haltonSequenceAt(7, 3);
	l_timeUniformBlock.haltonSeq2.w = TimerUtil::haltonSequenceAt(8, 3);

	l_timeUniformBlock.haltonSeq3.x = TimerUtil::haltonSequenceAt(9, 3);
	l_timeUniformBlock.haltonSeq3.y = TimerUtil::haltonSequenceAt(10, 3);
	l_timeUniformBlock.haltonSeq3.z = TimerUtil::haltonSequenceAt(11, 3);
	l_timeUniformBlock.haltonSeq3.w = TimerUtil::haltonSequenceAt(12, 3);

	l_timeUniformBlock.haltonSeq4.x = TimerUtil::haltonSequenceAt(13, 3);
	l_timeUniformBlock.haltonSeq4.y = TimerUtil::haltonSequenceAt(14, 3);
	l_timeUniformBlock.haltonSeq4.z = TimerUtil::haltonSequenceAt(15, 3);
	l_timeUniformBlock.haltonSeq4.w = TimerUtil::haltonSequenceAt(16, 3);

	l_timeUniformBlock.frameCount = 0;

	// We keep the buffer mapped to make it easier to copy data in update
	// TODO: Unsure of memory/performance implications of keeping buffers mapped
	BufferUtil::createMageBuffer(m_logicalDevice, m_physicalDevice, m_timeUniform[currentImageIndex].timeBuffer,
		sizeof(TimeUniformBlock), nullptr, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
	m_timeUniform[currentImageIndex].timeBuffer.map(m_logicalDevice);
	m_timeUniform[currentImageIndex].timeBuffer.copyDataToMappedBuffer(&l_timeUniformBlock);
}
void Scene::updateTimeUBO(uint32_t currentImageIndex)
{
	TimeUniformBlock& l_timeUniformBlock = m_timeUniform[currentImageIndex].uniformBlock;
	const float deltaTime = static_cast<float>(TimerUtil::getTimeElapsedSinceStart(m_prevtime));

	l_timeUniformBlock.time.x = deltaTime;
	l_timeUniformBlock.time.y += l_timeUniformBlock.time.x;

	l_timeUniformBlock.frameCount += 1;
	l_timeUniformBlock.frameCount = l_timeUniformBlock.frameCount % 16;

	m_timeUniform[currentImageIndex].timeBuffer.copyDataToMappedBuffer(&l_timeUniformBlock);
}

void Scene::initializeLightsUBO(uint32_t currentImageIndex)
{
	LightsUniformBlock l_lightsUniformBlock = m_lightsUniform[currentImageIndex].uniformBlock;
	float currentTime = m_timeUniform[currentImageIndex].uniformBlock.time.y * 0.0000001f;
	l_lightsUniformBlock.lightPos =
		glm::vec4(cos(glm::radians(currentTime * 360.0f)) * 40.0f,
			-20.0f + sin(glm::radians(currentTime * 360.0f)) * 20.0f,
			25.0f + sin(glm::radians(currentTime * 360.0f)) * 5.0f,
			0.0f);

	BufferUtil::createMageBuffer(m_logicalDevice, m_physicalDevice, m_lightsUniform[currentImageIndex].lightBuffer,
		sizeof(LightsUniformBlock), nullptr, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
	m_lightsUniform[currentImageIndex].lightBuffer.map(m_logicalDevice);
	m_lightsUniform[currentImageIndex].lightBuffer.copyDataToMappedBuffer(&l_lightsUniformBlock);
}
void Scene::updateLightsUBO(uint32_t currentImageIndex)
{
	LightsUniformBlock l_lightsUniformBlock = m_lightsUniform[currentImageIndex].uniformBlock;
	float currentTime = m_timeUniform[currentImageIndex].uniformBlock.time.y * 0.0000001f;
	l_lightsUniformBlock.lightPos =
		glm::vec4(cos(glm::radians(currentTime * 360.0f)) * 40.0f,
			-20.0f + sin(glm::radians(currentTime * 360.0f)) * 20.0f,
			25.0f + sin(glm::radians(currentTime * 360.0f)) * 5.0f,
			0.0f);

	m_lightsUniform[currentImageIndex].lightBuffer.copyDataToMappedBuffer(&l_lightsUniformBlock);
}

void Scene::expandDescriptorPool(std::vector<VkDescriptorPoolSize>& poolSizes)
{
	// Models
	for (auto& model : m_modelMap)
	{
		model.second->addToDescriptorPoolSize(poolSizes);
	}

	poolSizes.push_back({ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, m_numSwapChainImages });  // Compute
	poolSizes.push_back({ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, m_numSwapChainImages }); // Time
	poolSizes.push_back({ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, m_numSwapChainImages }); // Lights
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
		if (m_modelMap.size() > 0)
		{
			m_modelMap.begin()->second->createDescriptorSetLayout(m_DSL_model);
		}		

		// COMPUTE
		VkDescriptorSetLayoutBinding computeLayoutBinding = { 0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr };
		DescriptorUtil::createDescriptorSetLayout(m_logicalDevice, m_DSL_compute, 1, &computeLayoutBinding);
		
		// TIME
		VkDescriptorSetLayoutBinding timeSetLayoutBinding = { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_ALL, nullptr };
		DescriptorUtil::createDescriptorSetLayout(m_logicalDevice, m_DSL_time, 1, &timeSetLayoutBinding);

		// LIGHTS
		VkDescriptorSetLayoutBinding lightsSetLayoutBinding = { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_ALL, nullptr };
		DescriptorUtil::createDescriptorSetLayout(m_logicalDevice, m_DSL_lights, 1, &timeSetLayoutBinding);
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
		m_DS_lights.resize(m_numSwapChainImages);
		m_DS_compute.resize(m_numSwapChainImages);

		for (uint32_t i = 0; i < m_numSwapChainImages; i++)
		{
			// Compute
			DescriptorUtil::createDescriptorSets(m_logicalDevice, descriptorPool, 1, &m_DSL_compute, &m_DS_compute[i]);
			// Time
			DescriptorUtil::createDescriptorSets(m_logicalDevice, descriptorPool, 1, &m_DSL_time, &m_DS_time[i]);
			// Lights
			DescriptorUtil::createDescriptorSets(m_logicalDevice, descriptorPool, 1, &m_DSL_lights, &m_DS_lights[i]);
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
			VkWriteDescriptorSet writeTimeSetInfo =	DescriptorUtil::writeDescriptorSet(
				m_DS_time[i], 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &m_timeUniform[i].timeBuffer.descriptorInfo);
			vkUpdateDescriptorSets(m_logicalDevice, 1, &writeTimeSetInfo, 0, nullptr);
		}

		// Lights
		{
			VkWriteDescriptorSet writeLightsSetInfo = DescriptorUtil::writeDescriptorSet(
				m_DS_lights[i], 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &m_lightsUniform[i].lightBuffer.descriptorInfo);
			vkUpdateDescriptorSets(m_logicalDevice, 1, &writeLightsSetInfo, 0, nullptr);
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
	case DSL_TYPE::LIGHTS:
		return m_DS_lights[index];
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
	case DSL_TYPE::LIGHTS:
		return m_DSL_lights;
		break;
	default:
		throw std::runtime_error("no such Descriptor Set Layout Type (DSL_TYPE) exists");
	}

	return nullptr;
}