#include "Renderer.h"

Renderer::Renderer(GLFWwindow* window, VulkanManager* vulkanObject, Camera* camera, uint32_t width, uint32_t height)
	: m_window(window), m_vulkanObj(vulkanObject), m_camera(camera)
{
	initialize();
}
Renderer::~Renderer()
{
	cleanup();
	delete m_scene;
	delete m_rendererBackend;
	delete m_UI;
}

void Renderer::initialize()
{
	const uint32_t numFrames = m_vulkanObj->getSwapChainImageCount();
	const VkExtent2D windowsExtent = m_vulkanObj->getSwapChainVkExtent();
	m_rendererBackend = new VulkanRendererBackend(m_vulkanObj, numFrames, windowsExtent);

	VkQueue graphicsQueue = m_vulkanObj->getQueue(QueueFlags::Graphics);
	VkQueue computeQueue = m_vulkanObj->getQueue(QueueFlags::Compute);
	VkCommandPool computeCmdPool = m_rendererBackend->getComputeCommandPool();
	VkCommandPool graphicsCmdPool = m_rendererBackend->getGraphicsCommandPool();
	m_scene = new Scene(m_vulkanObj, numFrames, windowsExtent, graphicsQueue, graphicsCmdPool, computeQueue, computeCmdPool);

	setupDescriptorSets();
	createAllPipelines();
	recordAllCommandBuffers();

	RendererOptions rendererOptions = { 
		RenderAPI::VULKAN, // API
		false, false, false, // Anti-Aliasing 
		false, 1.0f, // Sample Rate Shading
		true, 16.0f	}; // Anisotropy

	m_UI = new UIManager(m_window, m_vulkanObj, rendererOptions);
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

	vkDeviceWaitIdle(m_vulkanObj->getLogicalDevice());

	cleanup();

	m_vulkanObj->recreate(m_window);
	m_rendererBackend->setWindowExtents(m_vulkanObj->getSwapChainVkExtent());
	m_rendererBackend->createRenderPassesAndFrameResources();

	writeToAndUpdateDescriptorSets();
	createAllPipelines();

	m_rendererBackend->recreateCommandBuffers();
	recordAllCommandBuffers();

	m_UI->recreate(m_window);
}
void Renderer::cleanup()
{
	m_vulkanObj->cleanup();
	m_rendererBackend->cleanup();
}


void Renderer::renderLoop()
{
	updateRenderState();
	acquireNextSwapChainImage();

	m_UI->update();

	m_rendererBackend->submitCommandBuffers();
	m_UI->submitDrawCommands();

	presentCurrentImageToSwapChainImage();
}
void Renderer::updateRenderState()
{
	// Update Uniforms
	{
		m_camera->updateUniformBuffer(m_vulkanObj->getIndex());
		m_scene->updateUniforms(m_vulkanObj->getIndex());
	}
}
void Renderer::acquireNextSwapChainImage()
{
	// Wait for the the frame to be finished before working on it
	m_vulkanObj->waitForAndResetInFlightFence();

	// Acquire image from swapchain
	// this is the image we will put our final render on and present
	bool result = m_vulkanObj->acquireNextSwapChainImage();
	if (!result) 
	{ 
		recreate();
	};
}
void Renderer::presentCurrentImageToSwapChainImage()
{
	// Return the image to the swapchain for presentation
	bool result = m_vulkanObj->presentImageToSwapChain();
	if (!result) 
	{
		recreate();
	}

	m_vulkanObj->advanceCurrentFrameIndex();
}


