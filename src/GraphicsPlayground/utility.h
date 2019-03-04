#pragma once
#include <global.h>

namespace DeviceUtils
{
	// Find out if the device supports all required queues
	inline QueueFamilyIndices checkDeviceQueueSupport(VkPhysicalDevice pDevice, QueueFlagBits requiredQueues, VkSurfaceKHR surface = VK_NULL_HANDLE)
	{
		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(pDevice, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(pDevice, &queueFamilyCount, queueFamilies.data());

		VkQueueFlags requiredVulkanQueues = 0;

		if (requiredQueues[QueueFlags::Graphics])
		{
			requiredVulkanQueues |= VK_QUEUE_GRAPHICS_BIT;
		}
		if (requiredQueues[QueueFlags::Compute])
		{
			requiredVulkanQueues |= VK_QUEUE_COMPUTE_BIT;
		}
		if (requiredQueues[QueueFlags::Transfer])
		{
			requiredVulkanQueues |= VK_QUEUE_TRANSFER_BIT;
		}

		QueueFamilyIndices indices = {};
		indices.fill(-1);
		VkQueueFlags supportedQueues = 0;

		bool needsPresent = requiredQueues[QueueFlags::Present];
		bool presentSupported = false;

		int i = 0;
		for (const auto& queueFamily : queueFamilies)
		{
			if (queueFamily.queueCount > 0)
			{
				supportedQueues |= queueFamily.queueFlags;

				if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
				{
					indices[QueueFlags::Graphics] = i;
				}
				if (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT)
				{
					indices[QueueFlags::Compute] = i;
				}
				if (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT)
				{
					indices[QueueFlags::Transfer] = i;
				}
			}

			if (needsPresent)
			{
				VkBool32 presentSupport = false;
				vkGetPhysicalDeviceSurfaceSupportKHR(pDevice, i, surface, &presentSupport);
				if (presentSupport)
				{
					presentSupported = true;
					indices[QueueFlags::Present] = i;
				}
			}

			// Exit out of loop as soon as we have minimal required queue and presentation support
			if ((requiredVulkanQueues & supportedQueues) == requiredVulkanQueues && (!needsPresent || presentSupported))
			{
				break;
			}

			i++;
		}

		if ((requiredVulkanQueues & supportedQueues) != requiredVulkanQueues)
		{
			fprintf(stderr, "The required VkQueues are not supported by the physical device \n");
			exit(EXIT_FAILURE);
		}

		if (needsPresent || !presentSupported)
		{
			fprintf(stderr, "present not found \n");
			exit(EXIT_FAILURE);
		}

		return indices;
	}

	inline bool checkDeviceExtensionSupport(VkPhysicalDevice pDevice, std::vector<const char*> requiredExtensions)
	{
		uint32_t extensionCount;
		vkEnumerateDeviceExtensionProperties(pDevice, nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(pDevice, nullptr, &extensionCount, availableExtensions.data());

		std::set<std::string> requiredExtensionSet(requiredExtensions.begin(), requiredExtensions.end());

		for (const auto& extension : availableExtensions)
		{
			requiredExtensionSet.erase(extension.extensionName);
		}

		return requiredExtensionSet.empty();
	}
}

namespace SwapChainUtils
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
		VkPresentModeKHR bestMode = VK_PRESENT_MODE_FIFO_KHR;

		for (const auto& availablePresentMode : availablePresentModes)
		{
			if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
			{
				// First choice
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
}