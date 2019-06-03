#pragma once

#include <iostream> 
#include <unordered_map>
#include <string>

#include <global.h>
#include <Utilities/bufferUtility.h>

#include "VulkanSetup/vulkanDevices.h"

#include "SceneElements/model.h"
#include "SceneElements/time.h"

class Scene 
{
private:
	VulkanDevices* m_devices;
	VkDevice m_logicalDevice;
	VkPhysicalDevice m_physicalDevice;
	VkQueue	m_graphicsQueue;
	VkQueue	m_computeQueue;
	VkCommandPool m_graphicsCommandPool;
	VkCommandPool m_computeCommandPool;
	int m_numSwapChainImages;
	uint32_t m_windowWidth, m_windowHeight;

	std::unordered_map<std::string, Model*> m_modelMap;
	std::unordered_map<std::string, Texture*> m_textureMap;

public:
	Scene() = delete;
	Scene(VulkanDevices* devices, int numSwapChainImages, uint32_t windowWidth, uint32_t windoHeight,
		VkQueue& graphicsQueue, VkCommandPool& graphicsCommandPool,
		VkQueue& computeQueue, VkCommandPool& computeCommandPool);
	~Scene();

	void createScene();
	void updateUniforms(uint32_t currentImageIndex);
	Model* getModel(std::string key);
	Texture* getTexture(std::string key);
};