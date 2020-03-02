#pragma once
#include <string>
#include <Vulkan\RendererBackend\vRendererBackend.h>

inline void VulkanRendererBackend::cleanupPostProcess()
{
	// Destroy Pipelines
	for (int i = 0; i<m_postProcess_Ps.size(); i++)
	{
		vkDestroyPipeline(m_logicalDevice, m_postProcess_Ps[i], nullptr);
		vkDestroyPipelineLayout(m_logicalDevice, m_postProcess_PLs[i], nullptr);
	}
	m_numPostEffects = 0;

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
inline void VulkanRendererBackend::recordCommandBuffer_PostProcess(VkCommandBuffer& graphicsCmdBuffer, unsigned int frameIndex)
{

}
inline void VulkanRendererBackend::prePostProcess()
{
	m_numPostEffects = 0;
	createAllPostProcessSamplers();

	m_prePostProcessInput.resize(m_numSwapChainImages);
	for (uint32_t i = 0; i < m_numSwapChainImages; i++)
	{
		m_prePostProcessInput[i].imageLayout = m_toDisplayRPI.imageSetInfo[i].imageLayout;
		m_prePostProcessInput[i].imageView = m_toDisplayRPI.imageSetInfo[i].imageView;
		m_prePostProcessInput[i].sampler = m_32bitSampler;
	}

	// TODO Setup last post process pass as the transition pass to the UI. 
	// It basically takes whatever was the last result and copies it into the swapchain. 
}



inline void VulkanRendererBackend::expandDescriptorPool_PostProcess(std::vector<VkDescriptorPoolSize>& poolSizes)
{
	// Tonemap
	poolSizes.push_back({ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, m_numSwapChainImages }); // input image
	poolSizes.push_back({ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, m_numSwapChainImages }); // output image
}
inline void VulkanRendererBackend::createDescriptors_PostProcess(VkDescriptorPool descriptorPool)
{
	int serialCount = 0;

	// Tonemap
	VkDescriptorSetLayoutBinding inputImageLayoutBinding = { 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr };
	VkDescriptorSetLayoutBinding outputImageBinding = { 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr };
	std::array<VkDescriptorSetLayoutBinding, 2> toneMapBindings = { inputImageLayoutBinding, outputImageBinding };
	createDescriptors_PostProcess(descriptorPool, serialCount, "Tonemap", 2, toneMapBindings.data());

	// TXAA and other Post Process effects
}
inline void VulkanRendererBackend::createDescriptors_PostProcess(VkDescriptorPool descriptorPool, int& index, const std::string name,
	uint32_t bindingCount, VkDescriptorSetLayoutBinding* bindings)
{
	m_postProcessDescriptors.resize(index + 1);
	m_postProcessDescriptors[index].postProcess_DSs.resize(m_numSwapChainImages);
	m_postProcessDescriptors[index].serialIndex = index;
	m_postProcessDescriptors[index].descriptorName = name;
	DescriptorUtil::createDescriptorSetLayout(m_logicalDevice,
		m_postProcessDescriptors[index].postProcess_DSL, bindingCount, bindings);

	for (uint32_t i = 0; i < m_numSwapChainImages; i++)
	{
		DescriptorUtil::createDescriptorSets(m_logicalDevice, descriptorPool, 1,
			&m_postProcessDescriptors[index].postProcess_DSL, &m_postProcessDescriptors[index].postProcess_DSs[i]);
	}

	index++;
}
inline void VulkanRendererBackend::writeToAndUpdateDescriptorSets_PostProcess()
{
	for (uint32_t i = 0; i < m_numSwapChainImages; i++)
	{
		// Tone Map
		// TODO: change to something that searches for the particular string name later
		int index = 0; // controls which descriptor is being accessed
		{
			// The input image for the tonemap is the last renderpass in the 32 bit passes.
			// TODO: The inputImages data is probably not needed and can be removed at a later point
			// We should simply be able to use the data from the last renderpass if things are in the correct order.
			// We are already doing this with the image layout.
			VkDescriptorImageInfo inputImageInfo = DescriptorUtil::createDescriptorImageInfo(
				m_32bitSampler,
				m_toneMapRPI.inputImages[i].view,
				m_prePostProcessInput[i].imageLayout);// m_32bitPasses.back().imageSetInfo[i].imageLayout);
			VkDescriptorImageInfo outputImageInfo = DescriptorUtil::createDescriptorImageInfo(
				m_toneMapSampler,
				m_toneMapRPI.imageSetInfo[i].imageView,
				m_toneMapRPI.imageSetInfo[i].imageLayout);

			const int descriptorCount = 2;
			std::array<VkWriteDescriptorSet, descriptorCount> writeToneMapSetInfo = {};
			writeToneMapSetInfo[0] = DescriptorUtil::writeDescriptorSet(
				m_postProcessDescriptors[index].postProcess_DSs[i], 0, 1, 
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &inputImageInfo);
			writeToneMapSetInfo[1] = DescriptorUtil::writeDescriptorSet(
				m_postProcessDescriptors[index].postProcess_DSs[i], 1, 1, 
				VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, &outputImageInfo);

			vkUpdateDescriptorSets(m_logicalDevice, descriptorCount, writeToneMapSetInfo.data(), 0, nullptr);
		}

		// Another Postprocess pass
		index++;
	}
}



inline void VulkanRendererBackend::addPostProcessPass(std::string effectName,
	std::vector<VkDescriptorSetLayout>& effectDSL, POST_PROCESS_GROUP postType)
{
	m_postEffectNames.push_back(effectName);
	m_postProcess_Ps.resize(m_numPostEffects + 1);
	m_postProcess_PLs.resize(m_numPostEffects + 1);

	if (postType == POST_PROCESS_GROUP::PASS_32BIT)
	{
		const VkFormat colorFormat = VK_FORMAT_R16G16B16A16_SFLOAT; // Technically not 32 bit but just higher resolution
		m_32bitPasses[m_numPostEffects].serialIndex = m_numPostEffects;
		addRenderPass_PostProcess(m_32bitPasses[m_numPostEffects].renderPass, colorFormat, m_depth.format);
		addFrameBuffers_PostProcess(m_32bitPasses[m_numPostEffects], colorFormat, POST_PROCESS_GROUP::PASS_32BIT);
		addPipeline_PostProcess(effectName, effectDSL, m_32bitPasses[m_numPostEffects].renderPass);
	}
	else if (postType == POST_PROCESS_GROUP::PASS_8BIT)
	{
		const VkFormat colorFormat = VK_FORMAT_B8G8R8A8_UNORM;
		m_8bitPasses[m_numPostEffects].serialIndex = m_numPostEffects;
		addRenderPass_PostProcess(m_32bitPasses[m_numPostEffects].renderPass, colorFormat, m_depth.format);
		addFrameBuffers_PostProcess(m_32bitPasses[m_numPostEffects], colorFormat, POST_PROCESS_GROUP::PASS_8BIT);
		addPipeline_PostProcess(effectName, effectDSL, m_32bitPasses[m_numPostEffects].renderPass);
	}
	else if (postType == POST_PROCESS_GROUP::PASS_TONEMAP)
	{
		const VkFormat startColorFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
		const VkFormat endColorFormat = VK_FORMAT_B8G8R8A8_UNORM;
		m_toneMapRPI.serialIndex = m_numPostEffects;

		// undefined depth format means we dont add depth as a attachment to the renderpass
		addRenderPass_PostProcess(m_toneMapRPI.renderPass, endColorFormat, VK_FORMAT_UNDEFINED);
		addFrameBuffers_PostProcess(m_toneMapRPI, endColorFormat, POST_PROCESS_GROUP::PASS_TONEMAP);
		addPipeline_PostProcess(effectName, effectDSL, m_toneMapRPI.renderPass);
	}

	m_numPostEffects++;
}

inline void VulkanRendererBackend::addRenderPass_PostProcess( VkRenderPass& l_renderPass, 
	const VkFormat colorFormat, const VkFormat depthFormat,	const VkImageLayout finalLayout)
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
		colorFormat, depthFormat, finalLayout, subpassDependencies);
}

