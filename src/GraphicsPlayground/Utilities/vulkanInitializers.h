#pragma once
#include <vector>
#include <vulkan/vulkan.h>

namespace VulkanInitializers
{
	//--------------------------------------------------------
	//					  Swap Chain
	//--------------------------------------------------------

	inline VkSwapchainCreateInfoKHR basicSwapChainCreateInfo(VkSurfaceKHR vkSurface,
		uint32_t minImageCount, VkFormat imageFormat, VkColorSpaceKHR imageColorSpace,
		VkExtent2D imageExtent, uint32_t imageArrayLayers, VkImageUsageFlags imageUsage,
		VkSurfaceTransformFlagBitsKHR preTransform, VkCompositeAlphaFlagBitsKHR compositeAlpha,
		VkPresentModeKHR presentMode, VkSwapchainKHR oldSwapchain)
	{
		// --- Create logical device ---
		VkSwapchainCreateInfoKHR swapchainCreateInfo = {};
		swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		// specify the surface the swapchain will be tied to
		swapchainCreateInfo.surface = vkSurface;

		swapchainCreateInfo.minImageCount = minImageCount;
		swapchainCreateInfo.imageFormat = imageFormat;
		swapchainCreateInfo.imageColorSpace = imageColorSpace;
		swapchainCreateInfo.imageExtent = imageExtent;
		swapchainCreateInfo.imageArrayLayers = imageArrayLayers;
		swapchainCreateInfo.imageUsage = imageUsage;

		// Specify transform on images in the swap chain
		// VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR == no transformations
		swapchainCreateInfo.preTransform = preTransform;
		// Specify alpha channel usage 
		swapchainCreateInfo.compositeAlpha = compositeAlpha;
		swapchainCreateInfo.presentMode = presentMode;
		// Reference to old swap chain in case current one becomes invalid
		swapchainCreateInfo.oldSwapchain = oldSwapchain;

		return swapchainCreateInfo;
	}

	inline void createSwapChain(VkDevice logicalDevice, VkSwapchainCreateInfoKHR& swapchainCreateInfo, VkSwapchainKHR& vkSwapChain)
	{
		if (vkCreateSwapchainKHR(logicalDevice, &swapchainCreateInfo, nullptr, &vkSwapChain) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create swap chain");
		}
	}

	inline VkImageViewCreateInfo basicImageViewCreateInfo(VkImage& image, VkImageViewType viewType, VkFormat format)
	{
		VkImageViewCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = image;

		createInfo.viewType = viewType;
		createInfo.format = format;

		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

		//No Mipmapping and no multiple targets
		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;

		return createInfo;
	}

	inline void createImageView(VkDevice logicalDevice, VkImageView* imageView, const VkImageViewCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator)
	{
		if (vkCreateImageView(logicalDevice, pCreateInfo, pAllocator, imageView) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create image views!");
		}
	}

	//--------------------------------------------------------
	//			Descriptor Sets and Descriptor Layouts
	// Reference: https://vulkan-tutorial.com/Uniform_buffers
	//--------------------------------------------------------

	inline void CreateDescriptorPool(VkDevice& logicalDevice, uint32_t poolSizeCount, VkDescriptorPoolSize* data, VkDescriptorPool& descriptorPool)
	{
		VkDescriptorPoolCreateInfo descriptorPoolInfo = {};
		descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		descriptorPoolInfo.pNext = nullptr;
		descriptorPoolInfo.poolSizeCount = poolSizeCount;
		descriptorPoolInfo.pPoolSizes = data;
		descriptorPoolInfo.maxSets = poolSizeCount;

		if (vkCreateDescriptorPool(logicalDevice, &descriptorPoolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create descriptor pool");
		}
	}

	inline VkDescriptorSet CreateDescriptorSet(VkDevice& logicalDevice, VkDescriptorPool descriptorPool, VkDescriptorSetLayout descriptorSetLayout)
	{
		VkDescriptorSetAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = descriptorPool;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &descriptorSetLayout;

		VkDescriptorSet descriptorSet;
		vkAllocateDescriptorSets(logicalDevice, &allocInfo, &descriptorSet);
		return descriptorSet;
	}
};

//--------------------------------------------------------
//			Miscellaneous Vulkan Structures
//--------------------------------------------------------

namespace VulkanUtil
{
	inline VkViewport createViewport(float x, float y, float width, float height, float minDepth, float maxDepth)
	{
		return { x, y, width, height, minDepth, maxDepth };
	}

	inline VkRect2D createRectangle(VkOffset2D offset, VkExtent2D extent)
	{
		return { offset, extent };
	}

	inline void submitToGraphicsQueue( VkQueue graphicsQueue,
		uint32_t waitSemaphoreCount, const VkSemaphore* pWaitSemaphores, 
		const VkPipelineStageFlags* pWaitDstStageMask,
		uint32_t commandBufferCount, const VkCommandBuffer* pCommandBuffers,
		uint32_t signalSemaphoreCount, const VkSemaphore* pSignalSemaphores,
		VkFence inFlightFence)
	{
		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		// These parameters specify which semaphores to wait on before execution begins and in which stage(s) of the pipeline to wait
		// Each entry in the waitStages array corresponds to the semaphore with the same index in pWaitSemaphores
		submitInfo.waitSemaphoreCount = waitSemaphoreCount;
		submitInfo.pWaitSemaphores = pWaitSemaphores;
		submitInfo.pWaitDstStageMask = pWaitDstStageMask;

		submitInfo.commandBufferCount = commandBufferCount;
		submitInfo.pCommandBuffers = pCommandBuffers;
		submitInfo.signalSemaphoreCount = signalSemaphoreCount;
		submitInfo.pSignalSemaphores = pSignalSemaphores;

		if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFence) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to submit graphics command buffer to graphics queue!");
		}
	}

	inline void submitToGraphicsQueue(VkQueue graphicsQueue, uint32_t cmdBufferCount, const VkCommandBuffer& cmdBuffer)
	{
		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = cmdBufferCount;
		submitInfo.pCommandBuffers = &cmdBuffer;

		// Unlike the draw commands, there are no events we need to wait on this time. 
		// We just want to execute the transfer on the buffers immediately. 
		// There are again two possible ways to wait on this transfer to complete. 
		// We could use a fence and wait with vkWaitForFences, or simply wait for the transfer queue to become idle with vkQueueWaitIdle. 
		// A fence would allow you to schedule multiple transfers simultaneously and wait for all of them complete, 
		// instead of executing one at a time. That may give the driver more opportunities to optimize.

		if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to submit command buffer to graphics queue!");
		}

		vkQueueWaitIdle(graphicsQueue);
	}
};
