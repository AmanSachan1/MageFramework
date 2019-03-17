#include "vulkanDevices.h"

VulkanDevices::VulkanDevices(VulkanInstance* _instance, std::vector<const char*> deviceExtensions,
	QueueFlagBits& requiredQueues, VkSurfaceKHR& vkSurface)
	: vulkanInstance(_instance)
{
	// Setup Physical Devices --> Find a GPU that supports Vusurfacelkan
	pickPhysicalDevice(deviceExtensions, requiredQueues, vkSurface);
	// Create a Logical Device
	createLogicalDevice(requiredQueues);
}

VulkanDevices::~VulkanDevices()
{
	vkDestroyDevice(logicalDevice, nullptr);
}

void VulkanDevices::createLogicalDevice(QueueFlagBits requiredQueues)
{
	bool queueSupport = true;
	std::set<int> uniqueQueueFamilies;	
	for (unsigned int i = 0; i < requiredQueues.size(); ++i)
	{
		if (requiredQueues[i])
		{
			queueSupport &= (queueFamilyIndices[i] >= 0);
			uniqueQueueFamilies.insert(queueFamilyIndices[i]);
		}
	}

	if (!queueSupport)
	{
		throw std::runtime_error("Device does not support requested queues");
	}

	// Create all of the required queues
	std::vector<VkDeviceQueueCreateInfo> deviceQueueCreateInfos;
	float queuePriority = 1.0f;

	for (int queueFamily : uniqueQueueFamilies)
	{
		VkDeviceQueueCreateInfo deviceQueueCreateInfo = VulkanInitializers::deviceQueueCreateInfo( 
			VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, queueFamily, 1, &queuePriority);

		deviceQueueCreateInfos.push_back(deviceQueueCreateInfo);
	}

	// Specify the set of device features used
	VkPhysicalDeviceFeatures deviceFeatures = {};
	deviceFeatures.samplerAnisotropy = VK_TRUE;

	// Actually create logical device
	VkDeviceCreateInfo logicalDeviceCreateInfo;
	if (ENABLE_VALIDATION)
	{
		logicalDeviceCreateInfo = VulkanInitializers::logicalDeviceCreateInfo(
			VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO, 
			static_cast<uint32_t>(deviceQueueCreateInfos.size()),
			deviceQueueCreateInfos.data(), 
			&deviceFeatures,
			static_cast<uint32_t>(deviceExtensions.size()), deviceExtensions.data(),
			static_cast<uint32_t>(validationLayers.size()), validationLayers.data());
	}
	else
	{
		logicalDeviceCreateInfo = VulkanInitializers::logicalDeviceCreateInfo(
			VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO, 
			static_cast<uint32_t>(deviceQueueCreateInfos.size()),
			deviceQueueCreateInfos.data(), 
			&deviceFeatures,
			static_cast<uint32_t>(deviceExtensions.size()), deviceExtensions.data(),
			0, nullptr);
	}
	VulkanInitializers::createLogicalDevice( physicalDevice, logicalDevice, logicalDeviceCreateInfo);

	//Get required queues
	for (unsigned int i = 0; i < requiredQueues.size(); ++i)
	{
		if (requiredQueues[i])
		{
			vkGetDeviceQueue(logicalDevice, queueFamilyIndices[i], 0, &queues[i]);
		}
	}
}

