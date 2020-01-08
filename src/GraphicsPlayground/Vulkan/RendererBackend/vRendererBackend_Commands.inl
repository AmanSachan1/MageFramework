#pragma once
#include <Vulkan\RendererBackend\vRendererBackend.h>

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