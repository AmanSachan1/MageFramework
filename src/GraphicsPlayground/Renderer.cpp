#include "Renderer.h"

Renderer::Renderer(GLFWwindow* window, RendererOptions rendererOptions, VulkanDevices* devices, VulkanPresentation* presentation,
	Camera* camera, uint32_t width, uint32_t height)
	: m_window(window), 
	m_devices(devices), m_logicalDevice(devices->getLogicalDevice()), m_physicalDevice(devices->getPhysicalDevice()),
	m_presentationObject(presentation),
	m_camera(camera),
	m_windowWidth(width),
	m_windowHeight(height)
{
	initialize();
}
Renderer::~Renderer()
{
	cleanup();

	// Models
	delete m_scene;

	// Command Pools
	vkDestroyCommandPool(m_logicalDevice, m_graphicsCommandPool, nullptr);
	vkDestroyCommandPool(m_logicalDevice, m_computeCommandPool, nullptr);
}

void Renderer::initialize()
{
	m_graphicsQueue = m_devices->getQueue(QueueFlags::Graphics);
	m_computeQueue = m_devices->getQueue(QueueFlags::Compute);
	createCommandPoolsAndBuffers();
	createRenderPass();

	m_scene = new Scene(m_devices, m_presentationObject->getCount(), m_windowWidth, m_windowHeight,
					   m_graphicsQueue, m_graphicsCommandPool, m_computeQueue, m_computeCommandPool);
	m_scene->createScene();

	setupDescriptorSets();
	setupMSAA();
	createDepthResources();
	createAllPipelines();
	createFrameBuffers();
	recordAllCommandBuffers();
}
void Renderer::renderLoop()
{
	//compute queue first
	VkCommandBuffer* computeCommandBuffer = &m_computeCommandBuffers[m_presentationObject->getIndex()];
	VulkanCommandUtil::submitToQueueSynced(m_computeQueue, 1, computeCommandBuffer,
		0, nullptr, nullptr,
		0, nullptr, nullptr);


	// Wait for the the frame to be finished before working on it
	VkFence inFlightFence = m_presentationObject->getInFlightFence();

	// The VK_TRUE we pass in vkWaitForFences indicates that we want to wait for all fences.
	vkWaitForFences(m_logicalDevice, 1, &inFlightFence, VK_TRUE, std::numeric_limits<uint64_t>::max());

	// Acquire image from swapchain
	bool flag_recreateSwapChain = m_presentationObject->acquireNextSwapChainImage(m_logicalDevice);
	if (!flag_recreateSwapChain) { recreate(); };	

	m_camera->updateUniformBuffer(m_presentationObject->getIndex());
	m_scene->updateUniforms(m_presentationObject->getIndex());

	//-------------------------------------
	//	Submit Commands To Graphics Queue
	//-------------------------------------
	VkSemaphore waitSemaphores[] = { m_presentationObject->getImageAvailableVkSemaphore() };
	// We want to wait with writing colors to the image until it's available, so we're specifying the stage of the graphics pipeline 
	// that writes to the color attachment. That means that theoretically the implementation can already start executing our vertex 
	// shader and such while the image is not available yet.
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	VkSemaphore signalSemaphores[] = { m_presentationObject->getRenderFinishedVkSemaphore() };	
	VkCommandBuffer* graphicsCommandBuffer = &m_graphicsCommandBuffers[m_presentationObject->getIndex()];

	vkResetFences(m_logicalDevice, 1, &inFlightFence);
	VulkanCommandUtil::submitToQueueSynced(m_graphicsQueue, 1, graphicsCommandBuffer, 1, waitSemaphores, waitStages, 1, signalSemaphores, inFlightFence);

	// Return the image to the swapchain for presentation
	flag_recreateSwapChain = m_presentationObject->presentImageToSwapChain(m_logicalDevice, m_resizeFrameBuffer);
	if (!flag_recreateSwapChain) { recreate(); };

	m_presentationObject->advanceCurrentFrameIndex();
}

