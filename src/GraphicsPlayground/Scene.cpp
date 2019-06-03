#include "Scene.h"

Scene::Scene(VulkanDevices* devices, int numSwapChainImages, uint32_t windowWidth, uint32_t windoHeight,
	VkQueue& graphicsQueue, VkCommandPool& graphicsCommandPool,
	VkQueue& computeQueue, VkCommandPool& computeCommandPool )
	: m_devices(devices), m_logicalDevice(devices->getLogicalDevice()), m_physicalDevice(devices->getPhysicalDevice()),
	m_numSwapChainImages(numSwapChainImages), m_windowWidth(windowWidth), m_windowHeight(windoHeight),
	m_graphicsQueue(graphicsQueue),	m_graphicsCommandPool(graphicsCommandPool),
	m_computeQueue(computeQueue), m_computeCommandPool(computeCommandPool)
{}
Scene::~Scene()
{
	for (auto it : m_modelMap)
	{
		delete it.second;
	}
	m_modelMap.clear();

	for (auto it : m_textureMap)
	{
		delete it.second;
	}
	m_textureMap.clear();
}

void Scene::createScene()
{
	Model* model = nullptr;
#ifdef DEBUG
	model = new Model(m_devices, m_graphicsQueue, m_graphicsCommandPool, m_numSwapChainImages, "teapot.obj", "statue.jpg", false, true);
#else
	model = new Model(m_devices, m_graphicsQueue, m_graphicsCommandPool, m_numSwapChainImages, "chalet.obj", "chalet.jpg", true, false);
#endif
	m_modelMap.insert({ "house", model });
	
	for (int i = 0; i < m_numSwapChainImages; i++)
	{
		std::string name = "compute" + std::to_string(i);
		Texture* texture = new Texture(m_devices, m_graphicsQueue, m_graphicsCommandPool, VK_FORMAT_R8G8B8A8_UNORM);
		texture->createEmpty2DTexture("chalet.jpg", m_windowWidth, m_windowHeight, 1, false,
			VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT);
		m_textureMap.insert({ name, texture });
	}
}

void Scene::updateUniforms(uint32_t currentImageIndex)
{
	for (auto& model : m_modelMap)
	{
		model.second->updateUniformBuffer(currentImageIndex);
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
		throw std::runtime_error("failed to find the model specified");
	}

	return found->second;
}