#pragma once
#include <Vulkan/RendererBackend/vRendererBackend.h>

static constexpr unsigned int WORKGROUP_SIZE = 32;

inline void VulkanRendererBackend::createCommandPoolsAndBuffers()
{
	// Commands in Vulkan, like drawing operations and memory transfers, are not executed directly using function calls. 
	// You have to record all of the operations you want to perform in command buffer objects. The advantage of this is 
	// that all of the hard work of setting up the drawing commands can be done in advance and in multiple threads.

	// Command buffers will be automatically freed when their command pool is destroyed, so we don't need an explicit cleanup.

	// Because one of the drawing commands involves binding the right VkFramebuffer, we'll actually have to 
	// record a command buffer for every image in the swap chain once again.

	VulkanCommandUtil::createCommandPool(m_logicalDevice, m_computeCommandPool, m_vulkanManager->getQueueIndex(QueueFlags::Compute));
	VulkanCommandUtil::createCommandPool(m_logicalDevice, m_graphicsCommandPool, m_vulkanManager->getQueueIndex(QueueFlags::Graphics));
	recreateCommandBuffers();
}
inline void VulkanRendererBackend::recreateCommandBuffers()
{
	m_computeCommandBuffers.resize(m_numSwapChainImages);
	m_graphicsCommandBuffers.resize(m_numSwapChainImages);	
	m_postProcessCommandBuffers.resize(m_numSwapChainImages);

	VulkanCommandUtil::allocateCommandBuffers(m_logicalDevice, m_computeCommandPool, m_computeCommandBuffers);
	VulkanCommandUtil::allocateCommandBuffers(m_logicalDevice, m_graphicsCommandPool, m_graphicsCommandBuffers);
	VulkanCommandUtil::allocateCommandBuffers(m_logicalDevice, m_graphicsCommandPool, m_postProcessCommandBuffers);
}
inline void VulkanRendererBackend::submitCommandBuffers()
{
	VkSemaphore waitSemaphores[] = { m_vulkanManager->getImageAvailableVkSemaphore() };
	// We want to wait with writing colors to the image until it's available, so we're specifying the stage of the graphics pipeline 
	// that writes to the color attachment. That means that theoretically the implementation can already start executing our vertex 
	// shader and such while the image is not available yet.
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	VkSemaphore signalSemaphores[] = { m_vulkanManager->getRenderFinishedVkSemaphore() };
	VkFence inFlightFence = m_vulkanManager->getInFlightFence();

	//	Submit Commands
	{
		// There is a fence attached to the submission of commands to the graphics queue. Since the fence is a CPU-GPU synchronization
		// it will wait  here until the frame is deemed available, meaning the other queues (in this case only the compute queue) 
		// dont have to create they're own fences that they need to keep track of.
		// The inflight fence basically tells us when a frame has finished rendering, and the submitToQueueSynced function 
		// signals the fence, informing it and thus us of the same.

		uint32_t index = m_vulkanManager->getIndex();
		
		// Graphics Command Buffer Synchronization
		VkSemaphore waitSemaphores_graphics[] = { m_vulkanManager->getImageAvailableVkSemaphore() };
		VkPipelineStageFlags waitStages_graphics[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		VkSemaphore signalSemaphores_graphics[] = { m_forwardRenderOperationsFinishedSemaphores[index] };
		VkFence inFlightFence_graphics = m_vulkanManager->getInFlightFence();

		// Compute Command Buffer Synchronization
		VkSemaphore waitSemaphores_compute[] = { m_forwardRenderOperationsFinishedSemaphores[index] };
		VkPipelineStageFlags waitStages_compute[] = { VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT };
		VkSemaphore signalSemaphores_compute[] = { m_computeOperationsFinishedSemaphores[index] };

		// Post Process Command Buffer Synchronization
		VkSemaphore waitSemaphores_postProcess[] = { m_forwardRenderOperationsFinishedSemaphores[index] };// , m_computeOperationsFinishedSemaphores[index] }; //
		VkPipelineStageFlags waitStages_postProcess[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		VkSemaphore signalSemaphores_postProcess[] = { m_postProcessFinishedSemaphores[index] };
		
		VulkanCommandUtil::submitToQueueSynced(	m_graphicsQueue, 1, &m_graphicsCommandBuffers[index],
			1, waitSemaphores_graphics, waitStages_graphics, 1, signalSemaphores_graphics, inFlightFence_graphics);
		
		//VulkanCommandUtil::submitToQueueSynced(m_computeQueue, 1, &m_computeCommandBuffers[index],
		//	1, waitSemaphores_compute, waitStages_compute, 1, signalSemaphores_compute, nullptr);
		VulkanCommandUtil::submitToQueueSynced(m_computeQueue, 1, &m_computeCommandBuffers[index], 0, nullptr, waitStages_compute, 1, signalSemaphores_compute, nullptr);

		VulkanCommandUtil::submitToQueueSynced(	m_graphicsQueue, 1, &m_postProcessCommandBuffers[index],
			1, waitSemaphores_postProcess, waitStages_postProcess, 1, signalSemaphores_postProcess, nullptr);
	}
}

inline void VulkanRendererBackend::createSyncObjects()
{
	// There are two ways of synchronizing swap chain events: fences and semaphores. 
	// The difference is that the state of fences can be accessed from your program using calls like vkWaitForFences and semaphores cannot be. 
	// Fences are mainly designed to synchronize your application itself with rendering operation
	// Semaphores are used to synchronize operations within or across command queues. 

	// Can synchronize between queues by using certain supported features
	// VkEvent: Versatile waiting, but limited to a single queue
	// VkSemaphore: GPU to GPU synchronization
	// vkFence: GPU to CPU synchronization

	// Create Semaphores
	m_forwardRenderOperationsFinishedSemaphores.resize(m_numSwapChainImages);
	m_computeOperationsFinishedSemaphores.resize(m_numSwapChainImages);
	m_postProcessFinishedSemaphores.resize(m_numSwapChainImages);

	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	for (uint32_t i = 0; i < m_numSwapChainImages; i++)
	{
		if (vkCreateSemaphore(m_logicalDevice, &semaphoreInfo, nullptr, &m_forwardRenderOperationsFinishedSemaphores[i]) != VK_SUCCESS ||
			vkCreateSemaphore(m_logicalDevice, &semaphoreInfo, nullptr, &m_computeOperationsFinishedSemaphores[i]) != VK_SUCCESS ||
			vkCreateSemaphore(m_logicalDevice, &semaphoreInfo, nullptr, &m_postProcessFinishedSemaphores[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create synchronization objects for a frame");
		}
	}
}

inline void VulkanRendererBackend::recordCommandBuffer_ComputeCmds(
	unsigned int frameIndex, VkCommandBuffer& ComputeCmdBuffer, std::shared_ptr<Scene> scene)
{
	// Test Compute Pass/Kernel
	{
		// Get compute texture
		std::shared_ptr<Texture2D> texture = scene->getTexture("compute", frameIndex);
		
		// Decide the size of the compute kernel
		// Number of threads in the compute kernel =  numBlocksX * numBlocksY * numBlocksZ
		const uint32_t numBlocksX = (texture->m_width + WORKGROUP_SIZE - 1) / WORKGROUP_SIZE;
		const uint32_t numBlocksY = (texture->m_height + WORKGROUP_SIZE - 1) / WORKGROUP_SIZE;
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
	unsigned int frameIndex, VkCommandBuffer& graphicsCmdBuffer, std::shared_ptr<Scene> scene, std::shared_ptr<Camera> camera,
	VkRect2D renderArea, uint32_t clearValueCount, const VkClearValue* clearValues)
{
	// Create a Image Memory Barrier between the compute pipeline that creates the image and the graphics pipeline that access the image
	// Image barriers should come before render passes begin, unless you're working with subpasses
	{
		std::shared_ptr<Texture2D> computeTexture = scene->getTexture("compute", frameIndex);
		VkImageSubresourceRange imageSubresourceRange = ImageUtil::createImageSubResourceRange(VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1);
		VkImageMemoryBarrier imageMemoryBarrier = ImageUtil::createImageMemoryBarrier(computeTexture->m_image,
			computeTexture->m_imageLayout, VK_IMAGE_LAYOUT_GENERAL,
			VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, imageSubresourceRange,
			m_vulkanManager->getQueueIndex(QueueFlags::Compute), m_vulkanManager->getQueueIndex(QueueFlags::Graphics));

		VulkanCommandUtil::pipelineBarrier(graphicsCmdBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
	}


	// Model Rendering Pipeline
	{
		const VkPipeline l_rasterP = getPipeline(PIPELINE_TYPE::RASTER);
		const VkPipelineLayout l_rasterPL = getPipelineLayout(PIPELINE_TYPE::RASTER);
		const VkDescriptorSet DS_camera = camera->getDescriptorSet(DSL_TYPE::CURRENT_FRAME_CAMERA, frameIndex);

		VulkanCommandUtil::beginRenderPass(graphicsCmdBuffer,
			m_rasterRPI.renderPass, m_rasterRPI.frameBuffers[frameIndex],
			renderArea, clearValueCount, clearValues);

		for (auto const& element : scene->m_modelMap)
		{	
			// Actual commands for the renderPass
			std::shared_ptr<Model> model = scene->getModel(element.first);
			model->recordDrawCmds(frameIndex, DS_camera, l_rasterP, l_rasterPL, graphicsCmdBuffer);
		}
		vkCmdEndRenderPass(graphicsCmdBuffer);
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
				renderArea, clearValueCount, clearValues);

			vkCmdBindDescriptorSets(graphicsCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, l_compositeComputeOntoRasterPL, 0, 1, &DS_compositeComputeOntoRaster, 0, nullptr);
			vkCmdBindPipeline(graphicsCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, l_compositeComputeOntoRasterP);
			vkCmdDraw(graphicsCmdBuffer, 3, 1, 0, 0);

			vkCmdEndRenderPass(graphicsCmdBuffer);
		}
	}
}
inline void VulkanRendererBackend::recordCommandBuffer_PostProcessCmds(
	unsigned int frameIndex, VkCommandBuffer& postProcessCmdBuffer, std::shared_ptr<Scene> scene,
	VkRect2D renderArea, uint32_t clearValueCount, const VkClearValue* clearValues)
{
	for (unsigned int postProcessIndex =0; postProcessIndex <m_numPostEffects; postProcessIndex++)
	{
		const VkPipeline l_Pipeline = m_postProcess_Ps[postProcessIndex];
		const VkPipelineLayout l_PipelineLayout = m_postProcess_PLs[postProcessIndex];
		const VkRenderPass l_renderPass = m_postProcessRPIs[postProcessIndex].renderPass;
		const VkFramebuffer l_frameBuffer = m_postProcessRPIs[postProcessIndex].frameBuffers[frameIndex];

		// Actual commands for the renderPass
		{
			VulkanCommandUtil::beginRenderPass(postProcessCmdBuffer, l_renderPass, l_frameBuffer, renderArea, clearValueCount, clearValues);
			
			const int numDescriptors = static_cast<int>(m_postProcessRPIs[postProcessIndex].descriptors.size() / 3);
			for (int i = 0; i < numDescriptors; i++)
			{
				const int index = i + frameIndex * numDescriptors;
				VkDescriptorSet DescSet = m_postProcessRPIs[postProcessIndex].descriptors[index];
				vkCmdBindDescriptorSets(postProcessCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, l_PipelineLayout, i, 1, &DescSet, 0, nullptr);
			}
			
			// Check requested push constant size against hardware limit
			// Specs require 128 bytes, so if the device complies our push constant buffer should always fit into memory
#ifdef DEBUG_MAGE_FRAMEWORK
			VkPhysicalDeviceProperties physicalDeviceProperties;
			vkGetPhysicalDeviceProperties(m_physicalDevice, &physicalDeviceProperties);
			assert(sizeof(shaderConstants) <= physicalDeviceProperties.limits.maxPushConstantsSize);
#endif
			// Submit shaderConstants via push constant (rather than a UBO)
			vkCmdPushConstants(	postProcessCmdBuffer, l_PipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(shaderConstants), &shaderConstants);

			vkCmdBindPipeline(postProcessCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, l_Pipeline);
			vkCmdDraw(postProcessCmdBuffer, 3, 1, 0, 0);

			vkCmdEndRenderPass(postProcessCmdBuffer);
		}
	}

	recordCommandBuffer_FinalCmds(frameIndex, postProcessCmdBuffer);
}
inline void VulkanRendererBackend::recordCommandBuffer_FinalCmds(
	unsigned int frameIndex, VkCommandBuffer& cmdBuffer)
{
	//--- Decoupling Post Process Passes from the swapchain ---
	// We are not going to be making the last post process effect write to the swapchain directly. 
	// This is because we want to decouple the post process passes, and hence their ordering, from the swapchain. 
	// Instead of creating a renderpass to copy the results of the last post process pass into the swapchain we simply use a copy image command.
	// The vkCmdCopyImage command requires that we do some image tranisitions for the source and destination images
	{
		const unsigned int fbaLowResLastUsed = (2 + (m_fbaLowResIndexInUse - 1)) % 2;
		VkImage& srcImage = m_fbaLowRes[fbaLowResLastUsed][frameIndex].image;

		VkImageLayout srcImageLayoutInitial = VK_IMAGE_LAYOUT_GENERAL;
		VkImageLayout srcImageLayoutForTransition = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

		VkImageLayout swapChainImageLayoutInitial = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; ;
		VkImageLayout swapChainImageLayoutBeforeUIPass = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		
		const uint32_t mipLevels = 1;
		// Pre-copy Transitions
		{
			// Transition Last post process pass's result image into VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
			ImageUtil::transitionImageLayout(m_logicalDevice, m_graphicsQueue, m_graphicsCommandPool, cmdBuffer,
				srcImage, m_lowResolutionRenderFormat, srcImageLayoutInitial, srcImageLayoutForTransition, mipLevels);

			// Transition swapchain to VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
			m_vulkanManager->transitionSwapChainImageLayout(frameIndex, swapChainImageLayoutInitial, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, cmdBuffer, m_graphicsCommandPool);
		}

		// Copy last post process pass's image into the swapchain image
		m_vulkanManager->copyImageToSwapChainImage(frameIndex, srcImage, cmdBuffer, m_graphicsCommandPool, m_windowExtents);

		// Post-copy Transitions
		{
			// Transition Last post process pass's result image back into it's original layout
			const VkAccessFlags srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
			const VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			const VkAccessFlags dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			const VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
			
			ImageUtil::transitionImageLayout(m_logicalDevice, m_graphicsQueue, m_graphicsCommandPool, cmdBuffer,
				srcImage, m_lowResolutionRenderFormat, srcImageLayoutForTransition, srcImageLayoutInitial, mipLevels,
				srcAccessMask, dstAccessMask, srcStageMask, dstStageMask);

			// Transition swapchain to the VkImageLayout expected by our UI manager
			m_vulkanManager->transitionSwapChainImageLayout(frameIndex, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, swapChainImageLayoutBeforeUIPass, cmdBuffer, m_graphicsCommandPool);
		}
	}
}