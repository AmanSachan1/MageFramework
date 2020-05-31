#include "Renderer.h"

#include <Vulkan/Utilities/vPipelineUtil.h>
#include <Vulkan/Utilities/vBufferUtil.h>
#include <SceneElements/texture.h>

Renderer::Renderer(GLFWwindow* window, 
	std::shared_ptr<VulkanManager> vulkanManager, 
	std::shared_ptr<Camera> camera, JSONItem::Scene& scene,
	RendererOptions& rendererOptions, uint32_t width, uint32_t height)
	: m_window(window), m_vulkanManager(vulkanManager), 
	m_camera(camera), m_rendererOptions(rendererOptions),
	m_windowResized(false)
{
	initialize(scene);
}
Renderer::~Renderer()
{
	vkDeviceWaitIdle(m_vulkanManager->getLogicalDevice());
	cleanup();

	if (m_rendererOptions.renderType == RENDER_TYPE::RAYTRACE)
	{
		for (auto& modelElement : m_scene->m_modelMap)
		{
			auto model = modelElement.second;
			vkFreeMemory(m_vulkanManager->getLogicalDevice(), model->m_bottomLevelAS.m_memory, nullptr);
			m_rendererBackend->vkDestroyAccelerationStructureNV(
				m_vulkanManager->getLogicalDevice(), model->m_bottomLevelAS.m_accelerationStructure, nullptr);
		}
	}
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

	if (m_rendererOptions.renderType == RENDER_TYPE::RAYTRACE)
	{
		m_rendererBackend->createAndBuildAccelerationStructures(m_scene);
		m_rendererBackend->createStorageImages(); // Create StorageImage for RayTraced Image
	}

	createAllPipelines();

	if (m_rendererOptions.renderType == RENDER_TYPE::RAYTRACE)
	{
		m_rendererBackend->createShaderBindingTable();
	}

	m_rendererBackend->createAllPostProcessEffects(m_scene);

	writeToAndUpdateDescriptorSets();
	m_rendererBackend->recordAllCommandBuffers(m_camera, m_scene);

	m_UI = std::make_shared<UIManager>(m_window, m_vulkanManager, m_rendererOptions);
}
void Renderer::recreate()
{
	// This while loop handles the case of minimization of the window
	int width = 0, height = 0;
	glfwGetFramebufferSize(m_window, &width, &height);
	while (width == 0 || height == 0)
	{
		glfwGetFramebufferSize(m_window, &width, &height);
		glfwWaitEvents();
	}

	vkDeviceWaitIdle(m_vulkanManager->getLogicalDevice());

	cleanup();

	m_scene->recreate();
	m_vulkanManager->recreate(m_window);
	m_rendererBackend->setWindowExtents(m_vulkanManager->getSwapChainVkExtent());
	m_rendererBackend->recreateCommandBuffers();
	m_rendererBackend->createRenderPassesAndFrameResources();

	if (m_rendererOptions.renderType == RENDER_TYPE::RAYTRACE)
	{
		m_rendererBackend->createStorageImages(); // Recreate StorageImage for RayTraced Image
	}

	createAllPipelines();

	if (m_rendererOptions.renderType == RENDER_TYPE::RAYTRACE)
	{
		m_rendererBackend->mapShaderBindingTable(); // Update shaderbinding table now that pipelines have been recreated
	}

	m_rendererBackend->createAllPostProcessEffects(m_scene);

	writeToAndUpdateDescriptorSets();
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
	acquireNextSwapChainImage();

	updateRenderState();
	m_UI->update(prevFrameTime);
	
	m_rendererBackend->submitCommandBuffers();
	VkSemaphore waitSemaphore = m_rendererBackend->getpostProcessFinishedVkSemaphore(m_vulkanManager->getImageIndex());
 	VkSemaphore signalSemaphore = m_vulkanManager->getRenderFinishedVkSemaphore();
	m_UI->submitDrawCommands(waitSemaphore, signalSemaphore);

	presentCurrentImageToSwapChainImage();
}
void Renderer::updateRenderState()
{
	// Update Uniforms
	const uint32_t currentImageIndex = m_vulkanManager->getImageIndex();
	{
		m_camera->updateUniformBuffer(currentImageIndex);
		m_scene->updateUniforms(currentImageIndex);
		m_rendererBackend->update(currentImageIndex);
	}
}
void Renderer::acquireNextSwapChainImage()
{
	// Wait for the the frame to be finished before working on it
	m_vulkanManager->waitForFrameInFlightFence();

	// Acquire image from swapchain
	// this is the image we will put our final render on and present
	bool result = m_vulkanManager->acquireNextSwapChainImage();
	if (!result)
	{
		recreate();
	}

	m_vulkanManager->waitForImageInFlightFence();
	m_vulkanManager->resetFrameInFlightFence();
}

void Renderer::presentCurrentImageToSwapChainImage()
{
	// Return the image to the swapchain for presentation
	bool result = m_vulkanManager->presentImageToSwapChain();
	if (!result || m_windowResized)
	{
		m_windowResized = false;
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
	m_camera->writeToAndUpdateDescriptorSets();
	m_scene->writeToAndUpdateDescriptorSets();
	
	if (m_rendererOptions.renderType == RENDER_TYPE::RAYTRACE)
	{
		m_rendererBackend->writeToAndUpdateDescriptorSets_rayTracing(m_camera, m_scene);
	}

	m_rendererBackend->writeToAndUpdateDescriptorSets();
}

void Renderer::createAllPipelines()
{
	DescriptorSetLayouts allDSLs;
	allDSLs.computeDSL						= { m_scene->getDescriptorSetLayout(DSL_TYPE::COMPUTE) };
	allDSLs.rasterDSL						= { m_camera->m_DSL_camera, m_scene->getDescriptorSetLayout(DSL_TYPE::MODEL) };
	allDSLs.raytraceDSL						= { m_rendererBackend->m_DSL_rayTrace };

	m_rendererBackend->createPipelines(allDSLs);	
}