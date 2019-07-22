#pragma once

#include <global.h>
#include <Utilities/pipelineUtility.h>
#include <Utilities/shaderUtility.h>

#include "VulkanSetup/vulkanDevices.h"
#include "VulkanSetup/vulkanPresentation.h"

#include "renderPassManager.h"
#include "SceneElements\texture.h"


#include <Utilities/bufferUtility.h>
#include <Utilities/renderUtility.h>
#include <Utilities/generalUtility.h>

struct DescriptorSetLayouts
{
	std::vector<VkDescriptorSetLayout> computeDSL;
	std::vector<VkDescriptorSetLayout> finalCompositeDSL;
	std::vector<VkDescriptorSetLayout> geomDSL;
};

struct ComputePipelineLayouts
{
	VkPipelineLayout sky;
	VkPipelineLayout clouds;
	VkPipelineLayout grass;	
	VkPipelineLayout water;
};

struct PostProcessPipelineLayouts
{
	VkPipelineLayout pp1;
};

struct DescriptorSetDependencies
{
	std::vector<Texture*> computeImages;
	std::vector<VkDescriptorImageInfo> geomRenderPassImageSet;
};

enum class PIPELINE_TYPE { COMMON, POST_PROCESS };

// This class will manage the pipelines created and used for rendering
// It should help abstract away that detail from the renderer class and prevent the renderpass stuff from being coupled with other things
class PipelineManager
{
public:
	PipelineManager() = delete;
	PipelineManager(VulkanDevices* devices, VulkanPresentation* presentationObject, 
		RenderPassManager* renderPassManager, int numSwapChainImages, VkExtent2D windowExtents);
	~PipelineManager();
	void cleanup();
	void recreate(VkExtent2D windowExtents, DescriptorSetLayouts& pipelineDescriptorSetLayouts);

	// Descriptor Sets
	void expandDescriptorPool(std::vector<VkDescriptorPoolSize>& poolSizes);
	void createDescriptorSetLayouts();
	void createAndWriteDescriptorSets(VkDescriptorPool descriptorPool, DescriptorSetDependencies& descSetDependencies);

	VkDescriptorSet getDescriptorSet(DSL_TYPE type, int index);
	VkDescriptorSetLayout getDescriptorSetLayout(DSL_TYPE key);

private:
	void createComputePipeline(VkPipeline& computePipeline, VkPipelineLayout computePipelineLayout, const std::string &pathToShader);
	void createRasterizationRenderPipeline(std::vector<VkDescriptorSetLayout>& rasterizationDSL);
	void createFinalCompositePipeline(std::vector<VkDescriptorSetLayout>&  compositeDSL);
	void createPostProcessPipelines(VkRenderPass& renderPass, unsigned int subpass);
	
	void createPipelineHelper(VkPipeline& pipeline, VkPipelineLayout& pipelineLayout, VkRenderPass& renderPass, uint32_t subpass,
		VkPipelineVertexInputStateCreateInfo& vertexInput, uint32_t stageCount, const VkPipelineShaderStageCreateInfo* stages);

private:
	VulkanDevices* m_devices;
	VkDevice m_logicalDevice;
	VkPhysicalDevice m_pDevice;
	VulkanPresentation* m_presentationObj;
	RenderPassManager* m_renderPassManager;
	uint32_t m_numSwapChainImages;
	VkExtent2D m_windowExtents;

public:
	// Pipeline Layouts -- PLs
	VkPipelineLayout m_compute_PL;
	VkPipelineLayout m_finalComposite_PL;
	VkPipelineLayout m_rasterization_PL;

	// Pipelines -- P
	VkPipeline m_compute_P;
	VkPipeline m_finalComposite_P;
	VkPipeline m_rasterization_P;

	//// ----- Post Process ----- 
	//VkPipelineLayout m_postProcess_ToneMap_PipelineLayout;
	//VkPipelineLayout m_postProcess_TXAA_PipelineLayout;
	//VkPipeline m_postProcess_ToneMap_Pipeline;
	//VkPipeline m_postProcess_TXAA_Pipeline;

private:
	VkDescriptorSetLayout m_DSL_finalComposite;
	std::vector<VkDescriptorSet> m_DS_finalComposite;
	VkDescriptorSetLayout m_DSL_compute;
	std::vector<VkDescriptorSet> m_DS_compute;
};