#include "postProcessManager.h"

PostProcessManager::PostProcessManager(VulkanManager* vulkanObject, FrameBufferAttachment& depthAttachment,
	int numSwapChainImages, VkExtent2D windowExtents) :
	m_vulkanObj(vulkanObject), m_logicalDevice(vulkanObject->getLogicalDevice()), m_physicalDevice(vulkanObject->getPhysicalDevice()),
	m_depth(depthAttachment), m_numSwapChainImages(numSwapChainImages), m_windowExtents(windowExtents)
{
	createAllPostProcessEffects();
}
PostProcessManager::~PostProcessManager()
{
	for (int i = 0; i < m_descriptors.size(); i++)
	{
		vkDestroyDescriptorSetLayout(m_logicalDevice, m_descriptors[i].postProcess_DSL, nullptr);
	}
}

void PostProcessManager::cleanup()
{
	// Destroy Pipelines
	for (int i=0; i<m_postProcess_Ps.size(); i++)
	{
		vkDestroyPipeline(m_logicalDevice, m_postProcess_Ps[i], nullptr);
		vkDestroyPipelineLayout(m_logicalDevice, m_postProcess_PLs[i], nullptr);
	}

	// Destroy Samplers
	vkDestroySampler(m_logicalDevice, m_toneMapSampler, nullptr);
	vkDestroySampler(m_logicalDevice, m_32bitSampler, nullptr);
	vkDestroySampler(m_logicalDevice, m_8bitSampler, nullptr);

	// Destroy 32 bit passes
	for (int i = 0; i < m_32bitPasses.size(); i++)
	{
		for (uint32_t j = 0; j < m_numSwapChainImages; j++)
		{
			// Destroy Framebuffers
			vkDestroyFramebuffer(m_logicalDevice, m_32bitPasses[i].frameBuffers[j], nullptr);

			// Destroy the input attachment
			vkDestroyImage(m_logicalDevice, m_32bitPasses[i].inputImages[j].image, nullptr);
			vkDestroyImageView(m_logicalDevice, m_32bitPasses[i].inputImages[j].view, nullptr);
			vkFreeMemory(m_logicalDevice, m_32bitPasses[i].inputImages[j].memory, nullptr);
		}
		// Destroy Renderpasses
		vkDestroyRenderPass(m_logicalDevice, m_32bitPasses[i].renderPass, nullptr);
	}

	// Destroy 8 bit passes
	for (int i = 0; i < m_8bitPasses.size(); i++)
	{
		for (uint32_t j = 0; j < m_numSwapChainImages; j++)
		{
			// Destroy Framebuffers
			vkDestroyFramebuffer(m_logicalDevice, m_8bitPasses[i].frameBuffers[j], nullptr);

			// Destroy the input attachment
			vkDestroyImage(m_logicalDevice, m_8bitPasses[i].inputImages[j].image, nullptr);
			vkDestroyImageView(m_logicalDevice, m_8bitPasses[i].inputImages[j].view, nullptr);
			vkFreeMemory(m_logicalDevice, m_8bitPasses[i].inputImages[j].memory, nullptr);
		}
		// Destroy Renderpasses
		vkDestroyRenderPass(m_logicalDevice, m_8bitPasses[i].renderPass, nullptr);
	}

	// Destroy tone map pass
	{
		for (uint32_t j = 0; j < m_numSwapChainImages; j++)
		{
			// Destroy Framebuffers
			vkDestroyFramebuffer(m_logicalDevice, m_toneMapRPI.frameBuffers[j], nullptr);

			// Destroy the input attachment
			vkDestroyImage(m_logicalDevice, m_toneMapRPI.inputImages[j].image, nullptr);
			vkDestroyImageView(m_logicalDevice, m_toneMapRPI.inputImages[j].view, nullptr);
			vkFreeMemory(m_logicalDevice, m_toneMapRPI.inputImages[j].memory, nullptr);
		}
		// Destroy Renderpasses
		vkDestroyRenderPass(m_logicalDevice, m_toneMapRPI.renderPass, nullptr);
	}
}
void PostProcessManager::recreate()
{
}

