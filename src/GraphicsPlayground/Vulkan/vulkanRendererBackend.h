#pragma once
#include <global.h>
#include <Utilities/pipelineUtility.h>
#include <Utilities/shaderUtility.h>
#include <Utilities/bufferUtility.h>
#include <Utilities/renderUtility.h>
#include <Utilities/descriptorUtility.h>
#include <Utilities/generalUtility.h>
#include "Vulkan/vulkanManager.h"

#include "SceneElements/texture.h"

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

struct DescriptorSetDependencies
{
	std::vector<Texture*> computeImages;
	std::vector<VkDescriptorImageInfo> geomRenderPassImageSet;
};

// This class will manage the pipelines created and used for rendering
// It should help abstract away that detail from the renderer class and prevent the renderpass stuff from being coupled with other things
class VulkanRendererBackend
{
public:
	VulkanRendererBackend() = delete;
	VulkanRendererBackend(VulkanManager* vulkanObject, int numSwapChainImages, VkExtent2D windowExtents);
	~VulkanRendererBackend();
	
	void createPipelines(DescriptorSetLayouts& pipelineDescriptorSetLayouts);
	void createRenderPassesAndFrameResources();
	void cleanup();
	
	// Descriptor Sets
	void expandDescriptorPool(std::vector<VkDescriptorPoolSize>& poolSizes);
	void createDescriptorPool(std::vector<VkDescriptorPoolSize>& poolSizes);
	void createDescriptors(VkDescriptorPool descriptorPool);
	void writeToAndUpdateDescriptorSets(DescriptorSetDependencies& descSetDependencies);

	// Command Buffers
	void recreateCommandBuffers();
	void submitCommandBuffers();

	// Getters
	VkPipeline getPipeline(PIPELINE_TYPE type);
	VkPipelineLayout getPipelineLayout(PIPELINE_TYPE type);
	VkDescriptorSet getDescriptorSet(DSL_TYPE type, int index);
	VkDescriptorSetLayout getDescriptorSetLayout(DSL_TYPE key);
	const VkDescriptorPool getDescriptorPool() const { return m_descriptorPool; }
	const VkCommandBuffer getComputeCommandBuffer(uint32_t index) const { return m_computeCommandBuffers[index]; }
	const VkCommandBuffer getGraphicsCommandBuffer(uint32_t index) const { return m_graphicsCommandBuffers[index]; }
	const VkCommandPool getComputeCommandPool() const { return m_computeCommandPool; }
	const VkCommandPool getGraphicsCommandPool() const { return m_graphicsCommandPool; }

	// Setters
	void setWindowExtents(VkExtent2D windowExtent) { m_windowExtents = windowExtent; }

public:
	// --- Render Passes --- 
	// RPI stands for Render Pass Info
	RenderPassInfo m_toDisplayRPI; //renders to actual swapChain Images -- Composite pass
	RenderPassInfo m_rasterRPI; // Renders to an offscreen framebuffer

private:
	void cleanupPipelines();
	void cleanupRenderPassesAndFrameResources();

	// Render Passes
	void createRenderPasses();
	void createPostProcessRenderPasses();
	// Frame Buffer Attachments -- Used in conjunction with RenderPasses but not needed for their creation
	void createDepthResources();
	void createFrameBuffers();
	
	// Pipelines
	void createComputePipeline(VkPipeline& computePipeline, VkPipelineLayout computePipelineLayout, const std::string &pathToShader);
	void createRasterizationRenderPipeline(std::vector<VkDescriptorSetLayout>& rasterizationDSL);
	void createFinalCompositePipeline(std::vector<VkDescriptorSetLayout>&  compositeDSL);

	// Command Buffers
	void createCommandPoolsAndBuffers();

private:
	VulkanManager* m_vulkanObj;
	VkDevice m_logicalDevice;
	VkPhysicalDevice m_physicalDevice;
	uint32_t m_numSwapChainImages;
	VkExtent2D m_windowExtents;


	VkDescriptorPool m_descriptorPool;
	// --- Descriptor Sets ---
	VkDescriptorSetLayout m_DSL_finalComposite;
	std::vector<VkDescriptorSet> m_DS_finalComposite;
	VkDescriptorSetLayout m_DSL_compute;
	std::vector<VkDescriptorSet> m_DS_compute;
	
	// --- Pipelines ---
	// Pipelines -- P
	VkPipeline m_rasterization_P;
	VkPipeline m_finalComposite_P;
	VkPipeline m_compute_P;
	// Pipeline Layouts -- PLs
	VkPipelineLayout m_rasterization_PL;
	VkPipelineLayout m_finalComposite_PL;
	VkPipelineLayout m_compute_PL;	

	// --- Frame Buffer Attachments --- 
	// Depth is going to be common to the scene across render passes as well
	VkFormat m_depthFormat;
	FrameBufferAttachment m_depth;
	
	// --- Command Buffers and Memory Pools --- 
	VkCommandPool m_graphicsCommandPool;
	VkCommandPool m_computeCommandPool;
	std::vector<VkCommandBuffer> m_graphicsCommandBuffers;
	std::vector<VkCommandBuffer> m_computeCommandBuffers;
	
	// --- Queues --- 
	VkQueue m_graphicsQueue;
	VkQueue m_computeQueue;
};