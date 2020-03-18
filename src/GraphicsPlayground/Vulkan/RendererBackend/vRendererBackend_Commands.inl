#pragma once
#include <Vulkan\RendererBackend\vRendererBackend.h>

static constexpr unsigned int WORKGROUP_SIZE = 32;

inline void VulkanRendererBackend::createCommandPoolsAndBuffers()
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
inline void VulkanRendererBackend::recreateCommandBuffers()
{
	m_graphicsCommandBuffers.resize(m_numSwapChainImages);
	m_computeCommandBuffers.resize(m_numSwapChainImages);
	VulkanCommandUtil::allocateCommandBuffers(m_logicalDevice, m_graphicsCommandPool, m_graphicsCommandBuffers);
	VulkanCommandUtil::allocateCommandBuffers(m_logicalDevice, m_computeCommandPool, m_computeCommandBuffers);
}
inline void VulkanRendererBackend::submitCommandBuffers()
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

inline void VulkanRendererBackend::recordCommandBuffer_ComputeCmds(
	unsigned int frameIndex, VkCommandBuffer& ComputeCmdBuffer, std::shared_ptr<Scene> scene)
{
	// Test Compute Pass/Kernel
	{
		// Get compute texture
		std::shared_ptr<Texture> texture = scene->getTexture("compute", frameIndex);
		
		// Decide the size of the compute kernel
		// Number of threads in the compute kernel =  numBlocksX * numBlocksY * numBlocksZ
		const uint32_t numBlocksX = (texture->getWidth() + WORKGROUP_SIZE - 1) / WORKGROUP_SIZE;
		const uint32_t numBlocksY = (texture->getHeight() + WORKGROUP_SIZE - 1) / WORKGROUP_SIZE;
		const uint32_t numBlocksZ = 1;
		
		const VkPipeline l_computeP = getPipeline(PIPELINE_TYPE::COMPUTE);
		const VkPipelineLayout l_computePL = getPipelineLayout(PIPELINE_TYPE::COMPUTE);
		const VkDescriptorSet DS_compute = scene->getDescriptorSet(DSL_TYPE::COMPUTE, frameIndex);

		vkCmdBindPipeline(ComputeCmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, l_computeP);
		vkCmdBindDescriptorSets(ComputeCmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, l_computePL, 0, 1, &DS_compute, 0, nullptr);
		vkCmdDispatch(ComputeCmdBuffer, numBlocksX, numBlocksY, numBlocksZ); // Dispatch the Compute Kernel
	}
}
inline void VulkanRendererBackend::recordCommandBuffer_GraphicsCmds(
	unsigned int frameIndex, VkCommandBuffer& graphicsCmdBuffer, std::shared_ptr<Scene> scene, std::shared_ptr<Camera> camera)
{
	const uint32_t numClearValues = 2;
	std::array<VkClearValue, numClearValues> clearValues = {};
	clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
	clearValues[1].depthStencil = { 1.0f, 0 };

	const VkRect2D renderArea = Util::createRectangle(m_vulkanObj->getSwapChainVkExtent());

	// Create a Image Memory Barrier between the compute pipeline that creates the image and the graphics pipeline that access the image
	// Image barriers should come before render passes begin, unless you're working with subpasses
	{
		std::shared_ptr<Texture> computeTexture = scene->getTexture("compute", frameIndex);
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
		const VkPipeline l_rasterP = getPipeline(PIPELINE_TYPE::RASTER);
		const VkPipelineLayout l_rasterPL = getPipelineLayout(PIPELINE_TYPE::RASTER);

		std::shared_ptr<Model> model = scene->getModel("house");
		VkBuffer vertexBuffers[] = { model->getVertexBuffer() };
		VkBuffer indexBuffer = model->getIndexBuffer();
		VkDeviceSize offsets[] = { 0 };

		const VkDescriptorSet DS_model = scene->getDescriptorSet(DSL_TYPE::MODEL, frameIndex, "house");
		const VkDescriptorSet DS_camera = camera->getDescriptorSet(DSL_TYPE::CURRENT_FRAME_CAMERA, frameIndex);

		// Actual commands for the renderPass
		{
			VulkanCommandUtil::beginRenderPass(graphicsCmdBuffer,
				m_rasterRPI.renderPass, m_rasterRPI.frameBuffers[frameIndex],
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

	// Composite Pipeline
	{
		const VkPipeline l_compositeComputeOntoRasterP = getPipeline(PIPELINE_TYPE::COMPOSITE_COMPUTE_ONTO_RASTER);
		const VkPipelineLayout l_compositeComputeOntoRasterPL = getPipelineLayout(PIPELINE_TYPE::COMPOSITE_COMPUTE_ONTO_RASTER);

		const VkDescriptorSet DS_compositeComputeOntoRaster = getDescriptorSet(DSL_TYPE::COMPOSITE_COMPUTE_ONTO_RASTER, frameIndex);

		// Actual commands for the renderPass
		{
			VulkanCommandUtil::beginRenderPass(graphicsCmdBuffer,
				m_compositeComputeOntoRasterRPI.renderPass, m_compositeComputeOntoRasterRPI.frameBuffers[frameIndex],
				renderArea, numClearValues, clearValues.data());

			vkCmdBindDescriptorSets(graphicsCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, l_compositeComputeOntoRasterPL, 0, 1, &DS_compositeComputeOntoRaster, 0, nullptr);
			vkCmdBindPipeline(graphicsCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, l_compositeComputeOntoRasterP);
			vkCmdDraw(graphicsCmdBuffer, 3, 1, 0, 0);

			vkCmdEndRenderPass(graphicsCmdBuffer);
		}
	}

	recordCommandBuffer_PostProcessCmds(frameIndex, graphicsCmdBuffer, scene, renderArea, numClearValues, clearValues.data());
	recordCommandBuffer_FinalCmds(frameIndex, graphicsCmdBuffer);
}
inline void VulkanRendererBackend::recordCommandBuffer_PostProcessCmds(
	unsigned int frameIndex, VkCommandBuffer& graphicsCmdBuffer, std::shared_ptr<Scene> scene,
	VkRect2D renderArea, uint32_t clearValueCount, const VkClearValue* clearValue)
{
	// Tonemap Render Pass
	{
		const unsigned int postProcessIndex = 0; // TODO: CHANGE this when there are multiple post process passes
		const VkPipeline l_toneMapP = m_postProcess_Ps[postProcessIndex];
		const VkPipelineLayout l_toneMapPL = m_postProcess_PLs[postProcessIndex];

		const VkDescriptorSet DS_tonemap = getDescriptorSet(DSL_TYPE::POST_PROCESS, frameIndex, postProcessIndex);
		const VkDescriptorSet DS_timeUBO = scene->getDescriptorSet(DSL_TYPE::TIME, frameIndex);

		// Actual commands for the renderPass
		{
			VulkanCommandUtil::beginRenderPass(graphicsCmdBuffer,
				m_toneMapRPI.renderPass, m_toneMapRPI.frameBuffers[frameIndex],
				renderArea, clearValueCount, clearValue);

			vkCmdBindDescriptorSets(graphicsCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, l_toneMapPL, 0, 1, &DS_tonemap, 0, nullptr);
			vkCmdBindDescriptorSets(graphicsCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, l_toneMapPL, 1, 1, &DS_timeUBO, 0, nullptr);
			vkCmdBindPipeline(graphicsCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, l_toneMapP);
			vkCmdDraw(graphicsCmdBuffer, 3, 1, 0, 0);

			vkCmdEndRenderPass(graphicsCmdBuffer);
		}
	}
}
inline void VulkanRendererBackend::recordCommandBuffer_FinalCmds(
	unsigned int frameIndex, VkCommandBuffer& graphicsCmdBuffer)
{
	//--- Decoupling Post Process Passes from the swapchain ---
	// We are not going to be making the last post process effect write to the swapchain directly. 
	// This is because we want to decouple the post process passes, and hence their ordering, from the swapchain. 
	// Instead of creating a renderpass to copy the results of the last post process pass into the swapchain we simply use a copy image command.
	// The vkCmdCopyImage command requires that we do some image tranisitions for the source and destination images
	{
		VkImage& srcImage = m_toneMapRPI.inputImages[frameIndex].image;
		VkImageLayout swapChainImageLayoutInitial = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; ;
		VkImageLayout swapChainImageLayoutBeforeUIPass = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		// Transition swapchain to VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
		m_vulkanObj->transitionSwapChainImageLayout(frameIndex, swapChainImageLayoutInitial, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, graphicsCmdBuffer, m_graphicsCommandPool);
		// Copy last post process pass's image into the swapchain image
		m_vulkanObj->copyImageToSwapChainImage(frameIndex, srcImage, graphicsCmdBuffer, m_graphicsCommandPool, m_windowExtents);
		// Transition swapchain to the VkImageLayout expected by our UI manager
		m_vulkanObj->transitionSwapChainImageLayout(frameIndex, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, swapChainImageLayoutBeforeUIPass, graphicsCmdBuffer, m_graphicsCommandPool);
	}
}