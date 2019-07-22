#include "Renderer.h"

Renderer::Renderer(GLFWwindow* window, RendererOptions rendererOptions, VulkanDevices* devices, VulkanPresentation* presentation,
	Camera* camera, uint32_t width, uint32_t height)
	: m_window(window), 
	m_devices(devices), m_logicalDevice(devices->getLogicalDevice()), m_physicalDevice(devices->getPhysicalDevice()),
	m_presentationObject(presentation),
	m_camera(camera)
{
	initialize();
}
Renderer::~Renderer()
{
	cleanup();

	delete m_scene;
	delete m_renderPassManager;
	delete m_pipelineManager;

	// Command Pools
	vkDestroyCommandPool(m_logicalDevice, m_graphicsCommandPool, nullptr);
	vkDestroyCommandPool(m_logicalDevice, m_computeCommandPool, nullptr);
}

void Renderer::initialize()
{
	m_graphicsQueue = m_devices->getQueue(QueueFlags::Graphics);
	m_computeQueue = m_devices->getQueue(QueueFlags::Compute);
	createCommandPoolsAndBuffers();

	const uint32_t numFrames = m_presentationObject->getCount();
	const VkExtent2D windowsExtent = m_presentationObject->getVkExtent();
	m_scene = new Scene(m_devices, numFrames, windowsExtent, m_graphicsQueue, m_graphicsCommandPool, m_computeQueue, m_computeCommandPool);
	m_renderPassManager = new RenderPassManager(m_devices, m_graphicsQueue, m_graphicsCommandPool, m_presentationObject, numFrames, windowsExtent);
	m_pipelineManager = new PipelineManager(m_devices, m_presentationObject, m_renderPassManager, numFrames, windowsExtent);

	setupDescriptorSets();
	createAllPipelines();
	recordAllCommandBuffers();
}
void Renderer::renderLoop()
{
	// Update Uniforms
	{
		m_camera->updateUniformBuffer(m_presentationObject->getIndex());
		m_scene->updateUniforms(m_presentationObject->getIndex());
	}

	// Wait for the the frame to be finished before working on it
	VkFence inFlightFence = m_presentationObject->getInFlightFence();
	// The VK_TRUE we pass in vkWaitForFences indicates that we want to wait for all fences.
	vkWaitForFences(m_logicalDevice, 1, &inFlightFence, VK_TRUE, std::numeric_limits<uint64_t>::max());


	// Acquire image from swapchain
	// this is the image we will put our final render on and present
	bool flag_recreateSwapChain = m_presentationObject->acquireNextSwapChainImage(m_logicalDevice);
	if (!flag_recreateSwapChain) { recreate(); };


	VkSemaphore waitSemaphores[] = { m_presentationObject->getImageAvailableVkSemaphore() };
	// We want to wait with writing colors to the image until it's available, so we're specifying the stage of the graphics pipeline 
	// that writes to the color attachment. That means that theoretically the implementation can already start executing our vertex 
	// shader and such while the image is not available yet.
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	VkSemaphore signalSemaphores[] = { m_presentationObject->getRenderFinishedVkSemaphore() };	
	VkCommandBuffer* graphicsCommandBuffer = &m_graphicsCommandBuffers[m_presentationObject->getIndex()];
	VkCommandBuffer computeCommandBuffer = m_computeCommandBuffers[m_presentationObject->getIndex()];

	vkResetFences(m_logicalDevice, 1, &inFlightFence);

	//	Submit Commands
	{
		// There is a fence attached to the submission of commands to the graphics queue. Since the fence is a CPU-GPU synchronization
		// it will wait  here until the frame is deemed available, meaning the other queues (in this case only the compute queue) 
		// dont have to create they're own fences that they need to keep track of.
		// The inflight fence basically tells us when a frame has finished rendering, and the submitToQueueSynced function 
		// signals the fence, informing it and thus us of the same.
		VulkanCommandUtil::submitToQueue(m_computeQueue, 1, computeCommandBuffer);
		VulkanCommandUtil::submitToQueueSynced(m_graphicsQueue, 1, graphicsCommandBuffer, 1, waitSemaphores, waitStages, 1, signalSemaphores, inFlightFence);
	}

	// Return the image to the swapchain for presentation
	{
		flag_recreateSwapChain = m_presentationObject->presentImageToSwapChain(m_logicalDevice, m_resizeFrameBuffer);
		if (!flag_recreateSwapChain) { recreate(); };
	}

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
	const VkExtent2D windowExtent = m_presentationObject->getVkExtent();
	m_renderPassManager->recreate(windowExtent);
	m_scene->updateSceneInfrequent(windowExtent);

	setupDescriptorSets();
	createAllPipelines();

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

	vkFreeCommandBuffers(m_logicalDevice, m_graphicsCommandPool, static_cast<uint32_t>(m_graphicsCommandBuffers.size()), m_graphicsCommandBuffers.data());
	vkFreeCommandBuffers(m_logicalDevice, m_computeCommandPool, static_cast<uint32_t>(m_computeCommandBuffers.size()), m_computeCommandBuffers.data());
	
	m_pipelineManager->cleanup();
	m_renderPassManager->cleanup();
	m_presentationObject->cleanup();
	m_camera->cleanup();
	m_scene->cleanup();
	
	vkDestroyDescriptorPool(m_logicalDevice, descriptorPool, nullptr);
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
void Renderer::recordAllCommandBuffers()
{
	for (unsigned int i = 0; i < m_computeCommandBuffers.size(); ++i)
	{
		VulkanCommandUtil::beginCommandBuffer(m_computeCommandBuffers[i]);
		recordComputeCommandBuffer(m_computeCommandBuffers[i], i);
		VulkanCommandUtil::endCommandBuffer(m_computeCommandBuffers[i]);
	}

	for (unsigned int i = 0; i < m_graphicsCommandBuffers.size(); ++i)
	{
		VulkanCommandUtil::beginCommandBuffer(m_graphicsCommandBuffers[i]);
		recordGraphicsCommandBuffer(m_graphicsCommandBuffers[i], i);
		VulkanCommandUtil::endCommandBuffer(m_graphicsCommandBuffers[i]);
	}
}
void Renderer::recordComputeCommandBuffer(VkCommandBuffer& ComputeCmdBuffer, unsigned int frameIndex)
{
	// get compute texture
	Texture* texture = m_scene->getTexture("compute", frameIndex);
	const uint32_t numBlocksX = (texture->getWidth() + WORKGROUP_SIZE - 1) / WORKGROUP_SIZE;
	const uint32_t numBlocksY = (texture->getHeight() + WORKGROUP_SIZE - 1) / WORKGROUP_SIZE;
	const uint32_t numBlocksZ = 1;

	const VkPipeline l_computeP = m_pipelineManager->m_compute_P;
	const VkPipelineLayout l_computePL = m_pipelineManager->m_compute_PL;

	const VkDescriptorSet DS_compute = m_scene->getDescriptorSet(DSL_TYPE::COMPUTE, frameIndex);

	vkCmdBindPipeline(ComputeCmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, l_computeP);
	vkCmdBindDescriptorSets(ComputeCmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, l_computePL, 0, 1, &DS_compute, 0, nullptr);

	// Dispatch the compute kernel, with number of threads =  numBlocksX * numBlocksY * numBlocksZ
	vkCmdDispatch(ComputeCmdBuffer, numBlocksX, numBlocksY, numBlocksZ);
}
void Renderer::recordGraphicsCommandBuffer(VkCommandBuffer& graphicsCmdBuffer, unsigned int frameIndex)
{
	const uint32_t numClearValues = 2;
	std::array<VkClearValue, numClearValues> clearValues = {};
	clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
	clearValues[1].depthStencil = { 1.0f, 0 };

	const VkRect2D renderArea = Util::createRectangle(m_presentationObject->getVkExtent());

	// Create a Image Memory Barrier between the compute pipeline that creates the image and the graphics pipeline that access the image
	// Image barriers should come before render passes begin, unless you're working with subpasses
	{
		Texture* computeTexture = m_scene->getTexture("compute", frameIndex);
		VkImageSubresourceRange imageSubresourceRange = ImageUtil::createImageSubResourceRange(VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1);
		VkImageMemoryBarrier imageMemoryBarrier = ImageUtil::createImageMemoryBarrier(computeTexture->getImage(),
			computeTexture->getImageLayout(), VK_IMAGE_LAYOUT_GENERAL,
			VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, imageSubresourceRange,
			m_devices->getQueueIndex(QueueFlags::Compute), m_devices->getQueueIndex(QueueFlags::Graphics));

		VulkanCommandUtil::pipelineBarrier(graphicsCmdBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
	}
	

	// Model Rendering Pipeline
	{
		const VkPipeline l_geomRenderP = m_pipelineManager->m_rasterization_P;
		const VkPipelineLayout l_geomRenderPL = m_pipelineManager->m_rasterization_PL;
		RenderPassInfo l_geomRenderPass = m_renderPassManager->m_geomRasterPass;

		Model* model = m_scene->getModel("house");
		VkBuffer vertexBuffers[] = { model->getVertexBuffer() };
		VkBuffer indexBuffer = model->getIndexBuffer();
		VkDeviceSize offsets[] = { 0 };

		const VkDescriptorSet DS_model = m_scene->getDescriptorSet(DSL_TYPE::MODEL, frameIndex, "house");
		const VkDescriptorSet DS_camera = m_camera->getDescriptorSet(DSL_TYPE::CURRENT_FRAME_CAMERA, frameIndex);
		
		// Actual commands for the renderPass
		{
			VulkanCommandUtil::beginRenderPass(graphicsCmdBuffer, 
				l_geomRenderPass.renderPass, l_geomRenderPass.frameBuffers[frameIndex],
				renderArea, numClearValues, clearValues.data());

			vkCmdBindDescriptorSets(graphicsCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, l_geomRenderPL, 0, 1, &DS_model, 0, nullptr);
			vkCmdBindDescriptorSets(graphicsCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, l_geomRenderPL, 1, 1, &DS_camera, 0, nullptr);

			vkCmdBindPipeline(graphicsCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, l_geomRenderP);
			vkCmdBindVertexBuffers(graphicsCmdBuffer, 0, 1, vertexBuffers, offsets);
			vkCmdBindIndexBuffer(graphicsCmdBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);

			vkCmdDrawIndexed(graphicsCmdBuffer, model->getNumIndices(), 1, 0, 0, 0);
			vkCmdEndRenderPass(graphicsCmdBuffer);
		}
	}

	// Final Composite Pipeline
	{
		const VkPipeline l_finalCompositeP = m_pipelineManager->m_finalComposite_P;
		const VkPipelineLayout l_finalCompositePL = m_pipelineManager->m_finalComposite_PL;
		RenderPassInfo l_finalCompositePass = m_renderPassManager->m_toDisplayRenderPass;
		
		const VkDescriptorSet DS_finalComposite = m_pipelineManager->getDescriptorSet(DSL_TYPE::FINAL_COMPOSITE, frameIndex );
		
		// Actual commands for the renderPass
		{
			VulkanCommandUtil::beginRenderPass(graphicsCmdBuffer, 
				l_finalCompositePass.renderPass, l_finalCompositePass.frameBuffers[frameIndex],
				renderArea, numClearValues, clearValues.data());

			vkCmdBindDescriptorSets(graphicsCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, l_finalCompositePL, 0, 1, &DS_finalComposite, 0, nullptr);
			vkCmdBindPipeline(graphicsCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, l_finalCompositeP);
			vkCmdDraw(graphicsCmdBuffer, 3, 1, 0, 0);
			
			vkCmdEndRenderPass(graphicsCmdBuffer);
		}
	}
}

void Renderer::setupDescriptorSets()
{
	int numSwapChainImages = m_presentationObject->getCount();
	
	// --- Create Descriptor Pool ---
	{
		// There's a  difference between descriptor Sets and bindings inside a descriptor Set, 
		// multiple bindings can exist inside a descriptor set. So a pool can have fewer sets than number of bindings.
		const uint32_t numSets = 9;
		const uint32_t numFrames = static_cast<uint32_t>(numSwapChainImages);
		const uint32_t maxSets = numSets * numFrames;

		std::vector<VkDescriptorPoolSize> poolSizes = {};
		m_camera->expandDescriptorPool(poolSizes);
		m_scene->expandDescriptorPool(poolSizes);
		m_pipelineManager->expandDescriptorPool(poolSizes);

		const uint32_t poolSizesCount = static_cast<uint32_t>(poolSizes.size());
		DescriptorUtil::createDescriptorPool(m_logicalDevice, maxSets, poolSizesCount, poolSizes.data(), descriptorPool);
	}

	// --- Create Descriptor Set Layouts ---
	{
		// Descriptor set layouts are specified in the pipeline layout object., i.e. during pipeline creation to tell Vulkan 
		// which descriptors the shaders will be using.
		// The numbers are bindingCount, binding, and descriptorCount respectively
		m_camera->createDescriptorSetLayouts();
		m_scene->createDescriptorSetLayouts();		
		m_pipelineManager->createDescriptorSetLayouts();
	}

	// --- Create Descriptor Sets ---
	{
		// dependencies
		DescriptorSetDependencies descSetDependencies;
		{			
			descSetDependencies.computeImages.resize(3);
			descSetDependencies.computeImages[0] = m_scene->getTexture("compute", 0);
			descSetDependencies.computeImages[1] = m_scene->getTexture("compute", 1);
			descSetDependencies.computeImages[2] = m_scene->getTexture("compute", 2);
			descSetDependencies.geomRenderPassImageSet = m_renderPassManager->m_geomRasterPass.imageSetInfo;
		}

		m_camera->createAndWriteDescriptorSets(descriptorPool);
		m_scene->createAndWriteDescriptorSets(descriptorPool);		
		m_pipelineManager->createAndWriteDescriptorSets(descriptorPool, descSetDependencies);
	}
}

void Renderer::createAllPipelines()
{
	std::vector<VkDescriptorSetLayout> computeDSL = { m_scene->getDescriptorSetLayout(DSL_TYPE::COMPUTE) };
	std::vector<VkDescriptorSetLayout> finalCompositeDSL = { m_pipelineManager->getDescriptorSetLayout(DSL_TYPE::FINAL_COMPOSITE)};
	std::vector<VkDescriptorSetLayout> rasterizationDSL = {	m_scene->getDescriptorSetLayout(DSL_TYPE::MODEL),
															m_camera->getDescriptorSetLayout(DSL_TYPE::CURRENT_FRAME_CAMERA) };
	
	DescriptorSetLayouts allDSLs = { computeDSL, finalCompositeDSL, rasterizationDSL };
	
	m_pipelineManager->recreate(m_presentationObject->getVkExtent(), allDSLs);
}