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

	VulkanCommandUtil::createCommandPool(m_logicalDevice, m_computeCmdPool, m_vulkanManager->getQueueIndex(QueueFlags::Compute));
	VulkanCommandUtil::createCommandPool(m_logicalDevice, m_graphicsCmdPool, m_vulkanManager->getQueueIndex(QueueFlags::Graphics));
	recreateCommandBuffers();
}
inline void VulkanRendererBackend::recreateCommandBuffers()
{
	m_computeCommandBuffers.resize(m_numSwapChainImages);
	m_graphicsCommandBuffers.resize(m_numSwapChainImages);
	m_rayTracingCommandBuffers.resize(m_numSwapChainImages);
	m_postProcessCommandBuffers.resize(m_numSwapChainImages);

	VulkanCommandUtil::allocateCommandBuffers(m_logicalDevice, m_computeCmdPool, m_computeCommandBuffers);
	VulkanCommandUtil::allocateCommandBuffers(m_logicalDevice, m_graphicsCmdPool, m_graphicsCommandBuffers);
	VulkanCommandUtil::allocateCommandBuffers(m_logicalDevice, m_graphicsCmdPool, m_rayTracingCommandBuffers);
	VulkanCommandUtil::allocateCommandBuffers(m_logicalDevice, m_graphicsCmdPool, m_postProcessCommandBuffers);
}
inline void VulkanRendererBackend::submitCommandBuffers()
{
	// We don't want to write to resources like images and buffers if they are currently in use, and hence not available. Thus we specify 
	// the stage of a pipeline that we can write to resources in.
	// e.g In a graphics pipeline we qould want to wait with writing colors to the render image until it's available, so we
	// specify the 'VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT' stage, as the stage we should wait on until the image is available for writing
	// That means that theoretically the implementation can already start executing our vertex shader and such while the image is not available yet.
	
	// There is a fence attached to the submission of commands to the graphics queue. Since the fence is a CPU-GPU synchronization
	// it will wait  here until the frame is deemed available, meaning the other queues (in this case only the compute queue) 
	// dont have to create they're own fences that they need to keep track of.
	// The inflight fence basically tells us when a frame has finished rendering, and the submitToQueueSynced function 
	// signals the fence, informing it and thus us of the same.

	uint32_t index = m_vulkanManager->getImageIndex();
	VkFence inFlightFence = m_vulkanManager->getInFlightFence();

	//	Submit Commands
	{
		if (m_rendererOptions.renderType == RENDER_TYPE::RASTERIZATION)
		{
			// Graphics Command Buffer Synchronization
			VkSemaphore waitSemaphores_rasterization[] = { m_vulkanManager->getImageAvailableVkSemaphore() };
			VkPipelineStageFlags waitStages_rasterization[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
			VkSemaphore signalSemaphores_rasterization[] = { m_renderOperationsFinishedSemaphores[index] };

			VulkanCommandUtil::submitToQueueSynced(m_graphicsQueue, 1, &m_graphicsCommandBuffers[index],
				1, waitSemaphores_rasterization, waitStages_rasterization, 1, signalSemaphores_rasterization, inFlightFence);
		}
		else if (m_rendererOptions.renderType == RENDER_TYPE::RAYTRACE)
		{
			// Ray Tracing Command Buffer Synchronization
			VkSemaphore waitSemaphores_rayTracing[] = { m_vulkanManager->getImageAvailableVkSemaphore() };
			VkPipelineStageFlags waitStages_rayTracing[] = { VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_NV };
			VkSemaphore signalSemaphores_rayTracing[] = { m_renderOperationsFinishedSemaphores[index] };

			VulkanCommandUtil::submitToQueueSynced(m_graphicsQueue, 1, &m_rayTracingCommandBuffers[index],
				1, waitSemaphores_rayTracing, waitStages_rayTracing, 1, signalSemaphores_rayTracing, inFlightFence);
		}

		// Compute Command Buffer Synchronization
		VkSemaphore waitSemaphores_compute[] = { m_renderOperationsFinishedSemaphores[index] };
		VkPipelineStageFlags waitStages_compute[] = { VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT };
		VkSemaphore signalSemaphores_compute[] = { m_computeOperationsFinishedSemaphores[index] };

		// Post Process Command Buffer Synchronization
		VkSemaphore waitSemaphores_postProcess[] = { m_computeOperationsFinishedSemaphores[index] };
		VkPipelineStageFlags waitStages_postProcess[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		VkSemaphore signalSemaphores_postProcess[] = { m_postProcessFinishedSemaphores[index] };
		
		VulkanCommandUtil::submitToQueueSynced(m_computeQueue, 1, &m_computeCommandBuffers[index],
			1, waitSemaphores_compute, waitStages_compute, 1, signalSemaphores_compute, nullptr);

		VulkanCommandUtil::submitToQueueSynced(m_graphicsQueue, 1, &m_postProcessCommandBuffers[index],
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
	m_renderOperationsFinishedSemaphores.resize(m_numSwapChainImages);
	m_computeOperationsFinishedSemaphores.resize(m_numSwapChainImages);
	m_postProcessFinishedSemaphores.resize(m_numSwapChainImages);

	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	for (uint32_t i = 0; i < m_numSwapChainImages; i++)
	{
		if (vkCreateSemaphore(m_logicalDevice, &semaphoreInfo, nullptr, &m_renderOperationsFinishedSemaphores[i]) != VK_SUCCESS ||
			vkCreateSemaphore(m_logicalDevice, &semaphoreInfo, nullptr, &m_computeOperationsFinishedSemaphores[i]) != VK_SUCCESS ||
			vkCreateSemaphore(m_logicalDevice, &semaphoreInfo, nullptr, &m_postProcessFinishedSemaphores[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create synchronization objects for a frame");
		}
	}
}

inline void VulkanRendererBackend::recordAllCommandBuffers(std::shared_ptr<Camera> camera, std::shared_ptr<Scene> scene)
{
	const uint32_t numClearValues = 2;
	std::array<VkClearValue, numClearValues> clearValues = {};
	clearValues[0].color = { 0.412f, 0.796f, 1.0f, 1.0f };
	clearValues[1].depthStencil = { 1.0f, 0 };

	const VkRect2D renderArea = Util::createRectangle(m_vulkanManager->getSwapChainVkExtent());
	const unsigned int numCommandBuffers = m_vulkanManager->getSwapChainImageCount();
	for (unsigned int i = 0; i < numCommandBuffers; i++)
	{
		VkCommandBuffer& computeCmdBuffer = m_computeCommandBuffers[i];
		VkCommandBuffer& rayTracingCmdBuffer = m_rayTracingCommandBuffers[i];
		VkCommandBuffer& graphicsCmdBuffer = m_graphicsCommandBuffers[i];
		VkCommandBuffer& postProcessCmdBuffer = m_postProcessCommandBuffers[i];
		
		VulkanCommandUtil::beginCommandBuffer(computeCmdBuffer);
		recordCommandBuffer_ComputeCmds(i, computeCmdBuffer, scene);
		VulkanCommandUtil::endCommandBuffer(computeCmdBuffer);

		if (m_rendererOptions.renderType == RENDER_TYPE::RAYTRACE)
		{
			VulkanCommandUtil::beginCommandBuffer(rayTracingCmdBuffer);
			recordCommandBuffer_rayTracingCmds(i, rayTracingCmdBuffer, camera, scene);
			VulkanCommandUtil::endCommandBuffer(rayTracingCmdBuffer);
		}
		else if (m_rendererOptions.renderType == RENDER_TYPE::RASTERIZATION)
		{
			VulkanCommandUtil::beginCommandBuffer(graphicsCmdBuffer);
			recordCommandBuffer_GraphicsCmds(i, graphicsCmdBuffer, scene, camera, renderArea, numClearValues, clearValues.data());
			VulkanCommandUtil::endCommandBuffer(graphicsCmdBuffer);
		}

		VulkanCommandUtil::beginCommandBuffer(postProcessCmdBuffer);
		recordCommandBuffer_PostProcessCmds(i, postProcessCmdBuffer, scene, renderArea, numClearValues, clearValues.data());
		recordCommandBuffer_FinalCmds(i, postProcessCmdBuffer);
		VulkanCommandUtil::endCommandBuffer(postProcessCmdBuffer);
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
		
		const VkDescriptorSet DS_compute = scene->getDescriptorSet(DSL_TYPE::COMPUTE, frameIndex);

		vkCmdBindPipeline(ComputeCmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_compute_P);
		vkCmdBindDescriptorSets(ComputeCmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_compute_PL, 0, 1, &DS_compute, 0, nullptr);
		vkCmdDispatch(ComputeCmdBuffer, numBlocksX, numBlocksY, numBlocksZ); // Dispatch the Compute Kernel
	}
}

inline void VulkanRendererBackend::recordCommandBuffer_rayTracingCmds(
	unsigned int frameIndex, VkCommandBuffer& rayTracingCmdBuffer, std::shared_ptr<Camera> camera, std::shared_ptr<Scene> scene)
{
	uint32_t width = m_vulkanManager->getSwapChainVkExtent().width;
	uint32_t height = m_vulkanManager->getSwapChainVkExtent().height;
	const unsigned int numCommandBuffers = m_vulkanManager->getSwapChainImageCount();
	VkImageSubresourceRange subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

	for (unsigned int i = 0; i < numCommandBuffers; i++)
	{
		// Dispatch the ray tracing commands
		vkCmdBindPipeline(rayTracingCmdBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_NV, m_rayTrace_P);
		vkCmdBindDescriptorSets(rayTracingCmdBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_NV, m_rayTrace_PL, 0, 1, &m_DS_rayTrace[i], 0, 0);

		// Calculate shader binding offsets, which is pretty straight forward in our example 
		VkDeviceSize bindingOffsetRayGenShader = m_rayTracingProperties.shaderGroupHandleSize * INDEX_RAYGEN;
		VkDeviceSize bindingOffsetMissShader = m_rayTracingProperties.shaderGroupHandleSize * INDEX_MISS;
		VkDeviceSize bindingOffsetHitShader = m_rayTracingProperties.shaderGroupHandleSize * INDEX_CLOSEST_HIT;
		VkDeviceSize bindingStride = m_rayTracingProperties.shaderGroupHandleSize;

		vkCmdTraceRaysNV(rayTracingCmdBuffer,
			m_shaderBindingTable.buffer, bindingOffsetRayGenShader,              // Ray Gen Shader Binding
			m_shaderBindingTable.buffer, bindingOffsetMissShader, bindingStride, // Miss Shader Binding
			m_shaderBindingTable.buffer, bindingOffsetHitShader, bindingStride,  // Hit Shader Binding
			VK_NULL_HANDLE, 0, 0,                                              // Callable Shader Binding
			width, height, 1);
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
		const VkDescriptorSet DS_camera = camera->getDescriptorSet(frameIndex);

		VulkanCommandUtil::beginRenderPass(graphicsCmdBuffer,
			m_rasterRPI.renderPass, m_rasterRPI.frameBuffers[frameIndex],
			renderArea, clearValueCount, clearValues);

		for (auto const& element : scene->m_modelMap)
		{	
			// Actual commands for the renderPass
			std::shared_ptr<Model> model = scene->getModel(element.first);
			model->recordDrawCmds(frameIndex, DS_camera, m_rasterization_P, m_rasterization_PL, graphicsCmdBuffer);
		}
		vkCmdEndRenderPass(graphicsCmdBuffer);
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
			ImageUtil::transitionImageLayout(m_logicalDevice, m_graphicsQueue, m_graphicsCmdPool, cmdBuffer,
				srcImage, m_lowResolutionRenderFormat, srcImageLayoutInitial, srcImageLayoutForTransition, mipLevels);

			// Transition swapchain to VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
			m_vulkanManager->transitionSwapChainImageLayout(frameIndex, swapChainImageLayoutInitial, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, cmdBuffer, m_graphicsCmdPool);
		}

		// Copy last post process pass's image into the swapchain image
		m_vulkanManager->copyImageToSwapChainImage(frameIndex, srcImage, cmdBuffer, m_graphicsCmdPool, m_windowExtents);

		// Post-copy Transitions
		{
			// Transition Last post process pass's result image back into it's original layout
			const VkAccessFlags srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
			const VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			const VkAccessFlags dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			const VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
			
			ImageUtil::transitionImageLayout(m_logicalDevice, m_graphicsQueue, m_graphicsCmdPool, cmdBuffer,
				srcImage, m_lowResolutionRenderFormat, srcImageLayoutForTransition, srcImageLayoutInitial, mipLevels,
				srcAccessMask, dstAccessMask, srcStageMask, dstStageMask);

			// Transition swapchain to the VkImageLayout expected by our UI manager
			m_vulkanManager->transitionSwapChainImageLayout(frameIndex, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, swapChainImageLayoutBeforeUIPass, cmdBuffer, m_graphicsCmdPool);
		}
	}
}