#include "vulkanRendererBackend.h"

VulkanRendererBackend::VulkanRendererBackend(VulkanManager* vulkanObject, int numSwapChainImages, VkExtent2D windowExtents) :
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
	// Destroy Command Pools
	vkDestroyCommandPool(m_logicalDevice, m_graphicsCommandPool, nullptr);
	vkDestroyCommandPool(m_logicalDevice, m_computeCommandPool, nullptr);

	// Descriptor Pool
	vkDestroyDescriptorPool(m_logicalDevice, m_descriptorPool, nullptr);
	// Descriptor Set Layouts
	vkDestroyDescriptorSetLayout(m_logicalDevice, m_DSL_finalComposite, nullptr);
	//vkDestroyDescriptorSetLayout(m_logicalDevice, m_DSL_compute, nullptr);
}
void VulkanRendererBackend::cleanup()
{
	// Command Buffers
	vkFreeCommandBuffers(m_logicalDevice, m_graphicsCommandPool, static_cast<uint32_t>(m_graphicsCommandBuffers.size()), m_graphicsCommandBuffers.data());
	vkFreeCommandBuffers(m_logicalDevice, m_computeCommandPool, static_cast<uint32_t>(m_computeCommandBuffers.size()), m_computeCommandBuffers.data());

	cleanupPipelines();
	cleanupRenderPassesAndFrameResources();
}
void VulkanRendererBackend::cleanupPipelines()
{
	// Compute Pipeline
	vkDestroyPipeline(m_logicalDevice, m_compute_P, nullptr);
	vkDestroyPipelineLayout(m_logicalDevice, m_compute_PL, nullptr);
	// Final Composite Pipeline
	vkDestroyPipeline(m_logicalDevice, m_finalComposite_P, nullptr);
	vkDestroyPipelineLayout(m_logicalDevice, m_finalComposite_PL, nullptr);
	// Rasterization Pipeline
	vkDestroyPipeline(m_logicalDevice, m_rasterization_P, nullptr);
	vkDestroyPipelineLayout(m_logicalDevice, m_rasterization_PL, nullptr);
}
void VulkanRendererBackend::cleanupRenderPassesAndFrameResources()
{
	// Destroy Depth Image Common to every render pass
	vkDestroyImage(m_logicalDevice, m_depth.image, nullptr);
	vkFreeMemory(m_logicalDevice, m_depth.memory, nullptr);
	vkDestroyImageView(m_logicalDevice, m_depth.view, nullptr);

	for (uint32_t i = 0; i < m_numSwapChainImages; i++)
	{
		// Destroy Framebuffers
		vkDestroyFramebuffer(m_logicalDevice, m_toDisplayRPI.frameBuffers[i], nullptr);
		vkDestroyFramebuffer(m_logicalDevice, m_rasterRPI.frameBuffers[i], nullptr);

		// Destroy the color attachments
		{
			// Don't need to do this for m_toDisplayRPI
			// m_rasterRPI.color
			vkDestroyImage(m_logicalDevice, m_rasterRPI.color[i].image, nullptr);
			vkDestroyImageView(m_logicalDevice, m_rasterRPI.color[i].view, nullptr);
			vkFreeMemory(m_logicalDevice, m_rasterRPI.color[i].memory, nullptr);
		}
	}

	// Destroy Samplers
	vkDestroySampler(m_logicalDevice, m_rasterRPI.sampler, nullptr);

	// Destroy Renderpasses
	vkDestroyRenderPass(m_logicalDevice, m_toDisplayRPI.renderPass, nullptr);
	vkDestroyRenderPass(m_logicalDevice, m_rasterRPI.renderPass, nullptr);
}