void Renderer::recreate()
{
	m_resizeFrameBuffer = false;
	// This while loop handles the case of minimization of the window
	int width = 0, height = 0;
	while (width == 0 || height == 0) 
	{
		glfwGetFramebufferSize(m_window, &width, &height);
		glfwWaitEvents();
	}

	cleanup();

	m_presentationObject->create(m_window);
	createRenderPass();
	setupDescriptorSets();

	setupMSAA();
	createDepthResources();
	
	createAllPipelines();
	createFrameBuffers();

	m_graphicsCommandBuffers.resize(m_presentationObject->getCount());
	m_computeCommandBuffers.resize(m_presentationObject->getCount());
	VulkanCommandUtil::allocateCommandBuffers(m_logicalDevice, m_graphicsCommandPool, m_graphicsCommandBuffers);
	VulkanCommandUtil::allocateCommandBuffers(m_logicalDevice, m_computeCommandPool, m_computeCommandBuffers);
	recordAllCommandBuffers();

	m_resizeFrameBuffer = false;
}
void Renderer::cleanup()
{
	vkDeviceWaitIdle(m_logicalDevice);
	
	vkDestroyImage(m_logicalDevice, m_depthImage, nullptr);
	vkFreeMemory(m_logicalDevice, m_depthImageMemory, nullptr);
	vkDestroyImageView(m_logicalDevice, m_depthImageView, nullptr);

	vkDestroyImage(m_logicalDevice, m_MSAAcolorImage, nullptr);
	vkFreeMemory(m_logicalDevice, m_MSAAcolorImageMemory, nullptr);
	vkDestroyImageView(m_logicalDevice, m_MSAAcolorImageView, nullptr);

	for (size_t i = 0; i < m_frameBuffers.size(); i++)
	{
		vkDestroyFramebuffer(m_logicalDevice, m_frameBuffers[i], nullptr);
	}

	vkFreeCommandBuffers(m_logicalDevice, m_graphicsCommandPool, static_cast<uint32_t>(m_graphicsCommandBuffers.size()), m_graphicsCommandBuffers.data());
	vkFreeCommandBuffers(m_logicalDevice, m_computeCommandPool, static_cast<uint32_t>(m_computeCommandBuffers.size()), m_computeCommandBuffers.data());
	
	vkDestroyPipeline(m_logicalDevice, m_graphicsPipeline, nullptr);
	vkDestroyPipelineLayout(m_logicalDevice, m_graphicsPipelineLayout, nullptr);

	vkDestroyPipeline(m_logicalDevice, m_computePipeline, nullptr);
	vkDestroyPipelineLayout(m_logicalDevice, m_computePipelineLayout, nullptr);
	
	//vkDestroyPipeline(m_logicalDevice, m_postProcess_ToneMap_PipeLine, nullptr);
	vkDestroyPipelineLayout(m_logicalDevice, m_postProcess_ToneMap_PipelineLayout, nullptr);
	//vkDestroyPipeline(m_logicalDevice, m_postProcess_TXAA_PipeLine, nullptr);
	vkDestroyPipelineLayout(m_logicalDevice, m_postProcess_TXAA_PipelineLayout, nullptr);
	
	vkDestroyRenderPass(m_logicalDevice, m_renderPass, nullptr);
	
	m_presentationObject->cleanup();

	// Descriptors
	//Descriptor sets are automatically deallocated when the descriptor pool is destroyed
	vkDestroyDescriptorSetLayout(m_logicalDevice, m_DSL_graphics, nullptr);
	vkDestroyDescriptorSetLayout(m_logicalDevice, m_DSL_compute, nullptr);
	//vkDestroyDescriptorSetLayout(m_logicalDevice, m_DSL_toneMap, nullptr);
	//vkDestroyDescriptorSetLayout(m_logicalDevice, m_DSL_TXAA, nullptr);

	vkDestroyDescriptorPool(m_logicalDevice, descriptorPool, nullptr);
}

