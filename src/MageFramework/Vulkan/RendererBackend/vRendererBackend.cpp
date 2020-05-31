#include "Vulkan/RendererBackend/vRendererBackend.h"

VulkanRendererBackend::VulkanRendererBackend(std::shared_ptr<VulkanManager> vulkanManager, 
	RendererOptions& rendererOptions, int numSwapChainImages, VkExtent2D windowExtents) :
	m_vulkanManager(vulkanManager), m_logicalDevice(vulkanManager->getLogicalDevice()), m_physicalDevice(vulkanManager->getPhysicalDevice()),
	m_graphicsQueue(vulkanManager->getQueue(QueueFlags::Graphics)), m_computeQueue(vulkanManager->getQueue(QueueFlags::Compute)),
	m_rendererOptions(rendererOptions), m_numSwapChainImages(numSwapChainImages), m_windowExtents(windowExtents)
{
	m_highResolutionRenderFormat = VK_FORMAT_R32G32B32A32_SFLOAT;
	m_lowResolutionRenderFormat = m_vulkanManager->getSwapChainImageFormat();
	m_depthFormat = FormatUtil::findDepthFormat(m_physicalDevice);

	if (m_rendererOptions.renderType == RENDER_TYPE::RAYTRACE)
	{
		getRayTracingFunctionPointers();
	}

	createCommandPoolsAndBuffers();
	createRenderPassesAndFrameResources();
}
VulkanRendererBackend::~VulkanRendererBackend()
{
	vkDeviceWaitIdle(m_logicalDevice);

	// Destroy Sync Objects
	const uint32_t numFrames = m_vulkanManager->getSwapChainImageCount();
	for (size_t i = 0; i < numFrames; i++)
	{
		vkDestroySemaphore(m_logicalDevice, m_renderOperationsFinishedSemaphores[i], nullptr);
		vkDestroySemaphore(m_logicalDevice, m_computeOperationsFinishedSemaphores[i], nullptr);
		vkDestroySemaphore(m_logicalDevice, m_postProcessFinishedSemaphores[i], nullptr);
	}

	// Destroy Command Pools
	vkDestroyCommandPool(m_logicalDevice, m_graphicsCmdPool, nullptr);
	vkDestroyCommandPool(m_logicalDevice, m_computeCmdPool, nullptr);

	// Descriptor Pool
	vkDestroyDescriptorPool(m_logicalDevice, m_descriptorPool, nullptr);

	// Descriptor Set Layouts
	vkDestroyDescriptorSetLayout(m_logicalDevice, m_DSL_rayTrace, nullptr);
	for (PostProcessDescriptors postProcessDescriptor_specific : m_postProcessDescriptorsSpecific) {
		vkDestroyDescriptorSetLayout(m_logicalDevice, postProcessDescriptor_specific.postProcess_DSL, nullptr);
	}
	for (PostProcessDescriptors postProcessDescriptor_common : m_postProcessDescriptorsCommon)	{
		vkDestroyDescriptorSetLayout(m_logicalDevice, postProcessDescriptor_common.postProcess_DSL, nullptr);
	}

	if (m_rendererOptions.renderType == RENDER_TYPE::RAYTRACE)
	{
		destroyRayTracing();
	}
}
void VulkanRendererBackend::cleanup()
{
	vkDeviceWaitIdle(m_logicalDevice);

	// Command Buffers
	vkFreeCommandBuffers(m_logicalDevice, m_computeCmdPool, static_cast<uint32_t>(m_computeCommandBuffers.size()), m_computeCommandBuffers.data());
	vkFreeCommandBuffers(m_logicalDevice, m_graphicsCmdPool, static_cast<uint32_t>(m_graphicsCommandBuffers.size()), m_graphicsCommandBuffers.data());
	vkFreeCommandBuffers(m_logicalDevice, m_graphicsCmdPool, static_cast<uint32_t>(m_rayTracingCommandBuffers.size()), m_rayTracingCommandBuffers.data());
	vkFreeCommandBuffers(m_logicalDevice, m_graphicsCmdPool, static_cast<uint32_t>(m_postProcessCommandBuffers.size()), m_postProcessCommandBuffers.data());

	cleanupPipelines();
	cleanupRenderPassesAndFrameResources();
	cleanupPostProcess();
	cleanupRayTracing();
}