inline void VulkanRendererBackend::addFrameBuffers_PostProcess(
	PostProcessRPI& passRPI, VkFormat colorFormat, POST_PROCESS_GROUP postType)
{
	passRPI.frameBuffers.resize(m_numSwapChainImages);
	passRPI.inputImages.resize(m_numSwapChainImages);
	passRPI.imageSetInfo.resize(m_numSwapChainImages);

	FrameResourcesUtil::createFrameBufferAttachments(m_logicalDevice, m_physicalDevice,
		m_numSwapChainImages, passRPI.inputImages, colorFormat, m_windowExtents);

	for (uint32_t i = 0; i < m_numSwapChainImages; i++)
	{
		std::array<VkImageView, 1> attachments = { passRPI.inputImages[i].view };// , m_depth.view

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

inline void VulkanRendererBackend::addPipeline_PostProcess(
	const std::string &shaderName, std::vector<VkDescriptorSetLayout>& l_postProcessDSL,
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
	const size_t lastIndex = m_numPostEffects;
	m_postProcess_PLs[lastIndex] = VulkanPipelineCreation::createPipelineLayout(m_logicalDevice, l_postProcessDSL, 0, nullptr);

	// -------- Create Post Process pipeline ---------
	VulkanPipelineCreation::createPostProcessPipeline(m_logicalDevice,
		m_postProcess_Ps[lastIndex], m_postProcess_PLs[lastIndex], l_renderPass, subpass,
		stageCount, shaderStages.data(), extents);

	// No need for the shader modules anymore, so we destory them!
	vkDestroyShaderModule(m_logicalDevice, vertShaderModule, nullptr);
	vkDestroyShaderModule(m_logicalDevice, fragShaderModule, nullptr);
}

inline void VulkanRendererBackend::createAllPostProcessSamplers()
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