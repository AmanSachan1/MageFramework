#pragma once
#include <string>
#include <Vulkan\RendererBackend\vRendererBackend.h>

inline void VulkanRendererBackend::cleanupPostProcess()
{
	// Destroy Pipelines
	for (unsigned int i = 0; i<m_numPostEffects; i++)
	{
		vkDestroyPipeline(m_logicalDevice, m_postProcess_Ps[i], nullptr);
		vkDestroyPipelineLayout(m_logicalDevice, m_postProcess_PLs[i], nullptr);
	}
	m_postEffectNames.clear();
	m_postProcess_Ps.clear();
	m_postProcess_PLs.clear();
	m_numPostEffects = 0;
	
	// Destroy Samplers
	vkDestroySampler(m_logicalDevice, m_postProcessSampler, nullptr);

	//Destroy the common frame buffer attachments
	for (unsigned int j = 0; j < 2; j++)
	{
		for (uint32_t i = 0; i < m_numSwapChainImages; i++)
		{
			// Destroy the frame buffer attachment
			vkDestroyImage(m_logicalDevice, m_fbaHighRes[j][i].image, nullptr);
			vkDestroyImageView(m_logicalDevice, m_fbaHighRes[j][i].view, nullptr);
			vkFreeMemory(m_logicalDevice, m_fbaHighRes[j][i].memory, nullptr);

			vkDestroyImage(m_logicalDevice, m_fbaLowRes[j][i].image, nullptr);
			vkDestroyImageView(m_logicalDevice, m_fbaLowRes[j][i].view, nullptr);
			vkFreeMemory(m_logicalDevice, m_fbaLowRes[j][i].memory, nullptr);
		}
		m_fbaHighRes[j].clear();
		m_fbaLowRes[j].clear();
	}

	// Destroy all post process passes
	for (unsigned int j = 0; j < m_postProcessRPIs.size(); j++)
	{
		for (uint32_t i = 0; i < m_numSwapChainImages; i++)
		{
			// Destroy Framebuffers
			vkDestroyFramebuffer(m_logicalDevice, m_postProcessRPIs[j].frameBuffers[i], nullptr);
		}
		// Destroy Renderpasses
		vkDestroyRenderPass(m_logicalDevice, m_postProcessRPIs[j].renderPass, nullptr);
	}
	m_postProcessRPIs.clear();
}

inline void VulkanRendererBackend::prePostProcess()
{
	m_numPostEffects = 0;
	m_fbaHighResIndexInUse = 0;
	m_fbaLowResIndexInUse = 0;

	// Create the post Process sampler
	{
		const float mipLevels = 1;
		const uint32_t arrayLayers = 1;
		const float anisotropy = 16.0f;

		ImageUtil::createImageSampler(m_logicalDevice, m_postProcessSampler, 
			VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, 
			VK_SAMPLER_MIPMAP_MODE_LINEAR, 0, 0, mipLevels, anisotropy, VK_COMPARE_OP_NEVER);
	}

	// Store info used to fill out descriptors sets
	m_prePostProcessInput.resize(m_numSwapChainImages);
	for (uint32_t i = 0; i < m_numSwapChainImages; i++)
	{
		m_prePostProcessInput[i].imageLayout = m_compositeComputeOntoRasterRPI.imageSetInfo[i].imageLayout;
		m_prePostProcessInput[i].imageView = m_compositeComputeOntoRasterRPI.imageSetInfo[i].imageView;
		m_prePostProcessInput[i].sampler = m_postProcessSampler;
	}

	// Transition swapchain images to VK_IMAGE_LAYOUT_PRESENT_SRC_KHR for consistency during later transitions
	for (uint32_t index = 0; index < m_numSwapChainImages; index++)
	{
		VkImageLayout oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		VkImageLayout newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;// VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; //VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
		m_vulkanManager->transitionSwapChainImageLayout_SingleTimeCommand(index, oldLayout, newLayout, m_graphicsCommandPool);
	}

	// Create the framebuffer attachments used by more than one post process pass
	{
		// Ping pong post process framebuffers that will be alternatively read from and written into.
		// Need 2 sets for the 2 types of resolutions we support
		const VkImageLayout layoutBeforeImageCreation = VK_IMAGE_LAYOUT_UNDEFINED;
		const VkImageLayout layoutToTransitionImageToAfterCreation = VK_IMAGE_LAYOUT_GENERAL; // No transition if same as layoutBeforeImageCreation
		const VkImageUsageFlags frameBufferUsage = 
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | 
			VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

		for (uint32_t j = 0; j < 2; j++)
		{
			m_fbaHighRes[j].resize(m_numSwapChainImages);
			m_fbaLowRes[j].resize(m_numSwapChainImages);

			FrameResourcesUtil::createFrameBufferAttachments(m_logicalDevice, m_physicalDevice, m_graphicsQueue, m_graphicsCommandPool,
				m_numSwapChainImages, m_fbaHighRes[j], m_highResolutionRenderFormat,
				layoutBeforeImageCreation, layoutToTransitionImageToAfterCreation, m_windowExtents, frameBufferUsage);

			FrameResourcesUtil::createFrameBufferAttachments(m_logicalDevice, m_physicalDevice, m_graphicsQueue, m_graphicsCommandPool,
				m_numSwapChainImages, m_fbaLowRes[j], m_lowResolutionRenderFormat,
				layoutBeforeImageCreation, layoutToTransitionImageToAfterCreation, m_windowExtents, frameBufferUsage);
		}
	}
}