void Renderer::recordAllCommandBuffers()
{
	for (unsigned int i = 0; i < m_graphicsCommandBuffers.size(); ++i)
	{
		VulkanCommandUtil::beginCommandBuffer(m_graphicsCommandBuffers[i]);
		recordGraphicsCommandBuffer(m_graphicsCommandBuffers[i], m_frameBuffers[i], i);
		VulkanCommandUtil::endCommandBuffer(m_graphicsCommandBuffers[i]);
	}

	for (unsigned int i = 0; i < m_computeCommandBuffers.size(); ++i)
	{
		VulkanCommandUtil::beginCommandBuffer(m_computeCommandBuffers[i]);
		recordComputeCommandBuffer(m_computeCommandBuffers[i], i);
		VulkanCommandUtil::endCommandBuffer(m_computeCommandBuffers[i]);
	}
}
void Renderer::recordGraphicsCommandBuffer(VkCommandBuffer& graphicsCmdBuffer, VkFramebuffer& frameBuffer, unsigned int frameIndex)
{
	std::array<VkClearValue, 2> clearValues = {};
	clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
	clearValues[1].depthStencil = { 1.0f, 0 };

	const VkRect2D renderArea = Util::createRectangle({ 0,0 }, m_presentationObject->getVkExtent());
	VulkanCommandUtil::beginRenderPass(graphicsCmdBuffer, m_renderPass, frameBuffer, renderArea, static_cast<uint32_t>(clearValues.size()), clearValues.data());
	vkCmdBindPipeline(graphicsCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);

	Model* model = m_scene->getModel("house");

	VkBuffer vertexBuffers[] = { model->getVertexBuffer() };
	VkBuffer indexBuffer = model->getIndexBuffer();
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(graphicsCmdBuffer, 0, 1, vertexBuffers, offsets);
	vkCmdBindIndexBuffer(graphicsCmdBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);

	vkCmdBindDescriptorSets(graphicsCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipelineLayout, 0, 1, &m_DS_graphics[frameIndex], 0, nullptr);

	// Draw Command Params, aside from the command buffer:
	// indexCount    : Number of indices
	// instanceCount : Number of instances
	// firstIndex    : Offset into the indexBuffer. Since it is a standalone buffer for us this is zero.
	// VertexOffset  : Used to specify an offset to add to the indices of the index buffer
	// firstInstance : Used as an offset for instanced rendering, defines the lowest value of gl_InstanceIndex.
	vkCmdDrawIndexed(graphicsCmdBuffer, model->getNumIndices(), 1, 0, 0, 0);

	vkCmdEndRenderPass(graphicsCmdBuffer);
}
void Renderer::recordComputeCommandBuffer(VkCommandBuffer& ComputeCmdBuffer, unsigned int frameIndex)
{
	// get compute texture
	std::string name = "compute" + std::to_string(frameIndex);
	Texture* texture = m_scene->getTexture(name);
	const uint32_t numBlocksX = (texture->getWidth() + WORKGROUP_SIZE - 1) / WORKGROUP_SIZE;
	const uint32_t numBlocksY = (texture->getHeight() + WORKGROUP_SIZE - 1) / WORKGROUP_SIZE;
	const uint32_t numBlocksZ = 1;
	
	vkCmdBindPipeline(ComputeCmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_computePipeline);

	//Bind Descriptor Sets for compute
	vkCmdBindDescriptorSets(ComputeCmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_computePipelineLayout, 0, 1, &m_DS_compute[frameIndex], 0, nullptr);

	// Dispatch the compute kernel, with number of threads =  numBlocksX * numBlocksY * numBlocksZ
	vkCmdDispatch(ComputeCmdBuffer, numBlocksX, numBlocksY, numBlocksZ);
}
void Renderer::createCommandPoolsAndBuffers()
{
	// Commands in Vulkan, like drawing operations and memory transfers, are not executed directly using function calls. 
	// You have to record all of the operations you want to perform in command buffer objects. The advantage of this is 
	// that all of the hard work of setting up the drawing commands can be done in advance and in multiple threads.

	// Command buffers will be automatically freed when their command pool is destroyed, so we don't need an explicit cleanup.

	// Because one of the drawing commands involves binding the right VkFramebuffer, we'll actually have to 
	// record a command buffer for every image in the swap chain once again.
	VulkanCommandUtil::createCommandPool(m_logicalDevice, m_graphicsCommandPool, m_devices->getQueueIndex(QueueFlags::Graphics));
	VulkanCommandUtil::createCommandPool(m_logicalDevice, m_computeCommandPool, m_devices->getQueueIndex(QueueFlags::Compute));

	m_graphicsCommandBuffers.resize(m_presentationObject->getCount());
	m_computeCommandBuffers.resize(m_presentationObject->getCount());

	VulkanCommandUtil::allocateCommandBuffers(m_logicalDevice, m_graphicsCommandPool, m_graphicsCommandBuffers);
	VulkanCommandUtil::allocateCommandBuffers(m_logicalDevice, m_computeCommandPool, m_computeCommandBuffers);
}