void VulkanDevices::pickPhysicalDevice(std::vector<const char*> deviceExtensions, QueueFlagBits& requiredQueues, VkSurfaceKHR& surface)
{
	uint32_t physicalDeviceCount = 0;
	vkEnumeratePhysicalDevices(vulkanInstance->GetVkInstance(), &physicalDeviceCount, nullptr);

	if (physicalDeviceCount == 0)
	{
		throw std::runtime_error("failed to find GPUs with Vulkan support!");
	}

	// Iterate through all physical devices,i.e. gpu's that were found
	std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
	vkEnumeratePhysicalDevices(vulkanInstance->GetVkInstance(), &physicalDeviceCount, physicalDevices.data());

	physicalDevice = VK_NULL_HANDLE;
	for (const auto& device : physicalDevices)
	{
		if (isPhysicalDeviceSuitable(device, deviceExtensions, requiredQueues, surface))
		{
			physicalDevice = device;
			break;
		}
	}

	this->deviceExtensions = deviceExtensions;

	if (physicalDevice == VK_NULL_HANDLE)
	{
		throw std::runtime_error("failed to find a suitable GPU!");
	}

	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &deviceMemoryProperties);
}

// --------
// Helpers
// --------

bool VulkanDevices::isPhysicalDeviceSuitable(VkPhysicalDevice pDevice, std::vector<const char*> deviceExtensions,
											QueueFlagBits requiredQueues, VkSurfaceKHR vkSurface)
{
	VkPhysicalDeviceProperties deviceProperties;
	VkPhysicalDeviceFeatures deviceFeatures;
	vkGetPhysicalDeviceProperties(pDevice, &deviceProperties);
	vkGetPhysicalDeviceFeatures(pDevice, &deviceFeatures);

	//Add features you really need in the gpu
	bool desiredFeaturesSupported = (deviceProperties.deviceType 
									 == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
										deviceFeatures.geometryShader &&
										deviceFeatures.tessellationShader);
	
	bool queueSupport = true;
	queueFamilyIndices = DeviceUtils::checkDeviceQueueSupport(pDevice, requiredQueues, vkSurface);
	for (unsigned int i = 0; i < requiredQueues.size(); i++)
	{
		if (requiredQueues[i])
		{
			queueSupport &= (queueFamilyIndices[i] >= 0);
		}
	}

	if (requiredQueues[QueueFlags::Present])
	{
		// Get basic surface capabilities
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(pDevice, vkSurface, &surfaceCapabilities);

		// Let's figure out the surface formats supported for the surface
		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(pDevice, vkSurface, &formatCount, nullptr);

		if (formatCount != 0)
		{
			surfaceFormats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(pDevice, vkSurface, &formatCount, surfaceFormats.data());
		}

		// Let's figure out the presentation modes supported for the surface
		uint32_t presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(pDevice, vkSurface, &presentModeCount, nullptr);

		if (presentModeCount != 0)
		{
			presentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(pDevice, vkSurface, &presentModeCount, presentModes.data());
		}
	}

	bool deviceExtensionsSupported = DeviceUtils::checkDeviceExtensionSupport(pDevice, deviceExtensions);

	// Swap chain support is sufficient if:
	//		- For the Surface we have there is:
	//			-- one supported image format 
	//			-- one supported presentation mode 
	//		- the surface has a maxImageCount of atleast 2 to support double buffering
	bool swapChainSupportAdequate = (!requiredQueues[QueueFlags::Present] || (!surfaceFormats.empty() && !presentModes.empty()));
	swapChainSupportAdequate = (swapChainSupportAdequate &&	(surfaceCapabilities.maxImageCount > 1));

	return queueSupport & swapChainSupportAdequate  & desiredFeaturesSupported & deviceExtensionsSupported;
}

// --------
// Getters
// --------

VulkanInstance* VulkanDevices::GetInstance()
{
	return vulkanInstance;
}

VkDevice VulkanDevices::GetLogicalDevice()
{
	return logicalDevice;
}

VkPhysicalDevice VulkanDevices::GetPhysicalDevice()
{
	return physicalDevice;
}

VkQueue VulkanDevices::GetQueue(QueueFlags flag)
{
	return queues[flag];
}

unsigned int VulkanDevices::GetQueueIndex(QueueFlags flag)
{
	return queueFamilyIndices[flag];
}

QueueFamilyIndices VulkanDevices::GetQueueFamilyIndices()
{
	return queueFamilyIndices;
}