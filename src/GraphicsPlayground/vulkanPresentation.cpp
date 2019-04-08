#include "vulkanPresentation.h"

VulkanPresentation::VulkanPresentation(VulkanDevices* _devices, VkSurfaceKHR& _vkSurface, GLFWwindow* _window)
	: m_vulkanDevices(_devices), m_surface(_vkSurface)
{
	create(_window);
	createSyncObjects();
}

VulkanPresentation::~VulkanPresentation()
{
	cleanup();

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) 
	{
		vkDestroySemaphore(m_vulkanDevices->getLogicalDevice(), m_renderFinishedSemaphores[i], nullptr);
		vkDestroySemaphore(m_vulkanDevices->getLogicalDevice(), m_imageAvailableSemaphores[i], nullptr);
		vkDestroyFence(m_vulkanDevices->getLogicalDevice(), m_inFlightFences[i], nullptr);
	}	
}

void VulkanPresentation::create(GLFWwindow* window)
{
	createSwapChain(window);
	createImageViews();
}

void VulkanPresentation::cleanup()
{
	for (size_t i = 0; i < m_swapChainImageViews.size(); i++)
	{
		vkDestroyImageView(m_vulkanDevices->getLogicalDevice(), m_swapChainImageViews[i], nullptr);
	}
	vkDestroySwapchainKHR(m_vulkanDevices->getLogicalDevice(), m_swapChain, nullptr);
}

void VulkanPresentation::createSwapChain(GLFWwindow* window)
{
	SwapChainSupportDetails swapChainSupport = m_vulkanDevices->querySwapChainSupport();

	VkSurfaceFormatKHR surfaceFormat = SwapChainUtils::chooseSwapSurfaceFormat(swapChainSupport.surfaceFormats);
	VkPresentModeKHR presentMode = SwapChainUtils::chooseSwapPresentMode(swapChainSupport.presentModes);
	VkExtent2D extent = SwapChainUtils::chooseSwapExtent(swapChainSupport.surfaceCapabilities, window);

	// Can do multiple buffering here!
	// minImageCount is almost definitely 1 (or it doesnt support presentation)
	// setting imageCount to 2 makes it possible to do double buffering
	uint32_t imageCount = swapChainSupport.surfaceCapabilities.minImageCount + 1;
	// 0 is a special value for maxImageCount, that means that there is no maximum
	if (swapChainSupport.surfaceCapabilities.maxImageCount > 0 && imageCount > swapChainSupport.surfaceCapabilities.maxImageCount)
	{
		imageCount = swapChainSupport.surfaceCapabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR swapChainCreateInfo = VulkanInitializers::basicSwapChainCreateInfo(
		m_surface, imageCount, surfaceFormat.format, surfaceFormat.colorSpace,
		extent, 1, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		swapChainSupport.surfaceCapabilities.currentTransform,
		VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		presentMode,
		VK_NULL_HANDLE);

	const auto& queueFamilyIndices = m_vulkanDevices->getQueueFamilyIndices();
	if (queueFamilyIndices[QueueFlags::Graphics] != queueFamilyIndices[QueueFlags::Present])
	{
		// Images can be used across multiple queue families without explicit ownership transfers
		swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swapChainCreateInfo.queueFamilyIndexCount = 2;
		unsigned int indices[] = { queueFamilyIndices[QueueFlags::Graphics], queueFamilyIndices[QueueFlags::Present] };
		swapChainCreateInfo.pQueueFamilyIndices = indices;
	}
	else
	{
		// An image is owned by one queue family at a time and ownership must be explicitly transfered between uses
		swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapChainCreateInfo.queueFamilyIndexCount = 0;
		swapChainCreateInfo.pQueueFamilyIndices = nullptr;
	}

	// Specify whether we can clip pixels that are obscured by other windows
	swapChainCreateInfo.clipped = VK_TRUE;

	VulkanInitializers::createSwapChain(m_vulkanDevices->getLogicalDevice(), swapChainCreateInfo, m_swapChain);

	// --- Retrieve swap chain images ---
	vkGetSwapchainImagesKHR(m_vulkanDevices->getLogicalDevice(), m_swapChain, &imageCount, nullptr);
	m_swapChainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(m_vulkanDevices->getLogicalDevice(), m_swapChain, &imageCount, m_swapChainImages.data());

	m_swapChainImageFormat = surfaceFormat.format;
	m_swapChainExtent = extent;
}

void VulkanPresentation::createImageViews() 
{
	m_swapChainImageViews.resize(m_swapChainImages.size());

	for (int i = 0; i < m_swapChainImages.size(); i++)
	{
		VkImageViewCreateInfo swapChainImageViewCreateInfo =
			VulkanInitializers::basicImageViewCreateInfo(m_swapChainImages[i], VK_IMAGE_VIEW_TYPE_2D, m_swapChainImageFormat);

		VulkanInitializers::createImageView(m_vulkanDevices->getLogicalDevice(), &m_swapChainImageViews[i], &swapChainImageViewCreateInfo, nullptr);
	}
}

void VulkanPresentation::createSyncObjects()
{
	// There are two ways of synchronizing swap chain events: fences and semaphores. 
	// The difference is that the state of fences can be accessed from your program using calls like vkWaitForFences and semaphores cannot be. 
	// Fences are mainly designed to synchronize your application itself with rendering operation
	// Semaphores are used to synchronize operations within or across command queues. 

	// Can synchronize between queues by using certain supported features
	// VkEvent: Versatile waiting, but limited to a single queue
	// VkSemaphore: GPU to GPU synchronization
	// vkFence: GPU to CPU synchronization


	// A render loop will perform the following operations:
	// - Acquire an image from the swap chain
	// - Execute the command buffer with that image as attachment in the framebuffer
	// - Return the image to the swap chain for presentation

	// Each of these operations are executed asynchronously. 
	// The function calls associated with the operations return before the operations actually finish executing.
	// The order of their execution is also undefined but each of the operations depends on the previous one finishing.
	// We want to synchronize the queue operations of draw commands and presentation, which makes semaphores the best fit.

	
	// Create Semaphores
	m_imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	m_renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	m_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo = {};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) 
	{
		if (vkCreateSemaphore(m_vulkanDevices->getLogicalDevice(), &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i]) != VK_SUCCESS ||
			vkCreateSemaphore(m_vulkanDevices->getLogicalDevice(), &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i]) != VK_SUCCESS ||
			vkCreateFence(m_vulkanDevices->getLogicalDevice(), &fenceInfo, nullptr, &m_inFlightFences[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create synchronization objects for a frame");
		}
	}
}

