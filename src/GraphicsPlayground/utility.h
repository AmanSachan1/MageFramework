#pragma once
#include <global.h>
#include <fstream>

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
}

namespace ShaderUtils
{
	std::vector<char> readFile(const std::string& filename) 
	{
		std::ifstream file(filename, std::ios::ate | std::ios::binary);

		if (!file.is_open()) 
		{
			throw std::runtime_error("Failed to open file");
		}

		size_t fileSize = (size_t)file.tellg();
		std::vector<char> buffer(fileSize);

		file.seekg(0);
		file.read(buffer.data(), fileSize);

		file.close();
		return buffer;
	}

	// Wrap the shaders in shader modules
	VkShaderModule createShaderModule(const std::vector<char>& code, VkDevice logicalDevice)
	{
		VkShaderModuleCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = code.size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

		VkShaderModule shaderModule;
		if (vkCreateShaderModule(logicalDevice, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) 
		{
			throw std::runtime_error("Failed to create shader module");
		}

		return shaderModule;
	}
	VkShaderModule createShaderModule(const std::string& filename, VkDevice logicalDevice)
	{
		return createShaderModule(readFile(filename), logicalDevice);
	}
}