inline void VulkanRendererBackend::expandDescriptorPool_PostProcess(std::vector<VkDescriptorPoolSize>& poolSizes)
{
	//Common
	poolSizes.push_back({ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, m_numSwapChainImages }); // before post process
	poolSizes.push_back({ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, m_numSwapChainImages }); // high res frame 1
	poolSizes.push_back({ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, m_numSwapChainImages }); // high res frame 2
	poolSizes.push_back({ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, m_numSwapChainImages }); // low res frame 1
	poolSizes.push_back({ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, m_numSwapChainImages }); // low res frame 2

	// Test High Res Pass
	// Tonemap
	// Test low Res Pass
}
inline void VulkanRendererBackend::createDescriptors_PostProcess_Common(VkDescriptorPool descriptorPool)
{
	int serialCount = 0;

	m_postProcessDescriptorsCommon.push_back(DescriptorUtil::createImgSamplerDescriptor(
		serialCount, "SampleFrameBeforePostProcess", m_numSwapChainImages, descriptorPool, m_logicalDevice));
	serialCount++;

	m_postProcessDescriptorsCommon.push_back( DescriptorUtil::createImgSamplerDescriptor(
		serialCount, "SampleFrameHighResolution1", m_numSwapChainImages, descriptorPool, m_logicalDevice) );
	serialCount++;

	m_postProcessDescriptorsCommon.push_back(DescriptorUtil::createImgSamplerDescriptor(
		serialCount, "SampleFrameHighResolution2", m_numSwapChainImages, descriptorPool, m_logicalDevice));
	serialCount++;

	m_postProcessDescriptorsCommon.push_back(DescriptorUtil::createImgSamplerDescriptor(
		serialCount, "SampleFrameLowResolution1", m_numSwapChainImages, descriptorPool, m_logicalDevice));
	serialCount++;

	m_postProcessDescriptorsCommon.push_back(DescriptorUtil::createImgSamplerDescriptor(
		serialCount, "SampleFrameLowResolution2", m_numSwapChainImages, descriptorPool, m_logicalDevice));
	serialCount++;
}
inline void VulkanRendererBackend::createDescriptors_PostProcess_Specific(VkDescriptorPool descriptorPool)
{
	int serialCount = 0;
	// Test High Res Pass
	//{
	//	PostProcessDescriptors tonemapDescriptor;
	//	tonemapDescriptor.descriptorName = "HighResTestPass";
	//	tonemapDescriptor.serialIndex = serialCount;
	//	tonemapDescriptor.postProcess_DSs.resize(m_numSwapChainImages);

	//	const uint32_t numBindings = 1;
	//	VkDescriptorSetLayoutBinding inputImageLayoutBinding = { 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr };
	//	std::array<VkDescriptorSetLayoutBinding, numBindings> toneMapBindings = { inputImageLayoutBinding };
	//	DescriptorUtil::createDescriptorSetLayout(m_logicalDevice, tonemapDescriptor.postProcess_DSL, numBindings, toneMapBindings.data());

	//	for (uint32_t i = 0; i < m_numSwapChainImages; i++)
	//	{
	//		DescriptorUtil::createDescriptorSets(m_logicalDevice, descriptorPool, 1, &tonemapDescriptor.postProcess_DSL, &tonemapDescriptor.postProcess_DSs[i]);
	//	}
	//	tonemapDescriptor.genericDescriptorsUsed.push_back(DSL_TYPE::TIME);

	//	m_postProcessDescriptorsSpecific.push_back(tonemapDescriptor);
	//	serialCount++;
	//}

	// Tonemap
	{
		//PostProcessDescriptors tonemapDescriptor;
		//tonemapDescriptor.descriptorName = "Tonemap";
		//tonemapDescriptor.serialIndex = serialCount;
		//tonemapDescriptor.postProcess_DSs.resize(m_numSwapChainImages);

		//tonemapDescriptor.genericDescriptorsUsed.push_back(DSL_TYPE::BEFOREPOST_FRAME);

		//m_postProcessDescriptorsSpecific.push_back(tonemapDescriptor);
		//serialCount++;
	}

	// TXAA and other Post Process effects
}
inline void VulkanRendererBackend::writeToAndUpdateDescriptorSets_PostProcess_Common()
{
	// Ordering is very very important here. 
	// It has to match the order of descriptors in createDescriptors_PostProcess_Common(...)
	int index = 0;

	DescriptorUtil::writeToImageSamplerDescriptor(m_postProcessDescriptorsCommon[index].postProcess_DSs,
		m_prePostProcessInput, m_postProcessSampler, m_numSwapChainImages, m_logicalDevice);
	index++;

	DescriptorUtil::writeToImageSamplerDescriptor(m_postProcessDescriptorsCommon[index].postProcess_DSs,
		m_fbaHighRes[0], VK_IMAGE_LAYOUT_GENERAL, m_postProcessSampler, m_numSwapChainImages, m_logicalDevice);
	index++;

	DescriptorUtil::writeToImageSamplerDescriptor(m_postProcessDescriptorsCommon[index].postProcess_DSs,
		m_fbaHighRes[1], VK_IMAGE_LAYOUT_GENERAL, m_postProcessSampler, m_numSwapChainImages, m_logicalDevice);
	index++;

	DescriptorUtil::writeToImageSamplerDescriptor(m_postProcessDescriptorsCommon[index].postProcess_DSs,
		m_fbaLowRes[0], VK_IMAGE_LAYOUT_GENERAL, m_postProcessSampler, m_numSwapChainImages, m_logicalDevice);
	index++;

	DescriptorUtil::writeToImageSamplerDescriptor(m_postProcessDescriptorsCommon[index].postProcess_DSs,
		m_fbaLowRes[1], VK_IMAGE_LAYOUT_GENERAL, m_postProcessSampler, m_numSwapChainImages, m_logicalDevice);
	index++;
}
inline void VulkanRendererBackend::writeToAndUpdateDescriptorSets_PostProcess_Specific()
{
	for (uint32_t i = 0; i < m_numSwapChainImages; i++)
	{
		// Tone Map
		// TODO: change to something that searches for the particular string name later
		int index = 0; // controls which descriptor is being accessed
		{
			//// The input image for the tonemap is the last renderpass in the 32 bit passes.
			//// TODO: The inputImages data is probably not needed and can be removed at a later point
			//// We should simply be able to use the data from the last renderpass if things are in the correct order.
			//// We are already doing this with the image layout.
			//VkDescriptorImageInfo inputImageInfo = DescriptorUtil::createDescriptorImageInfo(
			//	m_postProcessSampler,
			//	m_prePostProcessInput[i].imageView,
			//	m_prePostProcessInput[i].imageLayout);

			//const int descriptorCount = 1;
			//std::array<VkWriteDescriptorSet, descriptorCount> writeToneMapSetInfo = {};
			//writeToneMapSetInfo[0] = DescriptorUtil::writeDescriptorSet(
			//	m_postProcessDescriptorsSpecific[index].postProcess_DSs[i], 0, 1, 
			//	VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &inputImageInfo);

			//vkUpdateDescriptorSets(m_logicalDevice, descriptorCount, writeToneMapSetInfo.data(), 0, nullptr);
		}

		// Another Postprocess pass
		index++;
	}
}



