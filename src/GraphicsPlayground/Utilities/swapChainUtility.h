#pragma once
#include <global.h>

namespace SwapChainUtil
{
	// Specify the color channel format and color space type
	inline VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
	{
		// VK_FORMAT_UNDEFINED indicates that the surface has no preferred format, so we can choose any
		if (availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED)
		{
			return{ VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
		}

		// Otherwise, choose a preferred combination
		for (const auto& availableFormat : availableFormats)
		{
			// Ideal format and color space
			if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM &&
				availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			{
				return availableFormat;
			}
		}

		// Otherwise, return any format
		return availableFormats[0];
	}

	// Specify the presentation mode of the swap chain
	inline VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes)
	{
		// Second choice
		// good for implementing double buffering
		VkPresentModeKHR bestMode = VK_PRESENT_MODE_FIFO_KHR;

		for (const auto& availablePresentMode : availablePresentModes)
		{
			if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
			{
				// First choice
				// good for implementing triple buffering
				return availablePresentMode;
			}
			else if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
			{
				// Third choice
				bestMode = availablePresentMode;
			}
		}

		return bestMode;
	}

	// Specify the swap extent (resolution) of the swap chain
	inline VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window)
	{
		// Vulkan tells us to match the resolution of the window by setting the width and height in the 
		// currentExtent member. However, some window managers do allow us to differ here and this is 
		// indicated by setting the width and height in currentExtent to a special value: the maximum 
		// value of uint32_t.
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
		{
			return capabilities.currentExtent;
		}
		else
		{
			int width, height;
			glfwGetWindowSize(window, &width, &height);
			VkExtent2D actualExtent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };

			actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
			actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

			return actualExtent;
		}
	}

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
}