#pragma once
#include <global.h>
#include <Vulkan/Utilities/vPipelineUtil.h>
#include <Vulkan/Utilities/vBufferUtil.h>
#include <Vulkan/Utilities/vDescriptorUtil.h>
#include <Vulkan/Utilities/vShaderUtil.h>
#include <Vulkan/Utilities/vRenderUtil.h>
#include <Utilities/generalUtility.h>
#include <Vulkan/vulkanManager.h>

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

enum POST_PROCESS_GROUP { PASS_32BIT, PASS_TONEMAP, PASS_8BIT };

// This class will manage the pipelines created and used for rendering
// It should help abstract away that detail from the renderer class and prevent the renderpass stuff from being coupled with other things
class VulkanRendererBackend
{
public:
	VulkanRendererBackend() = delete;
	VulkanRendererBackend(VulkanManager* vulkanObject, int numSwapChainImages, VkExtent2D windowExtents);
	~VulkanRendererBackend();
	void cleanup();

	void createPipelines(DescriptorSetLayouts& pipelineDescriptorSetLayouts);
	void createRenderPassesAndFrameResources();
	void createAllPostProcessEffects();
	
	// Descriptor Sets
	void expandDescriptorPool(std::vector<VkDescriptorPoolSize>& poolSizes);
	void createDescriptorPool(std::vector<VkDescriptorPoolSize>& poolSizes);
	void createDescriptors(VkDescriptorPool descriptorPool);
	void writeToAndUpdateDescriptorSets(DescriptorSetDependencies& descSetDependencies);

	// Command Buffers
	void recreateCommandBuffers();
	void submitCommandBuffers();

	// Getters
	VkPipeline getPipeline(PIPELINE_TYPE type, int postProcessIndex = 0);
	VkPipelineLayout getPipelineLayout(PIPELINE_TYPE type, int postProcessIndex = 0);
	VkDescriptorSet getDescriptorSet(DSL_TYPE type, int frameIndex, int postProcessIndex = 0);
		VkDescriptorSetLayout getDescriptorSetLayout(DSL_TYPE type, int postProcessIndex = 0);
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
	RenderPassInfo m_toDisplayRPI; // Renders to actual swapChain Images -- Composite pass
	RenderPassInfo m_rasterRPI; // Renders to an offscreen framebuffer
	RenderPassInfo m_lastPostProcessRPI; // Renders to actual swapChain Images

private:
	void cleanupPipelines();
	void cleanupRenderPassesAndFrameResources();
	void cleanupPostProcess();

	// Render Passes
	void createRenderPasses();
	// Frame Buffer Attachments -- Used in conjunction with RenderPasses but not needed for their creation
	void createDepthResources();
	void createFrameBuffers();
	
	// Pipelines
	void createComputePipeline(VkPipeline& computePipeline, VkPipelineLayout computePipelineLayout, const std::string &pathToShader);
	void createRasterizationRenderPipeline(std::vector<VkDescriptorSetLayout>& rasterizationDSL);
	void createFinalCompositePipeline(std::vector<VkDescriptorSetLayout>&  compositeDSL);

	// Command Buffers
	void createCommandPoolsAndBuffers();
	void recordCommandBuffer_PostProcess(VkCommandBuffer& graphicsCmdBuffer, unsigned int frameIndex);

	// Post Process
	void expandDescriptorPool_PostProcess(std::vector<VkDescriptorPoolSize>& poolSizes);
	void createDescriptors_PostProcess(VkDescriptorPool descriptorPool);
	void createDescriptors_PostProcess(VkDescriptorPool descriptorPool, int& index, const std::string name,
		uint32_t bindingCount, VkDescriptorSetLayoutBinding* bindings);
	void writeToAndUpdateDescriptorSets_PostProcess();
	
	void prePostProcess();
	void createAllPostProcessSamplers();
	void addPostProcessPass(std::string effectName, std::vector<VkDescriptorSetLayout>& effectDSL, POST_PROCESS_GROUP postType);
	void addRenderPass_PostProcess(VkRenderPass& l_renderPass, const VkFormat colorFormat, const VkFormat depthFormat,
		const VkImageLayout finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	void addFrameBuffers_PostProcess(PostProcessRPI& passRPI, VkFormat colorFormat, POST_PROCESS_GROUP postType);
	void addPipeline_PostProcess(const std::string &shaderName, std::vector<VkDescriptorSetLayout>& l_postProcessDSL,
		VkRenderPass& l_renderPass, const uint32_t subpass = 0, VkExtent2D extents = { 0,0 });


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


public:
	// --- PostProcess ---
	// The last post process pass should write to the set of framebuffers in m_lastPostProcessRPI. 
	// This set can then be referenced by the UI pass easily.
	std::vector<VkDescriptorImageInfo> m_prePostProcessInput; // result of render passes that occur before post process work.

	std::vector<PostProcessDescriptors> m_postProcessDescriptors;
	std::vector<PostProcessRPI> m_32bitPasses;
	std::vector<PostProcessRPI> m_8bitPasses;
	PostProcessRPI m_toneMapRPI;


	// Should be able to use a single sampler for all the images as long as they are the same format and size
	VkSampler m_32bitSampler;
	VkSampler m_8bitSampler;
	VkSampler m_toneMapSampler;

	int m_numPostEffects;
	std::vector<std::string> m_postEffectNames;
	std::vector<VkPipeline> m_postProcess_Ps;
	std::vector<VkPipelineLayout> m_postProcess_PLs;
};

#pragma once
#include <Vulkan/RendererBackend/vRendererBackend_Commands.inl>
#include <Vulkan/RendererBackend/vRendererBackend_Pipelines.inl>
#include <Vulkan/RendererBackend/vRendererBackend_RenderPasses.inl>
#include <Vulkan/RendererBackend/vRendererBackend_PostProcess.inl>