void PostProcessManager::createAllPostProcessEffects()
{
	createAllSamplers();

	// Add 32 bit passes
	//addPostProcessPass("32bit", std::vector<VkDescriptorSetLayout>& effectDSL, POST_PROCESS_GROUP::PASS_32BIT);

	// Tone Map Pass
	{

		
		std::vector<VkDescriptorSet> effectDS;
		std::vector<VkDescriptorSetLayout> effectDSL;
		addPostProcessPass("Tonemap", effectDSL, POST_PROCESS_GROUP::PASS_TONEMAP);
	}

	// Add 8 bit passes
	//addPostProcessPass("8bit", std::vector<VkDescriptorSetLayout>& effectDSL, POST_PROCESS_GROUP::PASS_8BIT);

	// do any sort of transition work to the last image rendered for it to be accepted by the finalComposite renderpass
}
void PostProcessManager::addPostProcessPass(std::string effectName, std::vector<VkDescriptorSetLayout>& effectDSL,
	POST_PROCESS_GROUP postType)
{
	m_postEffectNames.push_back(effectName);

	if(postType == POST_PROCESS_GROUP::PASS_32BIT)
	{
		const VkFormat colorFormat = VK_FORMAT_R16G16B16A16_SFLOAT; // Technically not 32 bit but just higher resolution
		m_32bitPasses[m_numPostEffects].serialIndex = m_numPostEffects;
		addRenderPass(m_32bitPasses[m_numPostEffects].renderPass, colorFormat);
		addFrameBuffers(m_32bitPasses[m_numPostEffects], colorFormat, POST_PROCESS_GROUP::PASS_32BIT);
		addPipeline(effectName, effectDSL, m_32bitPasses[m_numPostEffects].renderPass);
	}
	else if (postType == POST_PROCESS_GROUP::PASS_8BIT)
	{
		const VkFormat colorFormat = VK_FORMAT_B8G8R8A8_UNORM;
		m_8bitPasses[m_numPostEffects].serialIndex = m_numPostEffects;
		addRenderPass(m_32bitPasses[m_numPostEffects].renderPass, colorFormat);
		addFrameBuffers(m_32bitPasses[m_numPostEffects], colorFormat, POST_PROCESS_GROUP::PASS_32BIT);
		addPipeline(effectName, effectDSL, m_32bitPasses[m_numPostEffects].renderPass);
	}
	else if (postType == POST_PROCESS_GROUP::PASS_TONEMAP)
	{
		//const VkFormat colorFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
		//m_toneMapRPI.serialIndex = m_numPostEffects;
		//addRenderPass(m_toneMapRPI.renderPass, colorFormat);
		//addFrameBuffers(m_toneMapRPI, colorFormat, POST_PROCESS_GROUP::PASS_TONEMAP);
		//addPipeline(effectName, effectDSL, m_toneMapRPIrenderPass);
	}

	m_numPostEffects++;
}


void PostProcessManager::recordCommandBuffer(VkCommandBuffer& graphicsCmdBuffer, unsigned int frameIndex)
{

}

// Descriptor Sets
void PostProcessManager::expandDescriptorPool(std::vector<VkDescriptorPoolSize>& poolSizes)
{
	// Tonemap
	poolSizes.push_back({ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, m_numSwapChainImages }); // input image
	poolSizes.push_back({ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, m_numSwapChainImages }); // output image
}
void PostProcessManager::createDescriptors(VkDescriptorPool descriptorPool)
{
	int serialCount = 0;
	// Tonemap
	VkDescriptorSetLayoutBinding inputImageLayoutBinding = { 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr };
	VkDescriptorSetLayoutBinding outputImageBinding = { 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr };
	std::array<VkDescriptorSetLayoutBinding, 2> toneMapBindings = { inputImageLayoutBinding, outputImageBinding };
	createDescriptors(descriptorPool, serialCount, "Tonemap", 2, toneMapBindings.data());
}
void PostProcessManager::createDescriptors(VkDescriptorPool descriptorPool, int& index, const std::string name,
	uint32_t bindingCount, VkDescriptorSetLayoutBinding* bindings)
{
	m_descriptors.resize(index + 1);
	m_descriptors[index].postProcess_DSs.resize(m_numSwapChainImages);
	m_descriptors[index].serialIndex = index;
	m_descriptors[index].descriptorName = name;
	DescriptorUtil::createDescriptorSetLayout(m_logicalDevice, 
		m_descriptors[index].postProcess_DSL, bindingCount, bindings);
	
	for (uint32_t i = 0; i < m_numSwapChainImages; i++)
	{
		DescriptorUtil::createDescriptorSets(m_logicalDevice, descriptorPool, 1,
			&m_descriptors[index].postProcess_DSL, &m_descriptors[index].postProcess_DSs[i]);
	}

	index++;
}
void PostProcessManager::writeToAndUpdateDescriptorSets()
{

}