void Renderer::recordAllCommandBuffers()
{
	const unsigned int numCommandBuffers = 3;
	for (unsigned int i = 0; i < numCommandBuffers; ++i)
	{
		VkCommandBuffer computeCmdBuffer = m_rendererBackend->getComputeCommandBuffer(i);
		VulkanCommandUtil::beginCommandBuffer(computeCmdBuffer);
		recordComputeCommandBuffer(computeCmdBuffer, i);
		VulkanCommandUtil::endCommandBuffer(computeCmdBuffer);
	}

	for (unsigned int i = 0; i < numCommandBuffers; ++i)
	{
		VkCommandBuffer graphicsCmdBuffer = m_rendererBackend->getGraphicsCommandBuffer(i);
		VulkanCommandUtil::beginCommandBuffer(graphicsCmdBuffer);
		recordGraphicsCommandBuffer(graphicsCmdBuffer, i);
		VulkanCommandUtil::endCommandBuffer(graphicsCmdBuffer);
	}
}
void Renderer::recordComputeCommandBuffer(VkCommandBuffer& ComputeCmdBuffer, unsigned int frameIndex)
{
	// get compute texture
	Texture* texture = m_scene->getTexture("compute", frameIndex);
	const uint32_t numBlocksX = (texture->getWidth() + WORKGROUP_SIZE - 1) / WORKGROUP_SIZE;
	const uint32_t numBlocksY = (texture->getHeight() + WORKGROUP_SIZE - 1) / WORKGROUP_SIZE;
	const uint32_t numBlocksZ = 1;

	const VkPipeline l_computeP = m_rendererBackend->getPipeline(PIPELINE_TYPE::COMPUTE);
	const VkPipelineLayout l_computePL = m_rendererBackend->getPipelineLayout(PIPELINE_TYPE::COMPUTE);
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

	const VkRect2D renderArea = Util::createRectangle(m_vulkanObj->getSwapChainVkExtent());

	// Create a Image Memory Barrier between the compute pipeline that creates the image and the graphics pipeline that access the image
	// Image barriers should come before render passes begin, unless you're working with subpasses
	{
		Texture* computeTexture = m_scene->getTexture("compute", frameIndex);
		VkImageSubresourceRange imageSubresourceRange = ImageUtil::createImageSubResourceRange(VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1);
		VkImageMemoryBarrier imageMemoryBarrier = ImageUtil::createImageMemoryBarrier(computeTexture->getImage(),
			computeTexture->getImageLayout(), VK_IMAGE_LAYOUT_GENERAL,
			VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, imageSubresourceRange,
			m_vulkanObj->getQueueIndex(QueueFlags::Compute), m_vulkanObj->getQueueIndex(QueueFlags::Graphics));

		VulkanCommandUtil::pipelineBarrier(graphicsCmdBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
	}
	

	// Model Rendering Pipeline
	{
		const VkPipeline l_rasterP = m_rendererBackend->getPipeline(PIPELINE_TYPE::RASTER);
		const VkPipelineLayout l_rasterPL = m_rendererBackend->getPipelineLayout(PIPELINE_TYPE::RASTER);
		RenderPassInfo l_rasterRPI = m_rendererBackend->m_rasterRPI;

		Model* model = m_scene->getModel("house");
		VkBuffer vertexBuffers[] = { model->getVertexBuffer() };
		VkBuffer indexBuffer = model->getIndexBuffer();
		VkDeviceSize offsets[] = { 0 };

		const VkDescriptorSet DS_model = m_scene->getDescriptorSet(DSL_TYPE::MODEL, frameIndex, "house");
		const VkDescriptorSet DS_camera = m_camera->getDescriptorSet(DSL_TYPE::CURRENT_FRAME_CAMERA, frameIndex);
		
		// Actual commands for the renderPass
		{
			VulkanCommandUtil::beginRenderPass(graphicsCmdBuffer, 
				l_rasterRPI.renderPass, l_rasterRPI.frameBuffers[frameIndex],
				renderArea, numClearValues, clearValues.data());

			vkCmdBindDescriptorSets(graphicsCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, l_rasterPL, 0, 1, &DS_model, 0, nullptr);
			vkCmdBindDescriptorSets(graphicsCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, l_rasterPL, 1, 1, &DS_camera, 0, nullptr);

			vkCmdBindPipeline(graphicsCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, l_rasterP);
			vkCmdBindVertexBuffers(graphicsCmdBuffer, 0, 1, vertexBuffers, offsets);
			vkCmdBindIndexBuffer(graphicsCmdBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);

			vkCmdDrawIndexed(graphicsCmdBuffer, model->getNumIndices(), 1, 0, 0, 0);
			vkCmdEndRenderPass(graphicsCmdBuffer);
		}
	}

	// Final Composite Pipeline
	{
		const VkPipeline l_finalCompositeP = m_rendererBackend->getPipeline(PIPELINE_TYPE::FINAL_COMPOSITE);
		const VkPipelineLayout l_finalCompositePL = m_rendererBackend->getPipelineLayout(PIPELINE_TYPE::FINAL_COMPOSITE);
		RenderPassInfo l_finalCompositeRPI = m_rendererBackend->m_toDisplayRPI;
		
		const VkDescriptorSet DS_finalComposite = m_rendererBackend->getDescriptorSet(DSL_TYPE::FINAL_COMPOSITE, frameIndex );
		
		// Actual commands for the renderPass
		{
			VulkanCommandUtil::beginRenderPass(graphicsCmdBuffer, 
				l_finalCompositeRPI.renderPass, l_finalCompositeRPI.frameBuffers[frameIndex],
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

	// --- Write to Descriptor Sets ---
	writeToAndUpdateDescriptorSets();
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
	std::vector<VkDescriptorSetLayout> finalCompositeDSL = { m_rendererBackend->getDescriptorSetLayout(DSL_TYPE::FINAL_COMPOSITE)};
	std::vector<VkDescriptorSetLayout> rasterizationDSL = {	m_scene->getDescriptorSetLayout(DSL_TYPE::MODEL),
															m_camera->getDescriptorSetLayout(DSL_TYPE::CURRENT_FRAME_CAMERA) };
	
	DescriptorSetLayouts allDSLs = { computeDSL, finalCompositeDSL, rasterizationDSL };
	
	m_rendererBackend->createPipelines(allDSLs);
}