inline void VulkanRendererBackend::addPostProcessPass(std::string effectName,
	std::vector<VkDescriptorSetLayout>& effectDSL, POST_PROCESS_TYPE postType, PostProcessRPI& postRPI)
{
	m_postEffectNames.push_back(effectName);	
	postRPI.serialIndex = m_numPostEffects;
	postRPI.postType = postType;

	// All framebuffer attachments have been transitioned to VK_IMAGE_LAYOUT_GENERAL after creation
	// Currently we only need to tranisiont the image to something other than VK_IMAGE_LAYOUT_GENERAL if it is the last pass.
	// This is because after the last pass we will directly copy the result into the swapchain
	const VkImageLayout layoutAfterImageCreation = VK_IMAGE_LAYOUT_GENERAL;
	const VkImageLayout layoutAfterRenderPassExecuted = VK_IMAGE_LAYOUT_GENERAL;
	const VkFormat depthFormat = VK_FORMAT_UNDEFINED; // m_depth.format
		
	if (postType == POST_PROCESS_TYPE::HIGH_RESOLUTION)
	{
		addRenderPass_PostProcess(postRPI.renderPass, m_highResolutionRenderFormat, depthFormat, layoutAfterImageCreation, layoutAfterRenderPassExecuted);
		addFrameBuffers_PostProcess(postRPI, m_highResolutionRenderFormat, postType, m_fbaHighRes[m_fbaHighResIndexInUse], layoutAfterRenderPassExecuted);
		addPipeline_PostProcess(effectName, effectDSL, postRPI.renderPass);

		m_fbaHighResIndexInUse = (m_fbaHighResIndexInUse + 1) % 2;
	}
	else if (postType == POST_PROCESS_TYPE::TONEMAP)
	{
		// undefined depth format means we dont add depth as a attachment to the renderpass
		addRenderPass_PostProcess(postRPI.renderPass, m_lowResolutionRenderFormat, depthFormat,	layoutAfterImageCreation, layoutAfterRenderPassExecuted);
		addFrameBuffers_PostProcess(postRPI, m_lowResolutionRenderFormat, postType,	m_fbaLowRes[m_fbaLowResIndexInUse], layoutAfterRenderPassExecuted);
		addPipeline_PostProcess(effectName, effectDSL, postRPI.renderPass);

		m_fbaLowResIndexInUse++; // Can do this because tone map pass should only happen once
	}
	else if (postType == POST_PROCESS_TYPE::LOW_RESOLUTION)
	{
		addRenderPass_PostProcess(postRPI.renderPass, m_lowResolutionRenderFormat, depthFormat,	layoutAfterImageCreation, layoutAfterRenderPassExecuted);
		addFrameBuffers_PostProcess(postRPI, m_lowResolutionRenderFormat, postType,	m_fbaLowRes[m_fbaLowResIndexInUse], layoutAfterRenderPassExecuted);
		addPipeline_PostProcess(effectName, effectDSL, postRPI.renderPass);

		m_fbaLowResIndexInUse = (m_fbaLowResIndexInUse + 1) % 2;
	}

	m_postProcessRPIs.push_back(postRPI);
	m_numPostEffects++;
}

