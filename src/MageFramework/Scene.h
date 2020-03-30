#pragma once
#include <global.h>
#include <unordered_map>
#include <string>
#include <Vulkan\Utilities\vBufferUtil.h>
#include <Vulkan\Utilities\vDescriptorUtil.h>
#include <Vulkan/vulkanManager.h>

#include "SceneElements/model.h"

struct TimeUBO
{
	//16 values stored in halton seq because 2D case
	glm::vec4 haltonSeq1;
	glm::vec4 haltonSeq2;
	glm::vec4 haltonSeq3;
	glm::vec4 haltonSeq4;
	glm::vec2 time = glm::vec2(0.0f, 0.0f); //stores delta time and total time packed as a vec2 so vulkan offsetting doesnt become an issue later
	int frameCount = 1;
};

class Scene 
{
public:
	Scene() = delete;
	Scene(std::shared_ptr<VulkanManager> vulkanManager, uint32_t numSwapChainImages, VkExtent2D windowExtents,
		VkQueue& graphicsQueue, VkCommandPool& graphicsCommandPool,	VkQueue& computeQueue, VkCommandPool& computeCommandPool);
	~Scene();

	void cleanup() {} //specifically clean up resources that are recreated on frame resizing
	void createScene();
	void updateSceneInfrequent() {}
	void updateUniforms(uint32_t currentImageIndex);

	// Time
	void initializeTimeUBO(uint32_t currentImageIndex);
	void updateTimeUBO(uint32_t currentImageIndex);

	// Descriptor Sets
	void expandDescriptorPool(std::vector<VkDescriptorPoolSize>& poolSizes);
	void createDescriptors(VkDescriptorPool descriptorPool);
	void writeToAndUpdateDescriptorSets();

	// Getters
	std::shared_ptr<Model> getModel(std::string key);
	std::shared_ptr<Texture> getTexture(std::string key);
	std::shared_ptr<Texture> getTexture(std::string key, unsigned int index);

	VkBuffer getTimeBuffer(unsigned int index) const;
	uint32_t getTimeBufferSize() const;

	VkDescriptorSet getDescriptorSet(DSL_TYPE type, int index, std::string key = "");
	VkDescriptorSetLayout getDescriptorSetLayout(DSL_TYPE key);

private:
	std::shared_ptr<VulkanManager> m_vulkanManager;
	VkDevice m_logicalDevice;
	VkPhysicalDevice m_physicalDevice;
	VkQueue	m_graphicsQueue;
	VkQueue	m_computeQueue;
	VkCommandPool m_graphicsCommandPool;
	VkCommandPool m_computeCommandPool;
	uint32_t m_numSwapChainImages;
	
	std::unordered_map<std::string, std::shared_ptr<Model>> m_modelMap;
	std::unordered_map<std::string, std::shared_ptr<Texture>> m_textureMap;
	
	//// Compute
	//std::vector<Texture*> m_computeTextures;

	// Time
	std::chrono::high_resolution_clock::time_point m_prevtime;

	std::vector<TimeUBO> m_timeUBOs;
	std::vector<VkBuffer> m_timeBuffers;
	std::vector<VkDeviceMemory> m_timeBufferMemories;
	VkDeviceSize m_timeBufferSize;
	std::vector<void*> m_mappedDataTimeBuffers;

	// Descriptor Set Stuff
	VkDescriptorSetLayout m_DSL_compute;
	VkDescriptorSetLayout m_DSL_model;
	VkDescriptorSetLayout m_DSL_time;
	std::vector<VkDescriptorSet> m_DS_compute;
	std::vector<VkDescriptorSet> m_DS_time;
};