#pragma once
#include <global.h>
#include <unordered_map>
#include <string>
#include <Vulkan/Utilities/vBufferUtil.h>
#include <Vulkan/Utilities/vDescriptorUtil.h>
#include <Vulkan/vulkanManager.h>

#include "SceneElements/model.h"
#include "Utilities/loadingUtility.h"

struct TimeUniformBlock
{
	//16 values stored in halton seq because 2D case
	glm::vec4 haltonSeq1;
	glm::vec4 haltonSeq2;
	glm::vec4 haltonSeq3;
	glm::vec4 haltonSeq4;
	glm::vec2 time = glm::vec2(0.0f, 0.0f); //stores delta time and total time packed as a vec2 so vulkan offsetting doesnt become an issue later
	int frameCount = 1;
};

struct TimeUniform
{
	TimeUniformBlock uniformBlock;
	mageVKBuffer timeBuffer;
};

struct LightsUniformBlock
{
	glm::vec4 lightPos;
};

struct LightsUniform
{
	LightsUniformBlock uniformBlock;
	mageVKBuffer lightBuffer;
};

class Scene 
{
public:
	Scene() = delete;
	Scene(std::shared_ptr<VulkanManager> vulkanManager, JSONItem::Scene& scene,
		RENDER_TYPE renderType, uint32_t numSwapChainImages, VkExtent2D windowExtents,
		VkQueue& graphicsQueue, VkCommandPool& graphicsCommandPool, VkQueue& computeQueue, VkCommandPool& computeCommandPool);
	~Scene();

	void cleanup() {} //specifically clean up resources that are recreated on frame resizing
	void createScene(JSONItem::Scene& scene);
	void recreate();
	void updateSceneInfrequent() {}
	void updateUniforms(uint32_t currentImageIndex);

	// Time
	void initializeTimeUBO(uint32_t currentImageIndex);
	void updateTimeUBO(uint32_t currentImageIndex);

	// Lights
	void initializeLightsUBO(uint32_t currentImageIndex);
	void updateLightsUBO(uint32_t currentImageIndex);

	// Descriptor Sets
	void expandDescriptorPool(std::vector<VkDescriptorPoolSize>& poolSizes);
	void createDescriptors(VkDescriptorPool descriptorPool);
	void writeToAndUpdateDescriptorSets();

	// Getters
	VkDescriptorSet getDescriptorSet(DSL_TYPE type, int index, std::string key = "");
	VkDescriptorSetLayout getDescriptorSetLayout(DSL_TYPE key);

	std::shared_ptr<Model> Scene::getModel(std::string key)
	{
		std::unordered_map<std::string, std::shared_ptr<Model>>::const_iterator found = m_modelMap.find(key);
		if (found == m_modelMap.end()) { throw std::runtime_error("failed to find the model specified"); }
		return found->second;
	};
	std::shared_ptr<Texture2D> Scene::getTexture(std::string key)
	{
		std::unordered_map<std::string, std::shared_ptr<Texture2D>>::const_iterator found = m_textureMap.find(key);
		if (found == m_textureMap.end()) { throw std::runtime_error("failed to find the texture specified"); }
		return found->second;
	};
	std::shared_ptr<Texture2D> Scene::getTexture(std::string name, unsigned int index)
	{
		std::string key = name + std::to_string(index);
		return getTexture(key);
	};

public:
	std::unordered_map<std::string, std::shared_ptr<Model>> m_modelMap;
	std::unordered_map<std::string, std::shared_ptr<Texture2D>> m_textureMap;
	
	std::vector<TimeUniform> m_timeUniform; // Time	
	std::vector<LightsUniform> m_lightsUniform; // Lights

private:
	std::shared_ptr<VulkanManager> m_vulkanManager;
	VkDevice m_logicalDevice;
	VkPhysicalDevice m_physicalDevice;
	VkQueue	m_graphicsQueue;
	VkQueue	m_computeQueue;
	VkCommandPool m_graphicsCmdPool;
	VkCommandPool m_computeCmdPool;
	uint32_t m_numSwapChainImages;
	RENDER_TYPE m_renderType;

	std::chrono::high_resolution_clock::time_point m_prevtime;
	
	// Descriptor Set Stuff
	VkDescriptorSetLayout m_DSL_model;
	VkDescriptorSetLayout m_DSL_compute;
	VkDescriptorSetLayout m_DSL_time;
	VkDescriptorSetLayout m_DSL_lights;
	std::vector<VkDescriptorSet> m_DS_compute;
	std::vector<VkDescriptorSet> m_DS_time;
	std::vector<VkDescriptorSet> m_DS_lights;
};