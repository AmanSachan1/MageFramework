#pragma once
#include <string>

#include <global.h>
#include <Utilities/bufferUtility.h>
#include <Utilities/shaderUtility.h>
#include <Utilities/imageUtility.h>
#include <Utilities/renderUtility.h>

#include "Vulkan/vulkanManager.h"
#include "Vulkan/vulkanRendererBackend.h"

enum POST_PROCESS_GROUP { PASS_32BIT, PASS_TONEMAP, PASS_8BIT };

class PostProcessManager 
{
public:
	PostProcessManager() = delete;
	PostProcessManager(VulkanManager* vulkanObject, FrameBufferAttachment& depthAttachment,
		int numSwapChainImages, VkExtent2D windowExtents);
	~PostProcessManager();

	void cleanup();
	void recreate();
	void createAllPostProcessEffects();
	void recordCommandBuffer(VkCommandBuffer& graphicsCmdBuffer, unsigned int frameIndex);

	// Descriptor Sets
	void expandDescriptorPool(std::vector<VkDescriptorPoolSize>& poolSizes);
	void createDescriptors(VkDescriptorPool descriptorPool);
	void createDescriptors(VkDescriptorPool descriptorPool, int& index, const std::string name,
		uint32_t bindingCount, VkDescriptorSetLayoutBinding* bindings);
	void writeToAndUpdateDescriptorSets();


private:
	void addPostProcessPass(std::string effectName,	std::vector<VkDescriptorSetLayout>& effectDSL, 
		POST_PROCESS_GROUP postType);

	void createAllSamplers();
	void addRenderPass(VkRenderPass& l_renderPass, const VkFormat colorFormat,
		const VkImageLayout finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	void addFrameBuffers(PostProcessRPI& passRPI, VkFormat colorFormat, POST_PROCESS_GROUP postType);
	void addPipeline(const std::string &shaderName,	std::vector<VkDescriptorSetLayout>& l_postProcessDSL,
		VkRenderPass& l_renderPass, const uint32_t subpass = 0, VkExtent2D extents = { 0,0 });
	
private:
	VulkanManager* m_vulkanObj;
	VkDevice m_logicalDevice;
	VkPhysicalDevice m_physicalDevice;
	uint32_t m_numSwapChainImages;
	VkExtent2D m_windowExtents;
	
	FrameBufferAttachment m_depth;
	std::vector<VkFramebuffer> finalFrameBuffers; // last post process pass should write to this set of framebuffers
	std::vector<FrameBufferAttachment> prePostProcessInput; // result of render passes that occur before post process work.

	std::vector<PostProcessDescriptors> m_descriptors;
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