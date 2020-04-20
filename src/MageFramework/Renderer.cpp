#include "Renderer.h"

#include <Vulkan/Utilities/vPipelineUtil.h>
#include <Vulkan/Utilities/vBufferUtil.h>
#include <SceneElements/texture.h>

Renderer::Renderer(GLFWwindow* window, 
	std::shared_ptr<VulkanManager> vulkanManager, 
	std::shared_ptr<Camera> camera, JSONItem::Scene& scene,
	RendererOptions& rendererOptions, uint32_t width, uint32_t height)
	: m_window(window), m_vulkanManager(vulkanManager), 
	m_camera(camera), m_rendererOptions(rendererOptions)
{
	initialize(scene);
}
Renderer::~Renderer()
{
	vkDeviceWaitIdle(m_vulkanManager->getLogicalDevice());
	cleanup();
}

void Renderer::initialize(JSONItem::Scene& scene)
{
	const uint32_t numFrames = m_vulkanManager->getSwapChainImageCount();
	const VkExtent2D windowsExtent = m_vulkanManager->getSwapChainVkExtent();
	m_rendererBackend = std::make_shared<VulkanRendererBackend>(m_vulkanManager, m_rendererOptions, numFrames, windowsExtent);

	VkQueue graphicsQueue = m_vulkanManager->getQueue(QueueFlags::Graphics);
	VkQueue computeQueue = m_vulkanManager->getQueue(QueueFlags::Compute);
	VkCommandPool computeCmdPool = m_rendererBackend->getComputeCommandPool();
	VkCommandPool graphicsCmdPool = m_rendererBackend->getGraphicsCommandPool();
	m_scene = std::make_shared<Scene>(m_vulkanManager, scene, m_rendererOptions.renderType, numFrames, windowsExtent, 
		graphicsQueue, graphicsCmdPool, computeQueue, computeCmdPool );

  	m_rendererBackend->createSyncObjects();
	setupDescriptorSets();

	if (m_rendererOptions.renderType == RENDER_TYPE::RASTERIZATION)
	{
		createAllPipelines();
		m_rendererBackend->createAllPostProcessEffects(m_scene);
		writeToAndUpdateDescriptorSets();
	}
	if (m_rendererOptions.renderType == RENDER_TYPE::RAYTRACE)
	{
		m_rendererBackend->setupRayTracing(m_camera, m_scene);
	}
	m_rendererBackend->recordAllCommandBuffers(m_camera, m_scene);

	m_UI = std::make_shared<UIManager>(m_window, m_vulkanManager, m_rendererOptions);
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

	if (m_rendererOptions.renderType == RENDER_TYPE::RASTERIZATION)
	{
		createAllPipelines();
		m_rendererBackend->createAllPostProcessEffects(m_scene);
		writeToAndUpdateDescriptorSets();
	}
	if (m_rendererOptions.renderType == RENDER_TYPE::RAYTRACE)
	{
		m_rendererBackend->setupRayTracing(m_camera, m_scene);
	}

	m_rendererBackend->recreateCommandBuffers();
	m_rendererBackend->recordAllCommandBuffers(m_camera, m_scene);
	
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

	VkSemaphore waitSemaphore;
	if (m_rendererOptions.renderType == RENDER_TYPE::RASTERIZATION)
	{
		waitSemaphore = m_rendererBackend->getpostProcessFinishedVkSemaphore(m_vulkanManager->getIndex());
	}
	if (m_rendererOptions.renderType == RENDER_TYPE::RAYTRACE)
	{
		waitSemaphore = m_rendererBackend->m_rayTracingOperationsFinishedSemaphores[m_vulkanManager->getIndex()];
	}
	
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


void Renderer::setupDescriptorSets()
{
	// --- Create Descriptor Pool ---
	std::vector<VkDescriptorPoolSize> poolSizes = {};
	m_camera->expandDescriptorPool(poolSizes);
	m_scene->expandDescriptorPool(poolSizes);
	m_rendererBackend->expandDescriptorPool(poolSizes);
	m_rendererBackend->createDescriptorPool(poolSizes);

	// --- Create Descriptor Set Layouts and the associated Descriptor Sets ---
	VkDescriptorPool descriptorPool = m_rendererBackend->getDescriptorPool();
	m_camera->createDescriptors(descriptorPool);
	m_scene->createDescriptors(descriptorPool);
	m_rendererBackend->createDescriptors(descriptorPool);
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
	std::vector<VkDescriptorSetLayout> rasterizationDSL = { m_camera->m_DSL_camera,
															m_scene->getDescriptorSetLayout(DSL_TYPE::MODEL)};
	
	DescriptorSetLayouts allDSLs = { computeDSL, compositeComputeOntoRasterDSL, rasterizationDSL };
	
	m_rendererBackend->createPipelines(allDSLs);
}