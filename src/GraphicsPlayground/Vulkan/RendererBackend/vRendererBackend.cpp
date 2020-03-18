#include "Vulkan/RendererBackend/vRendererBackend.h"

VulkanRendererBackend::VulkanRendererBackend(std::shared_ptr<VulkanManager> vulkanObject, int numSwapChainImages, VkExtent2D windowExtents) :
	m_vulkanObj(vulkanObject), m_logicalDevice(vulkanObject->getLogicalDevice()), m_physicalDevice(vulkanObject->getPhysicalDevice()),
	m_graphicsQueue(vulkanObject->getQueue(QueueFlags::Graphics)), m_computeQueue(vulkanObject->getQueue(QueueFlags::Compute)),
	m_numSwapChainImages(numSwapChainImages), m_windowExtents(windowExtents)
{
	m_depthFormat = FormatUtil::findDepthFormat(m_physicalDevice);
	
	createCommandPoolsAndBuffers();
	createRenderPassesAndFrameResources();
}
VulkanRendererBackend::~VulkanRendererBackend()
{
	vkDeviceWaitIdle(m_logicalDevice);

	// Destroy Command Pools
	vkDestroyCommandPool(m_logicalDevice, m_graphicsCommandPool, nullptr);
	vkDestroyCommandPool(m_logicalDevice, m_computeCommandPool, nullptr);

	// Descriptor Pool
	vkDestroyDescriptorPool(m_logicalDevice, m_descriptorPool, nullptr);
	// Descriptor Set Layouts
	vkDestroyDescriptorSetLayout(m_logicalDevice, m_DSL_compositeComputeOntoRaster, nullptr);
	//vkDestroyDescriptorSetLayout(m_logicalDevice, m_DSL_compute, nullptr);
	for (int i = 0; i < m_postProcessDescriptors.size(); i++)
	{
		vkDestroyDescriptorSetLayout(m_logicalDevice, m_postProcessDescriptors[i].postProcess_DSL, nullptr);
	}
}
void VulkanRendererBackend::cleanup()
{
	// Command Buffers
	vkFreeCommandBuffers(m_logicalDevice, m_graphicsCommandPool, static_cast<uint32_t>(m_graphicsCommandBuffers.size()), m_graphicsCommandBuffers.data());
	vkFreeCommandBuffers(m_logicalDevice, m_computeCommandPool, static_cast<uint32_t>(m_computeCommandBuffers.size()), m_computeCommandBuffers.data());

	cleanupPipelines();
	cleanupRenderPassesAndFrameResources();
	cleanupPostProcess();
}


void VulkanRendererBackend::createPipelines(DescriptorSetLayouts& pipelineDescriptorSetLayouts)
{
	std::vector<VkDescriptorSetLayout>& compute_DSL = pipelineDescriptorSetLayouts.computeDSL;
	std::vector<VkDescriptorSetLayout>& compositeComputeOntoRaster_DSL = pipelineDescriptorSetLayouts.compositeComputeOntoRasterDSL;
	std::vector<VkDescriptorSetLayout>& rasterization_DSL = pipelineDescriptorSetLayouts.geomDSL;

	m_compute_PL = VulkanPipelineCreation::createPipelineLayout(m_logicalDevice, compute_DSL, 0, nullptr);
	createComputePipeline(m_compute_P, m_compute_PL, "testComputeShader");

	createCompositeComputeOntoRasterPipeline(compositeComputeOntoRaster_DSL);
	createRasterizationRenderPipeline(rasterization_DSL);
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
	
	// Add 32 bit passes
	//addPostProcessPass("32bit", std::vector<VkDescriptorSetLayout>& effectDSL, POST_PROCESS_GROUP::PASS_32BIT);

	// Tone Map Pass
	{
		std::vector<VkDescriptorSetLayout> effectDSL;
		effectDSL.push_back(m_postProcessDescriptors[0].postProcess_DSL);
		effectDSL.push_back(scene->getDescriptorSetLayout(DSL_TYPE::TIME));
		addPostProcessPass("tonemap", effectDSL, POST_PROCESS_GROUP::PASS_TONEMAP);
	}

	// Add 8 bit passes
	//addPostProcessPass("8bit", std::vector<VkDescriptorSetLayout>& effectDSL, POST_PROCESS_GROUP::PASS_8BIT);

	// Transition the last image rendered to the layout accepted by the UI renderpass
}

//===============================================================================================
//===============================================================================================
//===============================================================================================

