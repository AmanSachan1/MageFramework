#include "Renderer.h"

Renderer::Renderer(GLFWwindow* window, 
	std::shared_ptr<VulkanManager> vulkanManager, 
	std::shared_ptr<Camera> camera,
	uint32_t width, uint32_t height)
	: m_window(window), m_vulkanManager(vulkanManager), m_camera(camera)
{
	initialize();
}
Renderer::~Renderer()
{
	vkDeviceWaitIdle(m_vulkanManager->getLogicalDevice());
	cleanup();
}

void Renderer::initialize()
{
	const uint32_t numFrames = m_vulkanManager->getSwapChainImageCount();
	const VkExtent2D windowsExtent = m_vulkanManager->getSwapChainVkExtent();
	m_rendererBackend = std::make_shared<VulkanRendererBackend>(m_vulkanManager, numFrames, windowsExtent);

	VkQueue graphicsQueue = m_vulkanManager->getQueue(QueueFlags::Graphics);
	VkQueue computeQueue = m_vulkanManager->getQueue(QueueFlags::Compute);
	VkCommandPool computeCmdPool = m_rendererBackend->getComputeCommandPool();
	VkCommandPool graphicsCmdPool = m_rendererBackend->getGraphicsCommandPool();
	m_scene = std::make_shared<Scene>(m_vulkanManager, numFrames, windowsExtent, graphicsQueue, graphicsCmdPool, computeQueue, computeCmdPool);

	m_rendererBackend->createSyncObjects();
	setupDescriptorSets();
	createAllPipelines();
	m_rendererBackend->createAllPostProcessEffects(m_scene);
	writeToAndUpdateDescriptorSets();
	recordAllCommandBuffers();

	RendererOptions rendererOptions = { 
		RenderAPI::VULKAN, // API
		false, false, false, // Anti-Aliasing 
		false, 1.0f, // Sample Rate Shading
		true, 16.0f	}; // Anisotropy

	m_UI = std::make_shared<UIManager>(m_window, m_vulkanManager, rendererOptions);
}
void Renderer::recreate()
{
	// This while loop handles the case of minimization of the window
	int width = 0, height = 0;
	while (width == 0 || height == 0)
	{
		glfwGetFramebufferSize(m_window, &width, &height);
		glfwWaitEvents();
	}

	vkDeviceWaitIdle(m_vulkanManager->getLogicalDevice());

	cleanup();

	m_vulkanManager->recreate(m_window);
	m_rendererBackend->setWindowExtents(m_vulkanManager->getSwapChainVkExtent());
	m_rendererBackend->createRenderPassesAndFrameResources();
	m_rendererBackend->createSyncObjects();

	createAllPipelines();
	m_rendererBackend->createAllPostProcessEffects(m_scene);	
	writeToAndUpdateDescriptorSets();

	m_rendererBackend->recreateCommandBuffers();
	recordAllCommandBuffers();

	m_UI->resize(m_window);
}
void Renderer::cleanup()
{
	m_vulkanManager->cleanup();
	m_rendererBackend->cleanup();
}


void Renderer::renderLoop(float prevFrameTime)
{
	updateRenderState();
	m_UI->update(prevFrameTime);

	acquireNextSwapChainImage();
	m_rendererBackend->submitCommandBuffers();

	VkSemaphore waitSemaphore = m_rendererBackend->getpostProcessFinishedVkSemaphore(m_vulkanManager->getIndex());
	VkSemaphore signalSemaphore = m_vulkanManager->getRenderFinishedVkSemaphore();
	m_UI->submitDrawCommands(waitSemaphore, signalSemaphore);
	presentCurrentImageToSwapChainImage();
}
void Renderer::updateRenderState()
{
	// Update Uniforms
	const uint32_t currentFrameIndex = m_vulkanManager->getIndex();
	{
		m_camera->updateUniformBuffer(currentFrameIndex);
		m_scene->updateUniforms(currentFrameIndex);
		m_rendererBackend->update(currentFrameIndex);
	}
}
void Renderer::acquireNextSwapChainImage()
{
	// Wait for the the frame to be finished before working on it
	m_vulkanManager->waitForAndResetInFlightFence();

	// Acquire image from swapchain
	// this is the image we will put our final render on and present
	bool result = m_vulkanManager->acquireNextSwapChainImage();
	if (!result) 
	{ 
		recreate();
	};
}
void Renderer::presentCurrentImageToSwapChainImage()
{
	// Return the image to the swapchain for presentation
	bool result = m_vulkanManager->presentImageToSwapChain();
	if (!result) 
	{
		recreate();
	}

	m_vulkanManager->advanceCurrentFrameIndex();
}