bool VulkanPresentation::acquireNextSwapChainImage(VkDevice& logicalDevice)
{
	// It is possible to use a semaphore, fence, or both as the synchronization objects that are to be signaled 
	// when the presentation engine is finished using the image
	VkResult result = vkAcquireNextImageKHR(logicalDevice, m_swapChain, std::numeric_limits<uint64_t>::max(),
		m_imageAvailableSemaphores[m_currentFrameIndex], VK_NULL_HANDLE, &m_imageIndex);

	// The vkAcquireNextImageKHR function can return the following special values to indicate that it's time to recreate the swapchain.
	// -- VK_ERROR_OUT_OF_DATE_KHR: The swap chain has become incompatible with the surface and can no longer be used for rendering.
	//								Usually happens after a window resize.
	// -- VK_SUBOPTIMAL_KHR : The swap chain can still be used to successfully present to the surface, but the surface properties are
	//						  no longer matched exactly.

	if (result == VK_ERROR_OUT_OF_DATE_KHR) 
	{
		return false;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
	{
		throw std::runtime_error("failed to acquire swap chain image!");
	}
	return true;
}
bool VulkanPresentation::presentImageToSwapChain(VkDevice& logicalDevice, bool frameBufferResized)
{
	VkSemaphore signalSemaphores[] = { m_renderFinishedSemaphores[m_currentFrameIndex] };

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &m_swapChain;
	presentInfo.pImageIndices = &m_imageIndex;
	presentInfo.pResults = nullptr; // Optional

	VkResult result = vkQueuePresentKHR(m_vulkanDevices->getQueue(QueueFlags::Present), &presentInfo);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || frameBufferResized)
	{
		frameBufferResized = false;
		return false;
	}
	else if (result != VK_SUCCESS) 
	{
		throw std::runtime_error("Failed to present swap chain image");
	}
	return true;
}

void VulkanPresentation::advanceCurrentFrameIndex()
{
	m_currentFrameIndex = (m_currentFrameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
}

//----------
//Getters
//----------
VkSwapchainKHR VulkanPresentation::getVulkanSwapChain() const
{
	return m_swapChain;
}
VkImage VulkanPresentation::getVkImage(uint32_t index) const
{
	return m_swapChainImages[index];
}
VkImageView VulkanPresentation::getVkImageView(uint32_t index) const
{
	return m_swapChainImageViews[index];
}
VkFormat VulkanPresentation::getVkImageFormat() const
{
	return m_swapChainImageFormat;
}
VkExtent2D VulkanPresentation::getVkExtent() const
{
	return m_swapChainExtent;
}
uint32_t VulkanPresentation::getCount() const
{
	return static_cast<uint32_t>(m_swapChainImages.size());
}
uint32_t VulkanPresentation::getIndex() const
{
	return m_imageIndex;
}
uint32_t VulkanPresentation::getFrameNumber() const
{
	return m_currentFrameIndex;
}
VkSemaphore VulkanPresentation::getImageAvailableVkSemaphore() const
{
	return m_imageAvailableSemaphores[m_currentFrameIndex];
}
VkSemaphore VulkanPresentation::getRenderFinishedVkSemaphore() const
{
	return m_renderFinishedSemaphores[m_currentFrameIndex];
}
VkFence VulkanPresentation::getInFlightFence() const
{
	return m_inFlightFences[m_currentFrameIndex];
}