// Descriptor Sets
void VulkanRendererBackend::expandDescriptorPool(std::vector<VkDescriptorPoolSize>& poolSizes)
{
	// Composite Compute Onto Raster pass writes to the swapchain image(s) so doesnt need a separate desciptor for that
	// It does need descriptors for any images it reads in

	// COMPOSITE COMPUTE ONTO RASTER
	poolSizes.push_back({ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, m_numSwapChainImages }); // compute image
	poolSizes.push_back({ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, m_numSwapChainImages }); // geomRenderPass image

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
	// Descriptor Set Layouts
	{
		// Descriptor set layouts are specified in the pipeline layout object., i.e. during pipeline creation to tell Vulkan 
		// which descriptors the shaders will be using.
		// The numbers are bindingCount, binding, and descriptorCount respectively

		// RASTER + COMPUTE COMPOSITE
		const uint32_t numImagesComposited = 2;
		VkDescriptorSetLayoutBinding computePassImageLB = { 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr };
		VkDescriptorSetLayoutBinding geomRenderedImageLB = { 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr };

		std::array<VkDescriptorSetLayoutBinding, numImagesComposited> compositeComputeOntoRasterLBs = { computePassImageLB, geomRenderedImageLB };
		DescriptorUtil::createDescriptorSetLayout(m_logicalDevice, m_DSL_compositeComputeOntoRaster, numImagesComposited, compositeComputeOntoRasterLBs.data());
	}

	// Descriptor Sets
	{
		m_DS_compositeComputeOntoRaster.resize(m_numSwapChainImages);
		for (uint32_t i = 0; i < m_numSwapChainImages; i++)
		{
			// Composite Compute Onto Raster
			DescriptorUtil::createDescriptorSets(m_logicalDevice, descriptorPool, 1, &m_DSL_compositeComputeOntoRaster, &m_DS_compositeComputeOntoRaster[i]);
		}
	}

	createDescriptors_PostProcess(descriptorPool);
}
void VulkanRendererBackend::writeToAndUpdateDescriptorSets(DescriptorSetDependencies& descSetDependencies)
{
	const uint32_t numImagesComposited = 2;

	for (uint32_t i = 0; i < m_numSwapChainImages; i++)
	{
		// Composite Compute onto Raster
		{
			VkDescriptorImageInfo computePassImageSetInfo = DescriptorUtil::createDescriptorImageInfo(
				descSetDependencies.computeImages[i]->getSampler(),
				descSetDependencies.computeImages[i]->getImageView(),
				descSetDependencies.computeImages[i]->getImageLayout());
			VkDescriptorImageInfo geomRenderedImageSetInfo = DescriptorUtil::createDescriptorImageInfo(
				descSetDependencies.geomRenderPassImageSet[i].sampler,
				descSetDependencies.geomRenderPassImageSet[i].imageView,
				descSetDependencies.geomRenderPassImageSet[i].imageLayout);

			std::array<VkWriteDescriptorSet, numImagesComposited> writeCompositeComputeOntoRasterSetInfo = {};
			writeCompositeComputeOntoRasterSetInfo[0] = DescriptorUtil::writeDescriptorSet(
				m_DS_compositeComputeOntoRaster[i], 0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &computePassImageSetInfo);
			writeCompositeComputeOntoRasterSetInfo[1] = DescriptorUtil::writeDescriptorSet(
				m_DS_compositeComputeOntoRaster[i], 1, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &geomRenderedImageSetInfo);

			vkUpdateDescriptorSets(m_logicalDevice, numImagesComposited, writeCompositeComputeOntoRasterSetInfo.data(), 0, nullptr);
		}
	}

	writeToAndUpdateDescriptorSets_PostProcess();
}

//===============================================================================================

// Getters
VkPipeline VulkanRendererBackend::getPipeline(PIPELINE_TYPE type, int postProcessIndex)
{
	switch (type)
	{
	case PIPELINE_TYPE::RASTER:
		return m_rasterization_P;
		break;
	case PIPELINE_TYPE::COMPOSITE_COMPUTE_ONTO_RASTER:
		return m_compositeComputeOntoRaster_P;
		break;
	case PIPELINE_TYPE::COMPUTE:
		return m_compute_P;
		break;
	case PIPELINE_TYPE::POST_PROCESS:
		return m_postProcess_Ps[postProcessIndex];
		break;
	default:
		throw std::runtime_error("no such Descriptor Set Layout Type (DSL_TYPE) exists");
	}

	return nullptr;
}
VkPipelineLayout VulkanRendererBackend::getPipelineLayout(PIPELINE_TYPE type, int postProcessIndex)
{
	switch (type)
	{
	case PIPELINE_TYPE::RASTER:
		return m_rasterization_PL;
		break;
	case PIPELINE_TYPE::COMPOSITE_COMPUTE_ONTO_RASTER:
		return m_compositeComputeOntoRaster_PL;
		break;
	case PIPELINE_TYPE::COMPUTE:
		return m_compute_PL;
		break;
	case PIPELINE_TYPE::POST_PROCESS:
		return m_postProcess_PLs[postProcessIndex];
		break;
	default:
		throw std::runtime_error("no such Descriptor Set Layout Type (DSL_TYPE) exists");
	}

	return nullptr;
}
VkDescriptorSet VulkanRendererBackend::getDescriptorSet(DSL_TYPE type, int frameIndex, int postProcessIndex)
{
	switch (type)
	{
	case DSL_TYPE::COMPOSITE_COMPUTE_ONTO_RASTER:
		return m_DS_compositeComputeOntoRaster[frameIndex];
		break;
	case DSL_TYPE::POST_PROCESS:
		return m_postProcessDescriptors[postProcessIndex].postProcess_DSs[frameIndex];
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
	case DSL_TYPE::COMPOSITE_COMPUTE_ONTO_RASTER:
		return m_DSL_compositeComputeOntoRaster;
		break;
	case DSL_TYPE::POST_PROCESS:
		return m_postProcessDescriptors[postProcessIndex].postProcess_DSL;
		break;
	default:
		throw std::runtime_error("no such Descriptor Set Layout Type (DSL_TYPE) exists");
	}

	assert(("Did not find a Descriptor Set Layout to return", true));
	return nullptr;
}