void VulkanRendererBackend::createPipelines(DescriptorSetLayouts& pipelineDescriptorSetLayouts)
{
	std::vector<VkDescriptorSetLayout>& compute_DSL = pipelineDescriptorSetLayouts.computeDSL;
	std::vector<VkDescriptorSetLayout>& finalComposite_DSL = pipelineDescriptorSetLayouts.finalCompositeDSL;
	std::vector<VkDescriptorSetLayout>& rasterization_DSL = pipelineDescriptorSetLayouts.geomDSL;

	m_compute_PL = VulkanPipelineCreation::createPipelineLayout(m_logicalDevice, compute_DSL, 0, nullptr);
	createComputePipeline(m_compute_P, m_compute_PL, "testComputeShader");

	createFinalCompositePipeline(finalComposite_DSL);
	createRasterizationRenderPipeline(rasterization_DSL);
}
void VulkanRendererBackend::createRenderPassesAndFrameResources()
{
	createRenderPasses();
	createDepthResources();
	createFrameBuffers();
	createPostProcessRenderPasses();
}


//===============================================================================================
//===============================================================================================
//===============================================================================================

// Descriptor Sets
void VulkanRendererBackend::expandDescriptorPool(std::vector<VkDescriptorPoolSize>& poolSizes)
{
	// Final Composite pass writes to the swapchain image(s) so doesnt need a separate desciptor for that
	// It does need descriptors for any images it reads in

	// FINAL COMPOSITE
	poolSizes.push_back({ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, m_numSwapChainImages }); // compute image
	poolSizes.push_back({ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, m_numSwapChainImages }); // geomRenderPass image
}
void VulkanRendererBackend::createDescriptorPool(std::vector<VkDescriptorPoolSize>& poolSizes)
{
	// There's a  difference between descriptor Sets and bindings inside a descriptor Set, 
	// multiple bindings can exist inside a descriptor set. So a pool can have fewer sets than number of bindings.
	const uint32_t numSets = 9;
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

		// FINAL COMPOSITE
		const uint32_t numImagesComposited = 2;
		VkDescriptorSetLayoutBinding computePassImageLB = { 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr };
		VkDescriptorSetLayoutBinding geomRenderedImageLB = { 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr };

		std::array<VkDescriptorSetLayoutBinding, numImagesComposited> finalCompositeLBs = { computePassImageLB, geomRenderedImageLB };
		DescriptorUtil::createDescriptorSetLayout(m_logicalDevice, m_DSL_finalComposite, numImagesComposited, finalCompositeLBs.data());
	}

	// Descriptor Sets
	{
		m_DS_finalComposite.resize(m_numSwapChainImages);
		for (uint32_t i = 0; i < m_numSwapChainImages; i++)
		{
			// Final Composite
			DescriptorUtil::createDescriptorSets(m_logicalDevice, descriptorPool, 1, &m_DSL_finalComposite, &m_DS_finalComposite[i]);
		}
	}
}
void VulkanRendererBackend::writeToAndUpdateDescriptorSets(DescriptorSetDependencies& descSetDependencies)
{
	const uint32_t numImagesComposited = 2;

	for (uint32_t i = 0; i < m_numSwapChainImages; i++)
	{
		// Final Composite
		{
			VkDescriptorImageInfo computePassImageSetInfo = DescriptorUtil::createDescriptorImageInfo(
				descSetDependencies.computeImages[i]->getSampler(),
				descSetDependencies.computeImages[i]->getImageView(),
				descSetDependencies.computeImages[i]->getImageLayout());
			VkDescriptorImageInfo geomRenderedImageSetInfo = DescriptorUtil::createDescriptorImageInfo(
				descSetDependencies.geomRenderPassImageSet[i].sampler,
				descSetDependencies.geomRenderPassImageSet[i].imageView,
				descSetDependencies.geomRenderPassImageSet[i].imageLayout);

			std::array<VkWriteDescriptorSet, numImagesComposited> writeFinalCompositeSetInfo = {};
			writeFinalCompositeSetInfo[0] = DescriptorUtil::writeDescriptorSet(
				m_DS_finalComposite[i], 0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &computePassImageSetInfo);
			writeFinalCompositeSetInfo[1] = DescriptorUtil::writeDescriptorSet(
				m_DS_finalComposite[i], 1, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &geomRenderedImageSetInfo);

			vkUpdateDescriptorSets(m_logicalDevice, numImagesComposited, writeFinalCompositeSetInfo.data(), 0, nullptr);
		}
	}
}