void VulkanRendererBackend::createPipelines(DescriptorSetLayouts& pipelineDescriptorSetLayouts)
{
	std::vector<VkDescriptorSetLayout>& compute_DSL = pipelineDescriptorSetLayouts.computeDSL;
	std::vector<VkDescriptorSetLayout>& rasterization_DSL = pipelineDescriptorSetLayouts.rasterDSL;
	std::vector<VkDescriptorSetLayout>& rayTrace_DSL = pipelineDescriptorSetLayouts.raytraceDSL;
	
	m_compute_PL = VulkanPipelineCreation::createPipelineLayout(m_logicalDevice, compute_DSL, 0, nullptr);
	createComputePipeline(m_compute_P, m_compute_PL, "testComputeShader");

	if (m_rendererOptions.renderType == RENDER_TYPE::RASTERIZATION)
	{
		createRasterizationRenderPipeline(rasterization_DSL);
	}
	if (m_rendererOptions.renderType == RENDER_TYPE::RAYTRACE)
	{
		createRayTracePipeline(rayTrace_DSL);
	}
}
void VulkanRendererBackend::createRenderPassesAndFrameResources()
{
	const VkImageLayout layoutBeforeImageCreation = VK_IMAGE_LAYOUT_UNDEFINED; // can be VK_IMAGE_LAYOUT_UNDEFINED or VK_IMAGE_LAYOUT_PREINITIALIZED
	const VkImageLayout layoutToTransitionImageToAfterCreation = VK_IMAGE_LAYOUT_UNDEFINED; // No transition if same as layoutBeforeImageCreation
	const VkImageLayout layoutAfterRenderPassExecuted = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	createRenderPasses(layoutToTransitionImageToAfterCreation, layoutAfterRenderPassExecuted);
	createDepthResources();
	createFrameBuffers(layoutBeforeImageCreation, layoutToTransitionImageToAfterCreation, layoutAfterRenderPassExecuted);
}
void VulkanRendererBackend::createAllPostProcessEffects(std::shared_ptr<Scene> scene)
{
	prePostProcess();

	const int& postProcessIndex = m_numPostEffects;
	// Add High Resolution passes
	{
		// Test High Res pass 1
		{
			PostProcessRPI postRPI;
			std::vector<VkDescriptorSetLayout> effectDSL;
			const DSL_TYPE inputToBeRead = DSL_TYPE::BEFOREPOST_FRAME;
			effectDSL.push_back(getDescriptorSetLayout(inputToBeRead));
			effectDSL.push_back(scene->getDescriptorSetLayout(DSL_TYPE::COMPUTE));
			for (uint32_t j = 0; j < m_numSwapChainImages; j++)
			{
				postRPI.descriptors.push_back(getDescriptorSet(inputToBeRead, j));
				postRPI.descriptors.push_back(scene->getDescriptorSet(DSL_TYPE::COMPUTE, j));
			}
			addPostProcessPass("HighResTestPass1", effectDSL, POST_PROCESS_TYPE::HIGH_RESOLUTION, postRPI);
		}

		// Test High Res pass 2
		{
			PostProcessRPI postRPI;
			std::vector<VkDescriptorSetLayout> effectDSL;
			const DSL_TYPE inputToBeRead = chooseHighResInput();
			effectDSL.push_back(getDescriptorSetLayout(inputToBeRead));
			for (uint32_t j = 0; j < m_numSwapChainImages; j++)
			{
				postRPI.descriptors.push_back(getDescriptorSet(inputToBeRead, j));
			}
			addPostProcessPass("HighResTestPass2", effectDSL, POST_PROCESS_TYPE::HIGH_RESOLUTION, postRPI);
		}
	}

	// Tone Map Pass
	{
		PostProcessRPI postRPI;
		std::vector<VkDescriptorSetLayout> effectDSL;
		const DSL_TYPE inputToBeRead = chooseHighResInput();
		effectDSL.push_back(getDescriptorSetLayout(inputToBeRead));
		effectDSL.push_back(scene->getDescriptorSetLayout(DSL_TYPE::TIME));
		// effectDSL.push_back(m_postProcessDescriptorsSpecific[postProcessIndex].postProcess_DSL);
		for (uint32_t j = 0; j < m_numSwapChainImages; j++)
		{
			postRPI.descriptors.push_back(getDescriptorSet(inputToBeRead, j));
			postRPI.descriptors.push_back(scene->getDescriptorSet(DSL_TYPE::TIME, j));
		}
 		addPostProcessPass("Tonemap", effectDSL, POST_PROCESS_TYPE::TONEMAP, postRPI);

		shaderConstants.toneMap_Exposure = 1.0f;
		shaderConstants.toneMap_WhitePoint = 1.0f;
	}

	// Add Low Resolution Passes
	{
		// Test Low Res pass 1
		{
			PostProcessRPI postRPI;
			std::vector<VkDescriptorSetLayout> effectDSL;
			const DSL_TYPE inputToBeRead = chooseLowResInput();
			effectDSL.push_back(getDescriptorSetLayout(inputToBeRead));
			for (uint32_t j = 0; j < m_numSwapChainImages; j++)
			{
				postRPI.descriptors.push_back(getDescriptorSet(inputToBeRead, j));
			}
			addPostProcessPass("LowResTestPass1", effectDSL, POST_PROCESS_TYPE::LOW_RESOLUTION, postRPI);
		}

		// Test Low Res pass 2
		{
			PostProcessRPI postRPI;
			std::vector<VkDescriptorSetLayout> effectDSL;
			const DSL_TYPE inputToBeRead = chooseLowResInput();
			effectDSL.push_back(getDescriptorSetLayout(inputToBeRead));
			for (uint32_t j = 0; j < m_numSwapChainImages; j++)
			{
				postRPI.descriptors.push_back(getDescriptorSet(inputToBeRead, j));
			}
			addPostProcessPass("LowResTestPass2", effectDSL, POST_PROCESS_TYPE::LOW_RESOLUTION, postRPI);
		}
	}
}


