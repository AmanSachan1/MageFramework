#pragma once

#include <unordered_map>
#include <global.h>
#include <Vulkan/vulkanManager.h>
#include <Vulkan/Utilities/vBufferUtil.h>
#include <Vulkan/Utilities/vRenderUtil.h>
#include <Vulkan/Utilities/vDescriptorUtil.h>
#include <Utilities/loadingUtility.h>
#include "Vulkan/RendererBackend/vAccelerationStructure.h"
#include <SceneElements/modelForward.h>

class Model
{
public:
	Model() = delete;
	Model(std::shared_ptr<VulkanManager> vulkanManager, VkQueue& graphicsQueue, VkCommandPool& commandPool, unsigned int numSwapChainImages, 
		const JSONItem::Model& jsonModel, bool isMipMapped = false, RENDER_TYPE renderType = RENDER_TYPE::RASTERIZATION);
	~Model();

	void updateUniformBuffer(uint32_t currentImageIndex);
	glm::mat3x4 getMatrix_3x4()
	{
		return glm::mat3x4(glm::row(m_transform, 0), glm::row(m_transform, 1), glm::row(m_transform, 2));
	}

	// Descriptor Sets
	void addToDescriptorPoolSize(std::vector<VkDescriptorPoolSize>& poolSizes);
	void createDescriptorSetLayout(VkDescriptorSetLayout& DSL_model);
	void createDescriptorSets(VkDescriptorPool descriptorPool, VkDescriptorSetLayout& DSL_model, uint32_t index);
	void writeToAndUpdateDescriptorSets(uint32_t index);

	void recordDrawCmds(unsigned int frameIndex, const VkDescriptorSet& DS_camera,
		const VkPipeline& rasterP, const VkPipelineLayout& rasterPL, VkCommandBuffer& graphicsCmdBuffer);

private:
	bool LoadModel(const JSONItem::Model& jsonModel, VkQueue& graphicsQueue, VkCommandPool& commandPool);

public:
	Vertices m_vertices;
	Indices m_indices;
	std::vector<std::shared_ptr<Texture2D>> m_textures;
	std::vector<vkMaterial*> m_materials;
	std::vector<vkNode*> m_nodes;
	std::vector<vkNode*> m_linearNodes;
	
	// RayTracing
	glm::mat4 m_transform;
	VkGeometryNV m_rayTracingGeom;
	vAccelerationStructure m_bottomLevelAS;

private:
	VkDevice m_logicalDevice;
	VkPhysicalDevice m_physicalDevice;
	uint32_t m_numSwapChainImages;
	
	std::vector<bool> m_updateUniforms;
	uint32_t m_materialCount;
	uint32_t m_primitiveCount;

	bool m_areTexturesMipMapped;
	RENDER_TYPE m_renderType;
};