void Renderer::recordAllCommandBuffers()
{
	const uint32_t numClearValues = 2;
	std::array<VkClearValue, numClearValues> clearValues = {};
	clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
	clearValues[1].depthStencil = { 1.0f, 0 };

	const VkRect2D renderArea = Util::createRectangle(m_vulkanManager->getSwapChainVkExtent());
	const unsigned int numCommandBuffers = m_vulkanManager->getSwapChainImageCount();
	unsigned int i = 0;
	for (; i < numCommandBuffers; i++)
	{
		VkCommandBuffer computeCmdBuffer = m_rendererBackend->getComputeCommandBuffer(i);
		VulkanCommandUtil::beginCommandBuffer(computeCmdBuffer);
		m_rendererBackend->recordCommandBuffer_ComputeCmds(i, computeCmdBuffer, m_scene);
		VulkanCommandUtil::endCommandBuffer(computeCmdBuffer);
	}

	for (i = 0; i < numCommandBuffers; i++)
	{
		VkCommandBuffer graphicsCmdBuffer = m_rendererBackend->getGraphicsCommandBuffer(i);
		VulkanCommandUtil::beginCommandBuffer(graphicsCmdBuffer);
		m_rendererBackend->recordCommandBuffer_GraphicsCmds(i, graphicsCmdBuffer, m_scene, m_camera, renderArea, numClearValues, clearValues.data());
		VulkanCommandUtil::endCommandBuffer(graphicsCmdBuffer);
	}

	for (i = 0; i < numCommandBuffers; i++)
	{
		VkCommandBuffer postProcessCmdBuffer = m_rendererBackend->getPostProcessCommandBuffer(i);
		VulkanCommandUtil::beginCommandBuffer(postProcessCmdBuffer);
		m_rendererBackend->recordCommandBuffer_PostProcessCmds(i, postProcessCmdBuffer, m_scene, renderArea, numClearValues, clearValues.data());
		VulkanCommandUtil::endCommandBuffer(postProcessCmdBuffer);
	}
}


void Renderer::setupDescriptorSets()
{
	// --- Create Descriptor Pool ---
	{
		std::vector<VkDescriptorPoolSize> poolSizes = {};
		m_camera->expandDescriptorPool(poolSizes);
		m_scene->expandDescriptorPool(poolSizes);
		m_rendererBackend->expandDescriptorPool(poolSizes);
		m_rendererBackend->createDescriptorPool(poolSizes);
	}

	VkDescriptorPool descriptorPool = m_rendererBackend->getDescriptorPool();

	// --- Create Descriptor Set Layouts and the associated Descriptor Sets ---
	{
		m_camera->createDescriptors(descriptorPool);
		m_scene->createDescriptors(descriptorPool);
		m_rendererBackend->createDescriptors(descriptorPool);
	}
}
void Renderer::writeToAndUpdateDescriptorSets()
{	
	VkDescriptorPool descriptorPool = m_rendererBackend->getDescriptorPool();
	
	// dependencies
	DescriptorSetDependencies descSetDependencies;
	{
		descSetDependencies.computeImages.resize(3);
		descSetDependencies.computeImages[0] = m_scene->getTexture("compute", 0);
		descSetDependencies.computeImages[1] = m_scene->getTexture("compute", 1);
		descSetDependencies.computeImages[2] = m_scene->getTexture("compute", 2);
		descSetDependencies.geomRenderPassImageSet = m_rendererBackend->m_rasterRPI.imageSetInfo;
	}

	m_camera->writeToAndUpdateDescriptorSets();
	m_scene->writeToAndUpdateDescriptorSets();
	m_rendererBackend->writeToAndUpdateDescriptorSets(descSetDependencies);
}

void Renderer::createAllPipelines()
{
	std::vector<VkDescriptorSetLayout> computeDSL = { m_scene->getDescriptorSetLayout(DSL_TYPE::COMPUTE) };
	std::vector<VkDescriptorSetLayout> compositeComputeOntoRasterDSL = { m_rendererBackend->getDescriptorSetLayout(DSL_TYPE::COMPOSITE_COMPUTE_ONTO_RASTER)};
	std::vector<VkDescriptorSetLayout> rasterizationDSL = {	m_scene->getDescriptorSetLayout(DSL_TYPE::MODEL),
															m_camera->getDescriptorSetLayout(DSL_TYPE::CURRENT_FRAME_CAMERA) };
	
	DescriptorSetLayouts allDSLs = { computeDSL, compositeComputeOntoRasterDSL, rasterizationDSL };
	
	m_rendererBackend->createPipelines(allDSLs);
}