void VulkanRendererBackend::update(uint32_t currentImageIndex)
{}

//===============================================================================================
//===============================================================================================
//===============================================================================================

// Descriptor Sets
void VulkanRendererBackend::expandDescriptorPool(std::vector<VkDescriptorPoolSize>& poolSizes)
{
	// COMPOSITE COMPUTE onto Raster Image in First Post Process Pass
	poolSizes.push_back({ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, m_numSwapChainImages }); // compute image read by First Post Process Pass

	// RAY TRACING
	poolSizes.push_back({ VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV, m_numSwapChainImages }); // ray Tracing acceleration structures
	poolSizes.push_back({ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, m_numSwapChainImages }); // rayTraced Image
	poolSizes.push_back({ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2*m_numSwapChainImages }); // Camera & Lights UBO
	poolSizes.push_back({ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2*m_numSwapChainImages }); // Vertex and Index Buffers

	expandDescriptorPool_PostProcess(poolSizes);
}
void VulkanRendererBackend::createDescriptorPool(std::vector<VkDescriptorPoolSize>& poolSizes)
{
	// There's a  difference between descriptor Sets and bindings inside a descriptor Set, 
	// multiple bindings can exist inside a descriptor set. So a pool can have fewer sets than number of bindings.
	const uint32_t numSets = 1000;
	const uint32_t numFrames = static_cast<uint32_t>(m_numSwapChainImages);
	const uint32_t maxSets = numSets * numFrames;
	const uint32_t poolSizesCount = static_cast<uint32_t>(poolSizes.size());
	DescriptorUtil::createDescriptorPool(m_logicalDevice, maxSets, poolSizesCount, poolSizes.data(), m_descriptorPool);
}
void VulkanRendererBackend::createDescriptors(VkDescriptorPool descriptorPool)
{
	// Create Descriptor Set Layouts and then the Create Descriptor Sets
	createDescriptors_PostProcess_Common(descriptorPool);
	createDescriptors_PostProcess_Specific(descriptorPool);
	createDescriptors_rayTracing(descriptorPool);
}
void VulkanRendererBackend::writeToAndUpdateDescriptorSets()
{
	writeToAndUpdateDescriptorSets_PostProcess_Common();
	writeToAndUpdateDescriptorSets_PostProcess_Specific();
}

//===============================================================================================

// Getters
VkDescriptorSet VulkanRendererBackend::getDescriptorSet(DSL_TYPE type, int frameIndex, int postProcessIndex)
{
	switch (type)
	{
	case DSL_TYPE::POST_PROCESS:
		return m_postProcessDescriptorsSpecific[postProcessIndex].postProcess_DSs[frameIndex];
		break;
	case DSL_TYPE::BEFOREPOST_FRAME:
		return m_postProcessDescriptorsCommon[0].postProcess_DSs[frameIndex];
		break;
	case DSL_TYPE::POST_HRFRAME1:
		return m_postProcessDescriptorsCommon[1].postProcess_DSs[frameIndex];
		break;
	case DSL_TYPE::POST_HRFRAME2:
		return m_postProcessDescriptorsCommon[2].postProcess_DSs[frameIndex];
		break;
	case DSL_TYPE::POST_LRFRAME1:
		return m_postProcessDescriptorsCommon[3].postProcess_DSs[frameIndex];
		break;
	case DSL_TYPE::POST_LRFRAME2:
		return m_postProcessDescriptorsCommon[4].postProcess_DSs[frameIndex];
		break;
	default:
		throw std::runtime_error("no such Descriptor Set Layout Type (DSL_TYPE) exists");
	}

	return nullptr;
}
VkDescriptorSetLayout VulkanRendererBackend::getDescriptorSetLayout(DSL_TYPE type, int postProcessIndex)
{
	switch (type)
	{
	case DSL_TYPE::POST_PROCESS:
		return m_postProcessDescriptorsSpecific[postProcessIndex].postProcess_DSL;
		break;
	case DSL_TYPE::BEFOREPOST_FRAME:
		return m_postProcessDescriptorsCommon[0].postProcess_DSL;
		break;
	case DSL_TYPE::POST_HRFRAME1:
		return m_postProcessDescriptorsCommon[1].postProcess_DSL;
		break;
	case DSL_TYPE::POST_HRFRAME2:
		return m_postProcessDescriptorsCommon[2].postProcess_DSL;
		break;
	case DSL_TYPE::POST_LRFRAME1:
		return m_postProcessDescriptorsCommon[3].postProcess_DSL;
		break;
	case DSL_TYPE::POST_LRFRAME2:
		return m_postProcessDescriptorsCommon[4].postProcess_DSL;
		break;
	default:
		throw std::runtime_error("no such Descriptor Set Layout Type (DSL_TYPE) exists");
	}

	return nullptr;
}