//===============================================================================================

// Render Passes and Frame Buffers
void VulkanRendererBackend::createRenderPasses()
{
	// https://vulkan-tutorial.com/Drawing_a_triangle/Graphics_pipeline_basics/Render_passes
	// A VkRenderPass object tells us the following things:
	// - the framebuffer attachments that will be used while rendering
	// - how many color and depth buffers there will be
	// - how many samples to use for each of them 
	// - and how their contents should be handled throughout the rendering operations

	// A single renderpass can consist of multiple subpasses. 
	// Subpasses are subsequent rendering operations that depend on the contents of framebuffers in previous passes, 
	// for example a sequence of post-processing effects that are applied one after another. If you group these 
	// rendering operations into one render pass, then Vulkan is able to reorder the operations and 
	// conserve memory bandwidth for possibly better performance. 

	// Indices for attachment descriptions are based on the ordering of their respective attachments. Similar indexing applies to attachment references

	// Subpass dependencies define layout transitions between subpasses -- Vulkan automatically handles these transitions

	VkFormat swapChainImageFormat = m_vulkanObj->getSwapChainImageFormat();
	VkFormat color32bitFormat = m_vulkanObj->getSwapChainImageFormat();

	// --- Raster Render Pass --- 
	// Renders Geometry to an offscreen buffer
	{
		const VkImageLayout finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		std::vector<VkSubpassDependency> subpassDependencies;
		{
			// Transition from tasks before this render pass (including runs through other pipelines before it, hence fragment shader bit)
			subpassDependencies.push_back(
				RenderPassUtil::subpassDependency(VK_SUBPASS_EXTERNAL, 0,
					VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT,
					VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT));

			//Transition from actual subpass to tasks after the renderpass -- to be read in the fragment shader of another render pass
			subpassDependencies.push_back(
				RenderPassUtil::subpassDependency(0, VK_SUBPASS_EXTERNAL,
					VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
					VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT));
		}
		RenderPassUtil::renderPassCreationHelper(m_logicalDevice, m_rasterRPI.renderPass,
			swapChainImageFormat, m_depthFormat, finalLayout, subpassDependencies);
	}

	createPostProcessRenderPasses();

	// --- To Display Render Pass --- 
	// Composites previous Passes and renders result onto swapchain. 
	// It is the last pass before the UI render pass, which also renders to the swapchain image.
	// Has to exist, other passes are optional
	{
		const VkImageLayout finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
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
		RenderPassUtil::renderPassCreationHelper(m_logicalDevice, m_toDisplayRPI.renderPass,
			swapChainImageFormat, m_depthFormat, finalLayout, subpassDependencies);
	}
}
void VulkanRendererBackend::createDepthResources()
{
	// At this point in time I'm only creating one depth image but to take advantage of more parallelization,
	// you may need to have as many depth images as there are swapchain images.
	// Reddit Discussion here: https://www.reddit.com/r/vulkan/comments/9at7rz/api_without_secrets_the_practical_approach_to/e542onj/

	FrameResourcesUtil::createDepthAttachment( m_logicalDevice, m_physicalDevice, 
		m_graphicsQueue, m_graphicsCommandPool, m_depth, m_depthFormat, m_windowExtents);
}
void VulkanRendererBackend::createFrameBuffers()
{
	// Framebuffers are essentially ImageView objects with more meta data. 
	// Framebuffers are not limited to use by the swapchain, and can also represent intermediate stages that store the results of renderpasses.

	// The attachments specified during render pass creation are bound by wrapping them into a VkFramebuffer object. 
	// A framebuffer object references all of the VkImageView objects that represent the attachments. 
	// Think color attachment and depth attachment 

	VkFormat swapChainImageFormat = m_vulkanObj->getSwapChainImageFormat();

	// To Display
	{
		// The framebuffers used for presentation are special and will be managed by the VulkanPresentation class, we just reference them here.
		m_toDisplayRPI.frameBuffers.resize(m_numSwapChainImages);
		m_toDisplayRPI.extents = m_windowExtents;

		const uint32_t numAttachments = 2;
		for (uint32_t i = 0; i < m_numSwapChainImages; i++)
		{
			// This one is different from the other render passes because it presents to the swap chain which has already created the
			// image, imageview, image memory, format, and sampler as part of the swapchain which we just reference here.

			std::array<VkImageView, numAttachments> attachments = { m_vulkanObj->getSwapChainImageView(i), m_depth.view };

			FrameResourcesUtil::createFrameBuffer(m_logicalDevice, 
				m_toDisplayRPI.frameBuffers[i], m_toDisplayRPI.renderPass, 
				m_windowExtents, numAttachments, attachments.data());
		}
	}
	// Render Geometry
	{
		m_rasterRPI.frameBuffers.resize(m_numSwapChainImages);
		m_rasterRPI.color.resize(m_numSwapChainImages);
		m_rasterRPI.imageSetInfo.resize(m_numSwapChainImages);
		m_rasterRPI.extents = m_windowExtents;

		FrameResourcesUtil::createFrameBufferAttachments(m_logicalDevice, m_physicalDevice,
			m_numSwapChainImages, m_rasterRPI.color, m_rasterRPI.sampler, swapChainImageFormat, m_windowExtents);

		for (uint32_t i = 0; i < m_numSwapChainImages; i++)
		{
			std::array<VkImageView, 2> attachments = { m_rasterRPI.color[i].view, m_depth.view };

			FrameResourcesUtil::createFrameBuffer(m_logicalDevice, m_rasterRPI.frameBuffers[i], m_rasterRPI.renderPass,
				m_windowExtents, static_cast<uint32_t>(attachments.size()), attachments.data());

			// Fill a descriptor for later use in a descriptor set 
			m_rasterRPI.imageSetInfo[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			m_rasterRPI.imageSetInfo[i].imageView = m_rasterRPI.color[i].view;
			m_rasterRPI.imageSetInfo[i].sampler = m_rasterRPI.sampler;
		}
	}
}

void VulkanRendererBackend::createPostProcessRenderPasses()
{
	// --- 32 Bit Post Process Passes --- 
	{
		// std::vector<VkSubpassDependency> &subpassDependencies
	}
	// --- 8 Bit Post Process Passes --- 
	{
		// std::vector<VkSubpassDependency> &subpassDependencies
	}
}

//===============================================================================================

// Pipelines
void VulkanRendererBackend::createComputePipeline(VkPipeline& computePipeline, VkPipelineLayout computePipelineLayout, const std::string &shaderName)
{
	// -------- Create Shader Stages -------------
	VkShaderModule compShaderModule;
	VkPipelineShaderStageCreateInfo compShaderStageInfo;
	ShaderUtil::createComputeShaderStageInfo(compShaderStageInfo, shaderName, compShaderModule, m_logicalDevice);

	VulkanPipelineCreation::createComputePipeline(m_logicalDevice, compShaderStageInfo, computePipeline, computePipelineLayout);

	// No need for the shader modules anymore, so we destory them!
	vkDestroyShaderModule(m_logicalDevice, compShaderModule, nullptr);
}
void VulkanRendererBackend::createFinalCompositePipeline(std::vector<VkDescriptorSetLayout>& compositeDSL)
{
	// -------- Create Pipeline Layout -------------
	m_finalComposite_PL = VulkanPipelineCreation::createPipelineLayout(m_logicalDevice, compositeDSL, 0, nullptr);

	// -------- Empty Vertex Input --------
	VkPipelineVertexInputStateCreateInfo vertexInput = VulkanPipelineStructures::vertexInputInfo(0, nullptr, 0, nullptr);

	// -------- Create Shader Stages -------------
	const uint32_t stageCount = 2;
	VkShaderModule vertShaderModule, fragShaderModule;
	std::vector<VkPipelineShaderStageCreateInfo> shaderStages; shaderStages.resize(2);

	ShaderUtil::createShaderStageInfos_RenderToQuad(shaderStages, "finalComposite", vertShaderModule, fragShaderModule, m_logicalDevice);

	// -------- Create graphics pipeline ---------	
	VulkanPipelineCreation::createGraphicsPipeline(m_logicalDevice, 
		m_finalComposite_P, m_finalComposite_PL, 
		m_toDisplayRPI.renderPass, 0,
		stageCount, shaderStages.data(), vertexInput, m_vulkanObj->getSwapChainVkExtent());

	// No need for the shader modules anymore, so we destory them!
	vkDestroyShaderModule(m_logicalDevice, vertShaderModule, nullptr);
	vkDestroyShaderModule(m_logicalDevice, fragShaderModule, nullptr);
}
void VulkanRendererBackend::createRasterizationRenderPipeline(std::vector<VkDescriptorSetLayout>& rasterizationDSL)
{
	// -------- Create Rasterization Layout -------------
	m_rasterization_PL = VulkanPipelineCreation::createPipelineLayout(m_logicalDevice, rasterizationDSL, 0, nullptr);

	// -------- Vertex Input --------
	// Vertex binding describes at which rate to load data from GPU memory 

	// All of our per-vertex data is packed together in 1 array so we only have one binding; 
	// The binding param specifies index of the binding in array of bindings
	const uint32_t numVertexAttributes = 4;
	VkVertexInputBindingDescription vertexInputBinding =
		VulkanPipelineStructures::vertexInputBindingDesc(0, static_cast<uint32_t>(sizeof(Vertex)));

	// Input attribute bindings describe shader attribute locations and memory layouts		
	std::array<VkVertexInputAttributeDescription, numVertexAttributes> vertexInputAttributes = Vertex::getAttributeDescriptions();

	VkPipelineVertexInputStateCreateInfo vertexInput =
		VulkanPipelineStructures::vertexInputInfo(1, &vertexInputBinding, numVertexAttributes, vertexInputAttributes.data());

	// -------- Create Shader Stages -------------
	const uint32_t stageCount = 2;
	VkShaderModule vertShaderModule, fragShaderModule;
	std::vector<VkPipelineShaderStageCreateInfo> shaderStages; shaderStages.resize(2);

	ShaderUtil::createShaderStageInfos(shaderStages, "geometryPlain", vertShaderModule, fragShaderModule, m_logicalDevice);

	// -------- Create graphics pipeline ---------	
	VulkanPipelineCreation::createGraphicsPipeline(m_logicalDevice,
		m_rasterization_P, m_rasterization_PL,
		m_rasterRPI.renderPass, 0,
		stageCount, shaderStages.data(), vertexInput, m_vulkanObj->getSwapChainVkExtent());

	// No need for the shader modules anymore, so we destory them!
	vkDestroyShaderModule(m_logicalDevice, vertShaderModule, nullptr);
	vkDestroyShaderModule(m_logicalDevice, fragShaderModule, nullptr);
}

//===============================================================================================

// Command Buffers
void VulkanRendererBackend::createCommandPoolsAndBuffers()
{
	// Commands in Vulkan, like drawing operations and memory transfers, are not executed directly using function calls. 
	// You have to record all of the operations you want to perform in command buffer objects. The advantage of this is 
	// that all of the hard work of setting up the drawing commands can be done in advance and in multiple threads.

	// Command buffers will be automatically freed when their command pool is destroyed, so we don't need an explicit cleanup.

	// Because one of the drawing commands involves binding the right VkFramebuffer, we'll actually have to 
	// record a command buffer for every image in the swap chain once again.

	VulkanCommandUtil::createCommandPool(m_logicalDevice, m_graphicsCommandPool, m_vulkanObj->getQueueIndex(QueueFlags::Graphics));
	VulkanCommandUtil::createCommandPool(m_logicalDevice, m_computeCommandPool, m_vulkanObj->getQueueIndex(QueueFlags::Compute));
	recreateCommandBuffers();
}
void VulkanRendererBackend::recreateCommandBuffers()
{
	m_graphicsCommandBuffers.resize(m_numSwapChainImages);
	m_computeCommandBuffers.resize(m_numSwapChainImages);
	VulkanCommandUtil::allocateCommandBuffers(m_logicalDevice, m_graphicsCommandPool, m_graphicsCommandBuffers);
	VulkanCommandUtil::allocateCommandBuffers(m_logicalDevice, m_computeCommandPool, m_computeCommandBuffers);
}
void VulkanRendererBackend::submitCommandBuffers()
{
	VkSemaphore waitSemaphores[] = { m_vulkanObj->getImageAvailableVkSemaphore() };
	// We want to wait with writing colors to the image until it's available, so we're specifying the stage of the graphics pipeline 
	// that writes to the color attachment. That means that theoretically the implementation can already start executing our vertex 
	// shader and such while the image is not available yet.
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	VkSemaphore signalSemaphores[] = { m_vulkanObj->getRenderFinishedVkSemaphore() };
	VkFence inFlightFence = m_vulkanObj->getInFlightFence();
	
	//	Submit Commands
	{
		// There is a fence attached to the submission of commands to the graphics queue. Since the fence is a CPU-GPU synchronization
		// it will wait  here until the frame is deemed available, meaning the other queues (in this case only the compute queue) 
		// dont have to create they're own fences that they need to keep track of.
		// The inflight fence basically tells us when a frame has finished rendering, and the submitToQueueSynced function 
		// signals the fence, informing it and thus us of the same.
		
		uint32_t index = m_vulkanObj->getIndex();
		VulkanCommandUtil::submitToQueue(m_computeQueue, 1, m_computeCommandBuffers[index]);
		VulkanCommandUtil::submitToQueueSynced(
			m_graphicsQueue, 1, &m_graphicsCommandBuffers[index],
			1, waitSemaphores, waitStages,
			1, signalSemaphores,
			inFlightFence);
	}
}

//===============================================================================================

// Getters
VkPipeline VulkanRendererBackend::getPipeline(PIPELINE_TYPE type)
{
	switch (type)
	{
	case PIPELINE_TYPE::RASTER:
		return m_rasterization_P;
		break;
	case PIPELINE_TYPE::FINAL_COMPOSITE:
		return m_finalComposite_P;
		break;
	case PIPELINE_TYPE::COMPUTE:
		return m_compute_P;
		break;
	case PIPELINE_TYPE::POST_PROCESS:
		return nullptr;
		break;
	default:
		throw std::runtime_error("no such Descriptor Set Layout Type (DSL_TYPE) exists");
	}

	return nullptr;
}
VkPipelineLayout VulkanRendererBackend::getPipelineLayout(PIPELINE_TYPE type)
{
	switch (type)
	{
	case PIPELINE_TYPE::RASTER:
		return m_rasterization_PL;
		break;
	case PIPELINE_TYPE::FINAL_COMPOSITE:
		return m_finalComposite_PL;
		break;
	case PIPELINE_TYPE::COMPUTE:
		return m_compute_PL;
		break;
	case PIPELINE_TYPE::POST_PROCESS:
		return nullptr;
		break;
	default:
		throw std::runtime_error("no such Descriptor Set Layout Type (DSL_TYPE) exists");
	}

	return nullptr;
}
VkDescriptorSet VulkanRendererBackend::getDescriptorSet(DSL_TYPE type, int index)
{
	switch (type)
	{
	case DSL_TYPE::FINAL_COMPOSITE:
		return m_DS_finalComposite[index];
		break;
	default:
		throw std::runtime_error("no such Descriptor Set Layout Type (DSL_TYPE) exists");
	}

	return nullptr;
}
VkDescriptorSetLayout VulkanRendererBackend::getDescriptorSetLayout(DSL_TYPE key)
{
	switch (key)
	{
	case DSL_TYPE::FINAL_COMPOSITE:
		return m_DSL_finalComposite;
		break;
	default:
		throw std::runtime_error("no such Descriptor Set Layout Type (DSL_TYPE) exists");
	}

	assert(("Did not find a Descriptor Set Layout to return", true));
	return nullptr;
}
