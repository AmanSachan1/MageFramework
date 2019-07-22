#include "vulkanDevices.h"

VulkanDevices::VulkanDevices(VulkanInstance* _instance, std::vector<const char*> deviceExtensions,
	QueueFlagBits& requiredQueues, VkSurfaceKHR& vkSurface)
	: m_vulkanInstance(_instance), m_surface(vkSurface)
{
	// Setup Physical Devices --> Find a GPU that supports Vusurfacelkan
	pickPhysicalDevice(deviceExtensions, requiredQueues, vkSurface);
	// Create a Logical Device
	createLogicalDevice(requiredQueues);
}

VulkanDevices::~VulkanDevices()
{
	vkDestroyDevice(m_logicalDevice, nullptr);
}

void VulkanDevices::createLogicalDevice(QueueFlagBits requiredQueues)
{
	bool queueSupport = true;
	std::set<int> uniqueQueueFamilies;	
	for (unsigned int i = 0; i < requiredQueues.size(); ++i)
	{
		if (requiredQueues[i])
		{
			queueSupport &= (m_queueFamilyIndices[i] >= 0);
			uniqueQueueFamilies.insert(m_queueFamilyIndices[i]);
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
		VkDeviceQueueCreateInfo deviceQueueCreateInfo = VulkanDevicesUtil::deviceQueueCreateInfo(
			VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, queueFamily, 1, &queuePriority);

		deviceQueueCreateInfos.push_back(deviceQueueCreateInfo);
	}

	// Specify the set of device features used
	VkPhysicalDeviceFeatures deviceFeatures = {};
	// enable anisotropic filtering -- HIGH Performance Cost
	deviceFeatures.samplerAnisotropy = VK_TRUE;
	// enable sample shading feature for the device - HIGH Performance Cost
	deviceFeatures.sampleRateShading = VK_TRUE;

	// Actually create logical device
	if (ENABLE_VALIDATION)
	{
		VulkanDevicesUtil::createLogicalDevice(m_physicalDevice, m_logicalDevice, &deviceFeatures,
			static_cast<uint32_t>(deviceQueueCreateInfos.size()), deviceQueueCreateInfos.data(),
			static_cast<uint32_t>(m_deviceExtensions.size()), m_deviceExtensions.data(),
			static_cast<uint32_t>(validationLayers.size()), validationLayers.data());
	}
	else
	{
		VulkanDevicesUtil::createLogicalDevice(m_physicalDevice, m_logicalDevice, &deviceFeatures,
			static_cast<uint32_t>(deviceQueueCreateInfos.size()), deviceQueueCreateInfos.data(),
			static_cast<uint32_t>(m_deviceExtensions.size()), m_deviceExtensions.data(),
			0, nullptr);
	}

	//Get required queues
	for (unsigned int i = 0; i < requiredQueues.size(); ++i)
	{
		if (requiredQueues[i])
		{
			vkGetDeviceQueue(m_logicalDevice, m_queueFamilyIndices[i], 0, &m_queues[i]);
		}
	}
}

void VulkanDevices::pickPhysicalDevice(std::vector<const char*> deviceExtensions, QueueFlagBits& requiredQueues, VkSurfaceKHR& surface)
{
	uint32_t physicalDeviceCount = 0;
	vkEnumeratePhysicalDevices(m_vulkanInstance->getVkInstance(), &physicalDeviceCount, nullptr);

	if (physicalDeviceCount == 0)
	{
		throw std::runtime_error("failed to find GPUs with Vulkan support!");
	}

	// Iterate through all physical devices,i.e. gpu's that were found
	std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
	vkEnumeratePhysicalDevices(m_vulkanInstance->getVkInstance(), &physicalDeviceCount, physicalDevices.data());

	m_physicalDevice = VK_NULL_HANDLE;
	for (const auto& device : physicalDevices)
	{
		if (isPhysicalDeviceSuitable(device, deviceExtensions, requiredQueues, surface))
		{
			m_physicalDevice = device;
			m_msaaSamples = VulkanDevicesUtil::getMaxUsableSampleCount(m_physicalDevice);
			break;
		}
	}

	this->m_deviceExtensions = deviceExtensions;

	if (m_physicalDevice == VK_NULL_HANDLE)
	{
		throw std::runtime_error("failed to find a suitable GPU!");
	}

	vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &deviceMemoryProperties);
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
										deviceFeatures.tessellationShader &&
										deviceFeatures.samplerAnisotropy);
	
	bool queueSupport = true;
	m_queueFamilyIndices = VulkanDevicesUtil::checkDeviceQueueSupport(pDevice, requiredQueues, vkSurface);
	for (unsigned int i = 0; i < requiredQueues.size(); i++)
	{
		if (requiredQueues[i])
		{
			queueSupport &= (m_queueFamilyIndices[i] >= 0);
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

	bool deviceExtensionsSupported = VulkanDevicesUtil::checkDeviceExtensionSupport(pDevice, deviceExtensions);

	// Swap chain support is sufficient if:
	//		- For the Surface we have there is:
	//			-- one supported image format 
	//			-- one supported presentation mode 
	//		- the surface has a maxImageCount of atleast 2 to support double buffering
	bool swapChainSupportAdequate = (!requiredQueues[QueueFlags::Present] || (!surfaceFormats.empty() && !presentModes.empty()));
	swapChainSupportAdequate = (swapChainSupportAdequate &&	(surfaceCapabilities.maxImageCount > 1));

	return queueSupport & swapChainSupportAdequate  & desiredFeaturesSupported & deviceExtensionsSupported;
}

SwapChainSupportDetails VulkanDevices::querySwapChainSupport()
{
	SwapChainSupportDetails details;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physicalDevice, m_surface, &details.surfaceCapabilities);

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, m_surface, &formatCount, nullptr);

	if (formatCount != 0)
	{
		details.surfaceFormats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, m_surface, &formatCount, details.surfaceFormats.data());
	}

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice, m_surface, &presentModeCount, nullptr);

	if (presentModeCount != 0)
	{
		details.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice, m_surface, &presentModeCount, details.presentModes.data());
	}

	return details;
}

// --------
// Getters
// --------

VulkanInstance* VulkanDevices::getInstance()
{
	return m_vulkanInstance;
}
const VkDevice VulkanDevices::getLogicalDevice() const
{
	return m_logicalDevice;
}
const VkPhysicalDevice VulkanDevices::getPhysicalDevice() const
{
	return m_physicalDevice;
}
VkQueue VulkanDevices::getQueue(QueueFlags flag)
{
	return m_queues[flag];
}
unsigned int VulkanDevices::getQueueIndex(QueueFlags flag)
{
	return m_queueFamilyIndices[flag];
}
QueueFamilyIndices VulkanDevices::getQueueFamilyIndices()
{
	return m_queueFamilyIndices;
}
