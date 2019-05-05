#pragma once
#include <global.h>

namespace VulkanDevicesUtil
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

	inline VkDeviceQueueCreateInfo deviceQueueCreateInfo(VkStructureType sType, uint32_t queueFamilyIndex, uint32_t queueCount, float* pQueuePriorities)
	{
		VkDeviceQueueCreateInfo deviceQueueCreateInfo{};
		deviceQueueCreateInfo.sType = sType;
		deviceQueueCreateInfo.queueFamilyIndex = queueFamilyIndex;
		deviceQueueCreateInfo.queueCount = queueCount;
		deviceQueueCreateInfo.pQueuePriorities = pQueuePriorities;

		return deviceQueueCreateInfo;
	}

	inline void createLogicalDevice(VkPhysicalDevice &physicalDevice, VkDevice& logicalDevice, 
		VkPhysicalDeviceFeatures* deviceFeatures,
		uint32_t queueCreateInfoCount, VkDeviceQueueCreateInfo* queueCreateInfos,
		uint32_t deviceExtensionCount, const char** deviceExtensionNames,
		uint32_t validationLayerCount, const char* const* validationLayerNames)
	{
		VkDeviceCreateInfo logicalDeviceCreateInfo = {};
		logicalDeviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		logicalDeviceCreateInfo.queueCreateInfoCount = queueCreateInfoCount;
		logicalDeviceCreateInfo.pQueueCreateInfos = queueCreateInfos;
		logicalDeviceCreateInfo.pEnabledFeatures = deviceFeatures;

		// Enable device-specific extensions and validation layers
		logicalDeviceCreateInfo.enabledExtensionCount = deviceExtensionCount;
		logicalDeviceCreateInfo.ppEnabledExtensionNames = deviceExtensionNames;
		logicalDeviceCreateInfo.enabledLayerCount = validationLayerCount;
		logicalDeviceCreateInfo.ppEnabledLayerNames = validationLayerNames;

		if (vkCreateDevice(physicalDevice, &logicalDeviceCreateInfo, nullptr, &logicalDevice) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create logical device");
		}
	}
}