void Renderer::createFrameBuffers()
{
	// The attachments specified during render pass creation are bound by wrapping them into a VkFramebuffer object. 
	// A framebuffer object references all of the VkImageView objects that represent the attachments. 
	// In our case that will be only a single one: the color attachment. 
	// The image that we have to use for the attachment depends on which image the swap chain returns when we retrieve one for presentation. 
	// That means that we have to create a framebuffer for all of the images in the swap chain and use the one that corresponds to 
	// the retrieved image at drawing time.

	m_frameBuffers.resize(m_presentationObject->getCount());

	for (uint32_t i = 0; i < m_presentationObject->getCount(); i++)
	{
		std::array<VkImageView, 3> attachments[] = { m_MSAAcolorImageView, m_depthImageView, m_presentationObject->getVkImageView(i)  };

		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = m_renderPass;
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments->size());
		framebufferInfo.pAttachments = attachments->data();
		framebufferInfo.width = m_presentationObject->getVkExtent().width;
		framebufferInfo.height = m_presentationObject->getVkExtent().height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(m_logicalDevice, &framebufferInfo, nullptr, &m_frameBuffers[i]) != VK_SUCCESS) 
		{
			throw std::runtime_error("failed to create framebuffer!");
		}
	}
}
void Renderer::createRenderPass()
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

	VkAttachmentReference colorAttachmentRef, depthAttachmentRef, colorAttachmentResolveRef;
	VkAttachmentDescription colorAttachment, depthAttachment, colorAttachmentResolve;
	{
		VkFormat swapChainImageFormat = m_presentationObject->getSwapChainImageFormat();
		VkFormat depthFormat = FormatUtil::findDepthFormat(m_physicalDevice);

		// FinalLayout for the color attachment is VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL because
		// multisampled images cannot be presented directly. We first need to resolve them to a regular image. 
		// This requirement does not apply to the depth buffer, since it won't be presented at any point.
		colorAttachment = RenderPassUtil::attachmentDescription(swapChainImageFormat, m_devices->getNumMSAASamples(),
				VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, //color data
				VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE, //stencil data
				VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		depthAttachment = RenderPassUtil::attachmentDescription(depthFormat, m_devices->getNumMSAASamples(),
				VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_DONT_CARE, //depth data
				VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE, //stencil data
				VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

		colorAttachmentResolve = RenderPassUtil::attachmentDescription(swapChainImageFormat, VK_SAMPLE_COUNT_1_BIT,
			VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_STORE, //color data
			VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_STORE, //stencil data
			VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

		// Our attachments array consists of a single VkAttachmentDescription, so its index is 0. 
		colorAttachmentRef = RenderPassUtil::attachmentReference(0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		depthAttachmentRef = RenderPassUtil::attachmentReference(1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
		colorAttachmentResolveRef = RenderPassUtil::attachmentReference(2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	}
	std::array<VkAttachmentDescription, 3> attachments = { colorAttachment, depthAttachment, colorAttachmentResolve };

	// The index of the color attachment in the color Attachment array is directly referenced from the fragment shader with the
	// layout(location = 0) out vec4 outColor directive!
	VkSubpassDescription subpass = 
		RenderPassUtil::subpassDescription(VK_PIPELINE_BIND_POINT_GRAPHICS, 0, nullptr, 
			1, &colorAttachmentRef, 
			&colorAttachmentResolveRef,
			&depthAttachmentRef,
			0, nullptr);

	VkSubpassDependency dependency = 
		RenderPassUtil::subpassDependency(VK_SUBPASS_EXTERNAL, 0,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);
	
	RenderPassUtil::createRenderPass(m_logicalDevice, m_renderPass, 
		static_cast<uint32_t>(attachments.size()), attachments.data(), 
		1, &subpass, 1, &dependency);
}

void Renderer::setupDescriptorSets()
{
	int numSwapChainImages = m_presentationObject->getCount();
	
	// --- Create Descriptor Pool ---
	{
		// There's a  difference between descriptor Sets and bindings inside a descriptor Set, 
		// multiple bindings can exist inside a descriptor set. So a pool can have fewer sets than number of bindings.
		int numSets = 0;
		// Graphics Model Set
		numSets++;
		std::array<VkDescriptorPoolSize, 5> poolSizes = {};
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[0].descriptorCount = static_cast<uint32_t>(numSwapChainImages);
		poolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[1].descriptorCount = static_cast<uint32_t>(numSwapChainImages);
		poolSizes[2].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[2].descriptorCount = static_cast<uint32_t>(numSwapChainImages);
		poolSizes[3].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[3].descriptorCount = static_cast<uint32_t>(numSwapChainImages);
		// Compute
		numSets++;
		poolSizes[4].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		poolSizes[4].descriptorCount = static_cast<uint32_t>(numSwapChainImages);

		const uint32_t maxSets = static_cast<uint32_t>(numSets * numSwapChainImages);
		const uint32_t poolSizesCount = static_cast<uint32_t>(poolSizes.size());

		DescriptorUtil::createDescriptorPool(m_logicalDevice, maxSets, poolSizesCount, poolSizes.data(), descriptorPool);
	}

	// --- Create Descriptor Set Layouts ---
	{
		// Descriptor set layouts are specified in the pipeline layout object., i.e. during pipeline creation to tell Vulkan 
		// which descriptors the shaders will be using.
		// The numbers are bindingCount, binding, and descriptorCount respectively
		VkDescriptorSetLayoutBinding modelLayoutBinding   = { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_ALL, nullptr };
		VkDescriptorSetLayoutBinding cameraLayoutBinding  = { 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_ALL, nullptr };
		VkDescriptorSetLayoutBinding samplerLayoutBinding = { 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr };
		VkDescriptorSetLayoutBinding readComputeLayoutBinding = { 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr };
		std::array<VkDescriptorSetLayoutBinding, 4> graphicsBindings = { modelLayoutBinding, cameraLayoutBinding, samplerLayoutBinding, readComputeLayoutBinding };

		DescriptorUtil::createDescriptorSetLayout(m_logicalDevice, m_DSL_graphics, static_cast<uint32_t>(graphicsBindings.size()), graphicsBindings.data());
	
		VkDescriptorSetLayoutBinding computeLayoutBinding = { 0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr };
		std::array<VkDescriptorSetLayoutBinding, 1> computeBindings = { computeLayoutBinding };

		DescriptorUtil::createDescriptorSetLayout(m_logicalDevice, m_DSL_compute, static_cast<uint32_t>(computeBindings.size()), computeBindings.data());
	}

	// --- Create Descriptor Sets ---
	{
		m_DS_graphics.resize(numSwapChainImages);
		m_DS_compute.resize(numSwapChainImages);
		std::vector<VkDescriptorSetLayout> DSLs_graphics(numSwapChainImages, m_DSL_graphics);		
		std::vector<VkDescriptorSetLayout> DSLs_compute(numSwapChainImages, m_DSL_compute);

		for (int i = 0; i < numSwapChainImages; i++)
		{
			DescriptorUtil::createDescriptorSets(m_logicalDevice, descriptorPool, 1, &DSLs_graphics[i], &m_DS_graphics[i]);

			std::array<VkWriteDescriptorSet, 4> writeGraphicsSetInfo = {};
			VkDescriptorBufferInfo modelBufferSetInfo, cameraBufferSetInfo;
			VkDescriptorImageInfo samplerImageSetInfo, readComputeSamplerImageSetInfo;

			// Model
			Model* model = m_scene->getModel("house");
			Texture* modelTexture = model->getTexture();

			std::string name = "compute" + std::to_string(i);
			Texture* computeTexture = m_scene->getTexture(name);

			modelBufferSetInfo = DescriptorUtil::createDescriptorBufferInfo(model->getUniformBuffer(i), 0, model->getUniformBufferSize());
			cameraBufferSetInfo = DescriptorUtil::createDescriptorBufferInfo(m_camera->getUniformBuffer(i), 0, m_camera->getUniformBufferSize());
			samplerImageSetInfo = DescriptorUtil::createDescriptorImageInfo(
					modelTexture->getSampler(), 
					modelTexture->getImageView(), 
					modelTexture->getImageLayout());
			readComputeSamplerImageSetInfo = DescriptorUtil::createDescriptorImageInfo(
				computeTexture->getSampler(), 
				computeTexture->getImageView(), 
				computeTexture->getImageLayout());;

			writeGraphicsSetInfo[0] = DescriptorUtil::writeDescriptorSet(m_DS_graphics[i], 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &modelBufferSetInfo);
			writeGraphicsSetInfo[1] = DescriptorUtil::writeDescriptorSet(m_DS_graphics[i], 1, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &cameraBufferSetInfo);
			writeGraphicsSetInfo[2] = DescriptorUtil::writeDescriptorSet(m_DS_graphics[i], 2, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &samplerImageSetInfo);
			writeGraphicsSetInfo[3] = DescriptorUtil::writeDescriptorSet(m_DS_graphics[i], 3, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &readComputeSamplerImageSetInfo);

			vkUpdateDescriptorSets(m_logicalDevice, static_cast<uint32_t>(writeGraphicsSetInfo.size()), writeGraphicsSetInfo.data(), 0, nullptr);
		}

		for (int i = 0; i < numSwapChainImages; i++)
		{
			// Compute
			std::string name = "compute" + std::to_string(i);
			Texture* computeTexture = m_scene->getTexture(name);
			DescriptorUtil::createDescriptorSets(m_logicalDevice, descriptorPool, 1, &DSLs_compute[i], &m_DS_compute[i]);
			std::array<VkWriteDescriptorSet, 1> writeComputeSetInfo = {};
			VkDescriptorImageInfo computeSetInfo;

			computeSetInfo = DescriptorUtil::createDescriptorImageInfo(computeTexture->getSampler(), computeTexture->getImageView(), computeTexture->getImageLayout());
			writeComputeSetInfo[0] = DescriptorUtil::writeDescriptorSet(m_DS_compute[i], 0, 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, &computeSetInfo);

			vkUpdateDescriptorSets(m_logicalDevice, static_cast<uint32_t>(writeComputeSetInfo.size()), writeComputeSetInfo.data(), 0, nullptr);
		}
	}
}

void Renderer::createDepthResources()
{
	VkExtent2D extent = m_presentationObject->getVkExtent();
	uint32_t width	= extent.width;
	uint32_t height	= extent.height;
	VkFormat depthFormat = FormatUtil::findDepthFormat(m_physicalDevice);

	ImageUtil::createImage(m_logicalDevice, m_physicalDevice, m_depthImage, m_depthImageMemory,
		VK_IMAGE_TYPE_2D, depthFormat, width, height, 1,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		m_devices->getNumMSAASamples(),
		VK_IMAGE_TILING_OPTIMAL,
		1, 1, VK_IMAGE_LAYOUT_UNDEFINED, VK_SHARING_MODE_EXCLUSIVE);

	ImageUtil::createImageView(m_logicalDevice, m_depthImage, &m_depthImageView, 
		VK_IMAGE_VIEW_TYPE_2D, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1, nullptr);

	ImageUtil::transitionImageLayout(m_logicalDevice, m_graphicsQueue, m_graphicsCommandPool,
		m_depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1);
}

void Renderer::setupMSAA()
{
	// MSAA does not solving potential problems caused by shader aliasing, 
	// i.e. MSAA only smoothens out the edges of geometry but not the interior filling. 
	// This may lead to a situation when you get a smooth polygon rendered on screen but the applied texture will still look aliased 
	// if it contains high contrasting colors.

	// Create a multisampled color buffer
	// Images with more than one sample per pixel can only have one mip level -- enforced by the Vulkan specification 
	VkExtent2D extent = m_presentationObject->getVkExtent();
	uint32_t width = extent.width;
	uint32_t height = extent.height;
	VkFormat colorFormat = m_presentationObject->getSwapChainImageFormat();

	ImageUtil::createImage(m_logicalDevice, m_physicalDevice, m_MSAAcolorImage, m_MSAAcolorImageMemory,
		VK_IMAGE_TYPE_2D, colorFormat, width, height, 1,
		VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		m_devices->getNumMSAASamples(),
		VK_IMAGE_TILING_OPTIMAL,
		1, 1, VK_IMAGE_LAYOUT_UNDEFINED, VK_SHARING_MODE_EXCLUSIVE);

	ImageUtil::createImageView(m_logicalDevice, m_MSAAcolorImage, &m_MSAAcolorImageView,
		VK_IMAGE_VIEW_TYPE_2D, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1, nullptr);

	ImageUtil::transitionImageLayout(m_logicalDevice, m_graphicsQueue, m_graphicsCommandPool,
		m_MSAAcolorImage, colorFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 1);
}

void Renderer::createAllPipelines()
{
	std::vector<VkDescriptorSetLayout> graphicsDSL = { m_DSL_graphics };
	std::vector<VkDescriptorSetLayout> computeDSL = { m_DSL_compute };

	m_graphicsPipelineLayout = VulkanPipelineCreation::createPipelineLayout(m_logicalDevice, graphicsDSL, 0, nullptr);
	m_computePipelineLayout = VulkanPipelineCreation::createPipelineLayout(m_logicalDevice, computeDSL, 0, nullptr);
	m_postProcess_ToneMap_PipelineLayout = VulkanPipelineCreation::createPipelineLayout(m_logicalDevice, graphicsDSL, 0, nullptr);
	m_postProcess_TXAA_PipelineLayout = VulkanPipelineCreation::createPipelineLayout(m_logicalDevice, graphicsDSL, 0, nullptr);

	createGraphicsPipeline(m_graphicsPipeline, m_graphicsPipelineLayout, m_renderPass, 0);
	createComputePipeline(m_computePipeline, m_computePipelineLayout, "GraphicsPlayground/shaders/testComputeShader.comp.spv");
	//createPostProcessPipelines( m_renderPass, 0);
}
void Renderer::createGraphicsPipeline(VkPipeline& graphicsPipeline, VkPipelineLayout graphicsPipelineLayout, VkRenderPass& renderPass, unsigned int subpassIndex)
{
	//--------------------------------------------------------
	//---------------- Set up shader stages ------------------
	//--------------------------------------------------------
	// Reference: https://vulkan-tutorial.com/Drawing_a_triangle/Graphics_pipeline_basics/Shader_modules
	// Create vert and frag shader modules
	VkShaderModule vertShaderModule = ShaderUtil::createShaderModule("GraphicsPlayground/shaders/testShader.vert.spv", m_logicalDevice);
	VkShaderModule fragShaderModule = ShaderUtil::createShaderModule("GraphicsPlayground/shaders/testShader.frag.spv", m_logicalDevice);

	// Assign each shader module to the appropriate stage in the pipeline
	VkPipelineShaderStageCreateInfo vertShaderStageInfo =
		ShaderUtil::shaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vertShaderModule, "main");
	VkPipelineShaderStageCreateInfo fragShaderStageInfo =
		ShaderUtil::shaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fragShaderModule, "main");

	//Add Shadermodules to the list of shader stages
	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

	//--------------------------------------------------------
	//------------- Set up fixed-function stages -------------
	//--------------------------------------------------------
	// Reference: https://vulkan-tutorial.com/Drawing_a_triangle/Graphics_pipeline_basics/Fixed_functions

	// -------- Vertex input binding --------
	// Vertex binding describes at which rate to load data from GPU memory 

	// All of our per-vertex data is packed together in 1 array so we only have one binding; 
	// The binding param specifies index of the binding in array of bindings
	VkVertexInputBindingDescription vertexInputBinding = VulkanPipelineStructures::vertexInputBindingDesc(0, sizeof(Vertex));

	// Input attribute bindings describe shader attribute locations and memory layouts
	std::array<VkVertexInputAttributeDescription, 4> vertexInputAttributes = Vertex::getAttributeDescriptions();

	// -------- Vertex input --------
	VkPipelineVertexInputStateCreateInfo vertexInput =
		VulkanPipelineStructures::vertexInputInfo(static_cast<uint32_t>(1),
												&vertexInputBinding, 
												static_cast<uint32_t>(vertexInputAttributes.size()),
												vertexInputAttributes.data());

	// -------- Input assembly --------
	// The VkPipelineInputAssemblyStateCreateInfo struct describes two things: what kind of geometry will be drawn 
	// from the vertices and if primitive restart should be enabled.

	VkPipelineInputAssemblyStateCreateInfo inputAssembly =
		VulkanPipelineStructures::inputAssemblyStateCreationInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FALSE);

	// -------- Tesselation State ---------
	// Set up Tesselation state here


	// -------- Viewport State ---------
	// Viewports and Scissors (rectangles that define in which regions pixels are stored)
	VkViewport viewport = Util::createViewport(
		0.0f, 0.0f,
		static_cast<float>(m_presentationObject->getVkExtent().width),
		static_cast<float>(m_presentationObject->getVkExtent().height),
		0.0f, 1.0f);

	// While viewports define the transformation from the image to the framebuffer, 
	// scissor rectangles define in which regions pixels will actually be stored.
	// we simply want to draw to the entire framebuffer, so we'll specify a scissor rectangle that covers the framebuffer entirely
	VkRect2D scissor = Util::createRectangle({ 0,0 }, m_presentationObject->getVkExtent());

	// Now this viewport and scissor rectangle need to be combined into a viewport state. It is possible to use 
	// multiple viewports and scissor rectangles. Using multiple requires enabling a GPU feature.
	VkPipelineViewportStateCreateInfo viewportState =
		VulkanPipelineStructures::viewportStateCreationInfo(1, &viewport, 1, &scissor);

	// -------- Rasterize --------
	// -- The rasterizer takes the geometry that is shaped by the vertices from the vertex shader and turns
	// it into fragments to be colored by the fragment shader.
	// -- It also performs depth testing, face culling and the scissor test, and it can be configured to output 
	// fragments that fill entire polygons or just the edges (wireframe rendering).
	// -- If rasterizerDiscardEnable is set to VK_TRUE, then geometry never passes through the rasterizer stage. 
	// This basically disables any output to the framebuffer.
	// -- depthBiasEnable: The rasterizer can alter the depth values by adding a constant value or biasing 
	// them based on a fragment's slope. This is sometimes used for shadow mapping.
	VkPipelineRasterizationStateCreateInfo rasterizer =
		VulkanPipelineStructures::rasterizerCreationInfo(VK_FALSE, VK_FALSE, VK_POLYGON_MODE_FILL, 1.0f,
			VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, VK_FALSE, 0.0f, 0.0f, 0.0f);

	// -------- Multisampling --------
	// (turned off here)
	VkPipelineMultisampleStateCreateInfo multisampling =
		VulkanPipelineStructures::multiSampleStateCreationInfo(m_devices->getNumMSAASamples(), VK_TRUE, 1.0f, nullptr, VK_FALSE, VK_FALSE);

	// -------- Depth and Stencil Testing --------
	VkPipelineDepthStencilStateCreateInfo depthAndStencil =
		VulkanPipelineStructures::depthStencilStateCreationInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS, VK_FALSE, 0.0f, 1.0f, VK_FALSE, {}, {});

	// -------- Color Blending ---------
	VkPipelineColorBlendAttachmentState colorBlendAttachment =
		VulkanPipelineStructures::colorBlendAttachmentStateInfo(
			VK_FALSE, VK_BLEND_OP_ADD, VK_BLEND_OP_ADD,
			VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
			VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
			VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO);

	// Global color blending settings 
	// --> set using colorBlendAttachment (add multiple attachments for multiple framebuffers with different blend settings)
	VkPipelineColorBlendStateCreateInfo colorBlending =
		VulkanPipelineStructures::colorBlendStateCreationInfo(VK_FALSE, VK_LOGIC_OP_COPY, 1, &colorBlendAttachment, 0.0f, 0.0f, 0.0f, 0.0f);

	// -------- Dynamic States ---------
	// Set up dynamic states here

	// -------- Create graphics pipeline ---------
	VkGraphicsPipelineCreateInfo graphicsPipelineInfo = 
		VulkanPipelineStructures::graphicsPipelineCreationInfo(
			2, shaderStages,
			&vertexInput,
			&inputAssembly,
			nullptr, //tessellation
			&viewportState,
			&rasterizer,
			&multisampling,
			&depthAndStencil,
			&colorBlending,
			nullptr, //dynamicState
			m_graphicsPipelineLayout,
			renderPass, subpassIndex,
			VK_NULL_HANDLE, -1);

	VulkanPipelineCreation::createGraphicsPipelines(m_logicalDevice, VK_NULL_HANDLE, 1, &graphicsPipelineInfo, &graphicsPipeline);

	// No need for the shader modules anymore, so we destory them!
	vkDestroyShaderModule(m_logicalDevice, vertShaderModule, nullptr);
	vkDestroyShaderModule(m_logicalDevice, fragShaderModule, nullptr);
}
void Renderer::createComputePipeline(VkPipeline& computePipeline, VkPipelineLayout computePipelineLayout, const std::string &pathToShader)
{
	VkShaderModule compShaderModule = ShaderUtil::createShaderModule(pathToShader, m_logicalDevice);
	
	VkPipelineShaderStageCreateInfo compShaderStageInfo = {};
	compShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	compShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	compShaderStageInfo.module = compShaderModule;
	compShaderStageInfo.pName = "main";

	VkComputePipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	pipelineInfo.stage = compShaderStageInfo;
	pipelineInfo.layout = computePipelineLayout;

	if (vkCreateComputePipelines(m_logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &computePipeline) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create pipeline");
	}

	vkDestroyShaderModule(m_logicalDevice, compShaderModule, nullptr);
}
void Renderer::createPostProcessPipelines(VkRenderPass& renderPass, unsigned int subpass)
{
	// Create multiple post process pipelines based that are very similar because they are all post process effects that 
	// operate solely on a big triangle that gets clipped to a quad.

	float viewportWidth = static_cast<float>(m_presentationObject->getVkExtent().width);
	float viewportHeight = static_cast<float>(m_presentationObject->getVkExtent().height);

	//Shader Stages are defined later on because it changes per pipeline binding since we are creating many pipelines here
	std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;

	VkPipelineVertexInputStateCreateInfo emptyVertexInput =	VulkanPipelineStructures::vertexInputInfo(0, nullptr, 0, nullptr);

	VkPipelineInputAssemblyStateCreateInfo inputAssembly =
		VulkanPipelineStructures::inputAssemblyStateCreationInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FALSE);

	VkViewport viewport = Util::createViewport(	0.0f, 0.0f, viewportWidth, viewportHeight, 0.0f, 1.0f);
	VkRect2D scissor = Util::createRectangle({ 0,0 }, m_presentationObject->getVkExtent());

	VkPipelineViewportStateCreateInfo viewportState = VulkanPipelineStructures::viewportStateCreationInfo(1, &viewport, 1, &scissor);

	VkPipelineRasterizationStateCreateInfo rasterizer =
		VulkanPipelineStructures::rasterizerCreationInfo(VK_FALSE, VK_FALSE, VK_POLYGON_MODE_FILL, 1.0f,
			VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE, VK_FALSE, 0.0f, 0.0f, 0.0f);

	VkPipelineMultisampleStateCreateInfo multisampling =
		VulkanPipelineStructures::multiSampleStateCreationInfo(VK_SAMPLE_COUNT_1_BIT, VK_FALSE, 0.0f, nullptr, VK_FALSE, VK_FALSE);

	VkPipelineDepthStencilStateCreateInfo depthAndStencil =
		VulkanPipelineStructures::depthStencilStateCreationInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS, VK_FALSE, 0.0f, 1.0f, VK_FALSE, {}, {});

	VkPipelineColorBlendAttachmentState colorBlendAttachment =
		VulkanPipelineStructures::colorBlendAttachmentStateInfo(
			VK_FALSE, VK_BLEND_OP_ADD, VK_BLEND_OP_ADD, 0x0,
			VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
			VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO);

	VkPipelineColorBlendStateCreateInfo colorBlending =
		VulkanPipelineStructures::colorBlendStateCreationInfo(VK_FALSE, VK_LOGIC_OP_COPY, 1, &colorBlendAttachment, 0.0f, 0.0f, 0.0f, 0.0f);
	
	VkGraphicsPipelineCreateInfo postProcessPipelineCreateInfo =
		VulkanPipelineStructures::graphicsPipelineCreationInfo(
			static_cast<uint32_t>(shaderStages.size()), shaderStages.data(),
			&emptyVertexInput,
			&inputAssembly,
			nullptr, //tessellation
			&viewportState,
			&rasterizer,
			&multisampling,
			&depthAndStencil,
			&colorBlending,
			nullptr, //dynamicState
			m_postProcess_ToneMap_PipelineLayout,
			renderPass, 0,
			VK_NULL_HANDLE, -1);

	// -------- Create Multiple Pipelines based on the above base ---------
	// Create a pipeline cache so multiple pieplines cane be created from the same pipeline creation Info
	VulkanPipelineCreation::createPipelineCache(m_logicalDevice, m_postProcessPipeLineCache);

	VkShaderModule generic_vertShaderModule = ShaderUtil::createShaderModule("GraphicsPlayground/shaders/postProcess_GenericVertShader.vert.spv", m_logicalDevice);
	shaderStages[0] = ShaderUtil::shaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, generic_vertShaderModule, "main");
	
	// --------- Tone Map Post Process Piepline ----------
	VkShaderModule toneMap_fragShaderModule = ShaderUtil::createShaderModule("GraphicsPlayground/shaders/postProcess_ToneMap.frag.spv", m_logicalDevice);
	shaderStages[1] = ShaderUtil::shaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, toneMap_fragShaderModule, "main");

	postProcessPipelineCreateInfo.layout = m_postProcess_ToneMap_PipelineLayout;

	VulkanPipelineCreation::createGraphicsPipelines(m_logicalDevice, m_postProcessPipeLineCache, 1, &postProcessPipelineCreateInfo, &m_postProcess_ToneMap_PipeLine);
	vkDestroyShaderModule(m_logicalDevice, toneMap_fragShaderModule, nullptr);
	
	// --------- TXAA Post Process Piepline ----------
	VkShaderModule TXAA_fragShaderModule = ShaderUtil::createShaderModule("GraphicsPlayground/shaders/postProcess_TXAA.frag.spv", m_logicalDevice);
	shaderStages[1] = ShaderUtil::shaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, TXAA_fragShaderModule, "main");

	postProcessPipelineCreateInfo.layout = m_postProcess_TXAA_PipelineLayout;

	VulkanPipelineCreation::createGraphicsPipelines(m_logicalDevice, m_postProcessPipeLineCache, 1, &postProcessPipelineCreateInfo, &m_postProcess_TXAA_PipeLine);

	vkDestroyShaderModule(m_logicalDevice, TXAA_fragShaderModule, nullptr);
	vkDestroyShaderModule(m_logicalDevice, generic_vertShaderModule, nullptr);
}