inline void VulkanRendererBackend::addRenderPass_PostProcess( VkRenderPass& l_renderPass, 
	const VkFormat colorFormat, const VkFormat depthFormat, const VkImageLayout initialLayout, const VkImageLayout finalLayout)
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
		colorFormat, depthFormat, initialLayout, finalLayout, subpassDependencies);
}

inline void VulkanRendererBackend::addFrameBuffers_PostProcess(
	PostProcessRPI& passRPI, VkFormat colorFormat, POST_PROCESS_TYPE postType,
	std::vector<FrameBufferAttachment>& fbAttachments, const VkImageLayout afterRenderPassExecuted)
{
	passRPI.frameBuffers.resize(m_numSwapChainImages);
	passRPI.imageSetInfo.resize(m_numSwapChainImages);
	
	for (uint32_t i = 0; i < m_numSwapChainImages; i++)
	{
		std::array<VkImageView, 1> attachments = { fbAttachments[i].view };

		FrameResourcesUtil::createFrameBuffer(m_logicalDevice, passRPI.frameBuffers[i], passRPI.renderPass,
			m_windowExtents, static_cast<uint32_t>(attachments.size()), attachments.data());

		// Fill a descriptor for later use in a descriptor set 
		passRPI.imageSetInfo[i].imageLayout = afterRenderPassExecuted;
		passRPI.imageSetInfo[i].imageView = fbAttachments[i].view;
		passRPI.imageSetInfo[i].sampler = m_postProcessSampler;
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
	// Define push constant
	// Spec requires a minimum of 128 bytes, bigger values need to be checked against maxPushConstantsSize
	VkPushConstantRange pushConstantRange = { VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(shaderConstants) };
	VkPipelineLayout postProcessPL = VulkanPipelineCreation::createPipelineLayout(m_logicalDevice, l_postProcessDSL, 1, &pushConstantRange);
	m_postProcess_PLs.push_back(postProcessPL);

	// -------- Create Post Process pipeline ---------
	VkPipeline postProcessP;
	VulkanPipelineCreation::createPostProcessPipeline(m_logicalDevice,
		postProcessP, m_postProcess_PLs[m_numPostEffects], l_renderPass, subpass,
		stageCount, shaderStages.data(), extents);
	m_postProcess_Ps.push_back(postProcessP);

	// No need for the shader modules anymore, so we destory them!
	vkDestroyShaderModule(m_logicalDevice, vertShaderModule, nullptr);
	vkDestroyShaderModule(m_logicalDevice, fragShaderModule, nullptr);
}


inline DSL_TYPE VulkanRendererBackend::chooseHighResInput()
{
	return (m_fbaHighResIndexInUse == 1) ? DSL_TYPE::POST_HRFRAME1 : DSL_TYPE::POST_HRFRAME2;
}
inline DSL_TYPE VulkanRendererBackend::chooseLowResInput()
{
	return (m_fbaLowResIndexInUse == 1) ? DSL_TYPE::POST_LRFRAME1 : DSL_TYPE::POST_LRFRAME2;
}