void PostProcessManager::addRenderPass(VkRenderPass& l_renderPass, const VkFormat colorFormat, 
	const VkImageLayout finalLayout)
{
	std::vector<VkSubpassDependency> subpassDependencies;
	{
		// Transition from tasks before this render pass (including runs through other pipelines before it, hence bottom of pipe)
		subpassDependencies.push_back(
			RenderPassUtil::subpassDependency(VK_SUBPASS_EXTERNAL, 0,
				VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_ACCESS_MEMORY_READ_BIT,
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT));

		//Transition from actual subpass to tasks after the renderpass
		subpassDependencies.push_back(
			RenderPassUtil::subpassDependency(0, VK_SUBPASS_EXTERNAL,
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
				VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_ACCESS_MEMORY_READ_BIT));
	}
	RenderPassUtil::renderPassCreationHelper(m_logicalDevice, l_renderPass,
		colorFormat, m_depth.format, finalLayout, subpassDependencies);
}
void PostProcessManager::addFrameBuffers(PostProcessRPI& passRPI, VkFormat colorFormat, POST_PROCESS_GROUP postType)
{
	passRPI.frameBuffers.resize(m_numSwapChainImages);
	passRPI.inputImages.resize(m_numSwapChainImages);
	passRPI.imageSetInfo.resize(m_numSwapChainImages);

	FrameResourcesUtil::createFrameBufferAttachments(m_logicalDevice, m_physicalDevice,
		m_numSwapChainImages, passRPI.inputImages, colorFormat, m_windowExtents);

	for (uint32_t i = 0; i < m_numSwapChainImages; i++)
	{
		std::array<VkImageView, 2> attachments = { passRPI.inputImages[i].view, m_depth.view };

		FrameResourcesUtil::createFrameBuffer(m_logicalDevice, passRPI.frameBuffers[i], passRPI.renderPass,
			m_windowExtents, static_cast<uint32_t>(attachments.size()), attachments.data());

		// Fill a descriptor for later use in a descriptor set 
		passRPI.imageSetInfo[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		passRPI.imageSetInfo[i].imageView = passRPI.inputImages[i].view;
		if (postType == PASS_32BIT)
		{
			passRPI.imageSetInfo[i].sampler = m_32bitSampler;
		}
		else if (postType == PASS_TONEMAP)
		{
			passRPI.imageSetInfo[i].sampler = m_toneMapSampler;
		}
		else if (postType == PASS_8BIT)
		{
			passRPI.imageSetInfo[i].sampler = m_8bitSampler;
		}
	}
}
void PostProcessManager::createAllSamplers()
{
	const float mipLevels = 1;
	const uint32_t arrayLayers = 1;
	const float anisotropy = 16.0f;
	const VkSamplerAddressMode samplerAddressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

	// 32 Bit Sampler
	ImageUtil::createImageSampler(m_logicalDevice, m_32bitSampler, VK_FILTER_LINEAR, VK_FILTER_LINEAR, samplerAddressMode,
		VK_SAMPLER_MIPMAP_MODE_LINEAR, 0, 0, mipLevels, anisotropy, VK_COMPARE_OP_NEVER);
	// 8 Bit Sampler
	ImageUtil::createImageSampler(m_logicalDevice, m_8bitSampler, VK_FILTER_LINEAR, VK_FILTER_LINEAR, samplerAddressMode,
		VK_SAMPLER_MIPMAP_MODE_LINEAR, 0, 0, mipLevels, anisotropy, VK_COMPARE_OP_NEVER);
	// Tonemap Sampler
	ImageUtil::createImageSampler(m_logicalDevice, m_toneMapSampler, VK_FILTER_LINEAR, VK_FILTER_LINEAR, samplerAddressMode,
		VK_SAMPLER_MIPMAP_MODE_LINEAR, 0, 0, mipLevels, anisotropy, VK_COMPARE_OP_NEVER);
}
void PostProcessManager::addPipeline(const std::string &shaderName,	std::vector<VkDescriptorSetLayout>& l_postProcessDSL,
	VkRenderPass& l_renderPass, const uint32_t subpass, VkExtent2D extents)
{
	if (extents.width <= 0 || extents.height <= 0)
	{
		extents = m_windowExtents;
	}

	// -------- Create Shader Stages -------------
	const uint32_t stageCount = 2;
	VkShaderModule vertShaderModule, fragShaderModule;
	std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
	shaderStages.resize(stageCount);

	ShaderUtil::createShaderStageInfos_RenderToQuad(shaderStages, shaderName, vertShaderModule, fragShaderModule, m_logicalDevice);

	// -------- Create Pipeline Layout -------------
	size_t lastIndex = m_postProcess_PLs.size();
	m_postProcess_PLs.push_back(
		VulkanPipelineCreation::createPipelineLayout(m_logicalDevice, l_postProcessDSL, 0, nullptr));

	// -------- Create Post Process pipeline ---------	
	m_postProcess_Ps.push_back(
		VulkanPipelineCreation::createPostProcessPipeline(m_logicalDevice,
			m_postProcess_PLs[lastIndex], l_renderPass, subpass,
			stageCount, shaderStages.data(), extents));

	// No need for the shader modules anymore, so we destory them!
	vkDestroyShaderModule(m_logicalDevice, vertShaderModule, nullptr);
	vkDestroyShaderModule(m_logicalDevice, fragShaderModule, nullptr);
}