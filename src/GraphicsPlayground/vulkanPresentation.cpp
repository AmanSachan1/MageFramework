#include "vulkanPresentation.h"

VulkanPresentation::VulkanPresentation(VulkanDevices* _devices, VkSurfaceKHR& _vkSurface, uint32_t  width, uint32_t height)
	: m_vulkanDevices(_devices), m_surface(_vkSurface)
{
	create(width, height);
	createSemaphores();
}

VulkanPresentation::~VulkanPresentation()
{
	vkDestroySemaphore(m_vulkanDevices->getLogicalDevice(), m_imageAvailableSemaphore, nullptr);
	vkDestroySemaphore(m_vulkanDevices->getLogicalDevice(), m_renderFinishedSemaphore, nullptr);

	for (auto imageView : m_swapChainImageViews)
	{
		vkDestroyImageView(m_vulkanDevices->getLogicalDevice(), imageView, nullptr);
	}
	vkDestroySwapchainKHR(m_vulkanDevices->getLogicalDevice(), m_swapChain, nullptr);
}

void VulkanPresentation::create(uint32_t  width, uint32_t height)
{
	VkExtent2D extent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
	createSwapChain(extent);
	createImageViews();
}

void VulkanPresentation::recreate(uint32_t  width, uint32_t height)
{
	vkDestroySwapchainKHR(m_vulkanDevices->getLogicalDevice(), m_swapChain, nullptr);
	create(width, height);
}

void VulkanPresentation::createSwapChain(VkExtent2D extent)
{
	const auto& surfaceCapabilities = m_vulkanDevices->surfaceCapabilities;
	VkSurfaceFormatKHR surfaceFormat = SwapChainUtils::chooseSwapSurfaceFormat(m_vulkanDevices->surfaceFormats);
	VkPresentModeKHR presentMode = SwapChainUtils::chooseSwapPresentMode(m_vulkanDevices->presentModes);

	// Can do multiple buffering here!
	// minImageCount is almost definitely 1 (or it doesnt support presentation)
	// setting imageCount to 2 makes it possible to do double buffering
	uint32_t imageCount = surfaceCapabilities.minImageCount + 1;
	// 0 is a special value for maxImageCount, that means that there is no maximum
	if (surfaceCapabilities.maxImageCount > 0 && imageCount > surfaceCapabilities.maxImageCount)
	{
		imageCount = surfaceCapabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR swapChainCreateInfo = VulkanInitializers::basicSwapChainCreateInfo(
		m_surface, imageCount, surfaceFormat.format, surfaceFormat.colorSpace,
		extent, 1, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		surfaceCapabilities.currentTransform,
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

void VulkanPresentation::createSemaphores()
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
	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	if (vkCreateSemaphore(m_vulkanDevices->getLogicalDevice(), &semaphoreInfo, nullptr, &m_imageAvailableSemaphore) != VK_SUCCESS ||
		vkCreateSemaphore(m_vulkanDevices->getLogicalDevice(), &semaphoreInfo, nullptr, &m_renderFinishedSemaphore) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create semaphores");
	}
}

void VulkanPresentation::acquireNextSwapChainImage(VkDevice& logicalDevice)
{
	// It is possible to use a semaphore, fence, or both as the synchronization objects that are to be signaled 
	// when the presentation engine is finished using the image
	vkAcquireNextImageKHR(logicalDevice, m_swapChain, std::numeric_limits<uint64_t>::max(),
		m_imageAvailableSemaphore, VK_NULL_HANDLE, &m_imageIndex);
}
void VulkanPresentation::presentImageToSwapChain(VkDevice& logicalDevice)
{
	VkSemaphore signalSemaphores[] = { m_renderFinishedSemaphore };

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &m_swapChain;
	presentInfo.pImageIndices = &m_imageIndex;
	presentInfo.pResults = nullptr; // Optional

	VkResult result = vkQueuePresentKHR(m_vulkanDevices->getQueue(QueueFlags::Present), &presentInfo);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) 
	{
		//Recreate(vkSwapChainExtent.width, vkSwapChainExtent.height);
	}
	else if (result != VK_SUCCESS) 
	{
		throw std::runtime_error("Failed to present swap chain image");
	}
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
VkSemaphore VulkanPresentation::getImageAvailableVkSemaphore() const
{
	return m_imageAvailableSemaphore;
}
VkSemaphore VulkanPresentation::getRenderFinishedVkSemaphore() const
{
	return m_renderFinishedSemaphore;
}