#include "vulkanPresentation.h"

VulkanPresentation::VulkanPresentation(VulkanDevices* _devices, VkSurfaceKHR& _vkSurface, uint32_t  width, uint32_t height)
	: vulkanDevices(_devices), vkSurface(_vkSurface)
{
	Create(width, height);

	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	if (vkCreateSemaphore(vulkanDevices->GetLogicalDevice(), &semaphoreInfo, nullptr, &imageAvailableSemaphore) != VK_SUCCESS ||
		vkCreateSemaphore(vulkanDevices->GetLogicalDevice(), &semaphoreInfo, nullptr, &renderFinishedSemaphore) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create semaphores");
	}
}

VulkanPresentation::~VulkanPresentation()
{
	vkDestroySemaphore(vulkanDevices->GetLogicalDevice(), imageAvailableSemaphore, nullptr);
	vkDestroySemaphore(vulkanDevices->GetLogicalDevice(), renderFinishedSemaphore, nullptr);

	for (auto imageView : vkSwapChainImageViews) 
	{
		vkDestroyImageView(vulkanDevices->GetLogicalDevice(), imageView, nullptr);
	}
	vkDestroySwapchainKHR(vulkanDevices->GetLogicalDevice(), vkSwapChain, nullptr);
}

void VulkanPresentation::Create(uint32_t  width, uint32_t height)
{
	VkExtent2D extent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
	createSwapChain(extent);
	createImageViews();
}

void VulkanPresentation::Recreate(uint32_t  width, uint32_t height)
{
	vkDestroySwapchainKHR(vulkanDevices->GetLogicalDevice(), vkSwapChain, nullptr);
	Create(width, height);
}



void VulkanPresentation::createSwapChain(VkExtent2D extent)
{
	const auto& surfaceCapabilities = vulkanDevices->surfaceCapabilities;
	VkSurfaceFormatKHR surfaceFormat = SwapChainUtils::chooseSwapSurfaceFormat(vulkanDevices->surfaceFormats);
	VkPresentModeKHR presentMode = SwapChainUtils::chooseSwapPresentMode(vulkanDevices->presentModes);

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
		vkSurface, imageCount, surfaceFormat.format, surfaceFormat.colorSpace,
		extent, 1, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		surfaceCapabilities.currentTransform,
		VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		presentMode,
		VK_NULL_HANDLE);

	const auto& queueFamilyIndices = vulkanDevices->GetQueueFamilyIndices();
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

	VulkanInitializers::createSwapChain(vulkanDevices->GetLogicalDevice(), swapChainCreateInfo, vkSwapChain);

	// --- Retrieve swap chain images ---
	vkGetSwapchainImagesKHR(vulkanDevices->GetLogicalDevice(), vkSwapChain, &imageCount, nullptr);
	vkSwapChainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(vulkanDevices->GetLogicalDevice(), vkSwapChain, &imageCount, vkSwapChainImages.data());

	vkSwapChainImageFormat = surfaceFormat.format;
	vkSwapChainExtent = extent;
}

void VulkanPresentation::createImageViews() 
{
	vkSwapChainImageViews.resize(vkSwapChainImages.size());

	for (int i = 0; i < vkSwapChainImages.size(); i++) 
	{
		VkImageViewCreateInfo swapChainImageViewCreateInfo =
			VulkanInitializers::basicImageViewCreateInfo(vkSwapChainImages[i], VK_IMAGE_VIEW_TYPE_2D, vkSwapChainImageFormat);

		VulkanInitializers::createImageView(vulkanDevices->GetLogicalDevice(), &vkSwapChainImageViews[i], &swapChainImageViewCreateInfo, nullptr);
	}
}

//----------
//Getters
//----------
inline VkSwapchainKHR VulkanPresentation::GetVulkanSwapChain() const
{
	return vkSwapChain;
}
inline VkImage VulkanPresentation::GetVkImage(uint32_t index) const
{
	return vkSwapChainImages[index];
}
inline VkImageView VulkanPresentation::GetVkImageView(uint32_t index) const
{
	return vkSwapChainImageViews[index];
}
inline VkFormat VulkanPresentation::GetVkImageFormat() const
{
	return vkSwapChainImageFormat;
}
inline VkExtent2D VulkanPresentation::GetVkExtent() const
{
	return vkSwapChainExtent;
}
inline uint32_t VulkanPresentation::GetCount() const
{
	return static_cast<uint32_t>(vkSwapChainImages.size());
}
inline uint32_t VulkanPresentation::GetIndex() const
{
	return imageIndex;
}
inline VkSemaphore VulkanPresentation::GetImageAvailableVkSemaphore() const
{
	return imageAvailableSemaphore;
}
inline VkSemaphore VulkanPresentation::GetRenderFinishedVkSemaphore() const
{
	return renderFinishedSemaphore;
}