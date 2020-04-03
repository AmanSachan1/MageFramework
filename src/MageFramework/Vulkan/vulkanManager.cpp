#include "vulkanManager.h"

const uint32_t maxFramesInFlight = 3;

VulkanManager::VulkanManager(GLFWwindow* _window, const char* applicationName)
{
	unsigned int glfwExtensionCount = 0;
	const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	// Vulkan Instance
	initVulkanInstance(applicationName, glfwExtensionCount, glfwExtensions);

	// Create Drawing Surface, i.e. window where things are rendered to
	if (glfwCreateWindowSurface(m_instance, _window, nullptr, &m_surface) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create window surface!");
	}

	// Create The physical and logical devices required by vulkan
	// Dictate the type of queues that need to be supported 
	QueueFlagBits requiredQueues = QueueFlagBit::GraphicsBit | QueueFlagBit::ComputeBit | QueueFlagBit::TransferBit | QueueFlagBit::PresentBit;
	// Setup Physical Devices --> Find a GPU that supports Vusurfacelkan
	pickPhysicalDevice( { VK_KHR_SWAPCHAIN_EXTENSION_NAME }, requiredQueues );
	// Create a Logical Device
	createLogicalDevice(requiredQueues);

	createPresentationObjects(_window);
	createSyncObjects();
}
VulkanManager::~VulkanManager() 
{
	vkDeviceWaitIdle(getLogicalDevice());

	if (ENABLE_VALIDATION)
	{
		destroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr);
	}

	vkDestroyDevice(m_logicalDevice, nullptr);
	vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
	vkDestroyInstance(m_instance, nullptr);
}
void VulkanManager::cleanup()
{
	vkDestroySwapchainKHR(m_logicalDevice, m_swapChain, nullptr);
	for (size_t i = 0; i < m_swapChainImageViews.size(); i++)
	{
		vkDestroyImageView(m_logicalDevice, m_swapChainImageViews[i], nullptr);
	}

	for (size_t i = 0; i < maxFramesInFlight; i++)
	{
		vkDestroySemaphore(m_logicalDevice, m_renderFinishedSemaphores[i], nullptr);
		vkDestroySemaphore(m_logicalDevice, m_imageAvailableSemaphores[i], nullptr);
		vkDestroyFence(m_logicalDevice, m_inFlightFences[i], nullptr);
	}
}
void VulkanManager::recreate(GLFWwindow* window)
{
	createPresentationObjects(window);
	createSyncObjects();
}

void VulkanManager::initVulkanInstance(const char* applicationName, unsigned int additionalExtensionCount, const char** additionalExtensions)
{
	// --- Specify details about our application ---
	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = applicationName;
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "Mage Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	// --- Create Vulkan instance ---
	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	// Get extensions necessary for Vulkan to interface with GLFW
	auto extensions = getRequiredExtensions();
	for (unsigned int i = 0; i < additionalExtensionCount; ++i)
	{
		extensions.push_back(additionalExtensions[i]);
	}
	createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();

	// Specify global validation layers
	if (ENABLE_VALIDATION)
	{
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
	}
	else
	{
		createInfo.enabledLayerCount = 0;
	}

	if (ENABLE_VALIDATION && !checkValidationLayerSupport())
	{
		throw std::runtime_error("validation layers requested, but not available!");
	}

	// Create instance
	if (vkCreateInstance(&createInfo, nullptr, &m_instance) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create instance");
	}

	initDebugReport();
}

void VulkanManager::pickPhysicalDevice(std::vector<const char*> deviceExtensions, QueueFlagBits& requiredQueues)
{
	uint32_t physicalDeviceCount = 0;
	vkEnumeratePhysicalDevices(m_instance, &physicalDeviceCount, nullptr);

	if (physicalDeviceCount == 0)
	{
		throw std::runtime_error("failed to find GPUs with Vulkan support!");
	}

	// Iterate through all physical devices,i.e. gpu's that were found
	std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
	vkEnumeratePhysicalDevices(m_instance, &physicalDeviceCount, physicalDevices.data());

	m_physicalDevice = VK_NULL_HANDLE;
	for (const auto& device : physicalDevices)
	{
		if (isPhysicalDeviceSuitable(device, deviceExtensions, requiredQueues, m_surface))
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
void VulkanManager::createLogicalDevice(QueueFlagBits requiredQueues)
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
	deviceFeatures.geometryShader = VK_TRUE;
	deviceFeatures.tessellationShader = VK_TRUE;
	// enable anisotropic filtering -- HIGH Performance Cost
	deviceFeatures.samplerAnisotropy = VK_TRUE;
	// enable sample shading feature for the device -- HIGH Performance Cost
	deviceFeatures.sampleRateShading = VK_TRUE;
	// Needed otherwise compiler complains, possibly related to https://github.com/KhronosGroup/Vulkan-ValidationLayers/issues/327
	deviceFeatures.fragmentStoresAndAtomics = VK_TRUE;


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

void VulkanManager::createPresentationObjects(GLFWwindow* window)
{
	createSwapChain(window);
	createSwapChainImageViews();
}

bool VulkanManager::acquireNextSwapChainImage()
{
	// It is possible to use a semaphore, fence, or both as the synchronization objects that are to be signaled 
	// when the presentation engine is finished using the image
	VkResult result = vkAcquireNextImageKHR(m_logicalDevice, m_swapChain, std::numeric_limits<uint64_t>::max(),
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
bool VulkanManager::presentImageToSwapChain()
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

	VkResult result = vkQueuePresentKHR(getQueue(QueueFlags::Present), &presentInfo);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
	{
		return false;
	}
	else if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to present swap chain image");
	}
	return true;
}

void VulkanManager::advanceCurrentFrameIndex()
{
	m_currentFrameIndex = (m_currentFrameIndex + 1) % maxFramesInFlight;
}

void VulkanManager::waitForAndResetInFlightFence()
{
	VkFence& inFlightFence = m_inFlightFences[m_currentFrameIndex];
	// The VK_TRUE we pass in vkWaitForFences indicates that we want to wait for all fences.
	vkWaitForFences(m_logicalDevice, 1, &inFlightFence, VK_TRUE, std::numeric_limits<uint64_t>::max());
	vkResetFences(m_logicalDevice, 1, &inFlightFence);
}

void VulkanManager::transitionSwapChainImageLayout(uint32_t index, VkImageLayout oldLayout, VkImageLayout newLayout, VkCommandBuffer& graphicsCmdBuffer, VkCommandPool& graphicsCmdPool)
{
	const uint32_t mipLevels = 1;
	VkQueue graphicsQueue = getQueue(QueueFlags::Graphics);
	ImageUtil::transitionImageLayout(m_logicalDevice, graphicsQueue, graphicsCmdPool, graphicsCmdBuffer, m_swapChainImages[index], m_swapChainImageFormat, oldLayout, newLayout, mipLevels);
}
void VulkanManager::transitionSwapChainImageLayout_SingleTimeCommand(uint32_t index, VkImageLayout oldLayout, VkImageLayout newLayout, VkCommandPool& graphicsCmdPool)
{
	const uint32_t mipLevels = 1;
	VkQueue graphicsQueue = getQueue(QueueFlags::Graphics);
	ImageUtil::transitionImageLayout_SingleTimeCommand(m_logicalDevice, graphicsQueue, graphicsCmdPool, m_swapChainImages[index], m_swapChainImageFormat, oldLayout, newLayout, mipLevels);
}


void VulkanManager::copyImageToSwapChainImage(uint32_t index, VkImage& srcImage, VkCommandBuffer& graphicsCmdBuffer, VkCommandPool& graphicsCmdPool, VkExtent2D imgExtents)
{
	VkQueue graphicsQueue = getQueue(QueueFlags::Graphics);
	ImageUtil::copyImageToImage(m_logicalDevice, graphicsQueue, graphicsCmdPool, graphicsCmdBuffer,
		srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, m_swapChainImages[index], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, imgExtents.width, imgExtents.height, 1);
}


// -------------------------------------
// Vulkan Presentation Helper Functions
// -------------------------------------

void VulkanManager::createSwapChain(GLFWwindow* window)
{
	m_swapChainSupport = querySwapChainSupport();

	m_surfaceFormat = SwapChainUtil::chooseSwapSurfaceFormat(m_swapChainSupport.surfaceFormats);
	m_presentMode = SwapChainUtil::chooseSwapPresentMode(m_swapChainSupport.presentModes);
	VkExtent2D extent = SwapChainUtil::chooseSwapExtent(m_swapChainSupport.surfaceCapabilities, window);

	// Can do multiple buffering here!
	// minImageCount is almost definitely 1 (or it doesnt support presentation)
	// setting imageCount to 2 makes it possible to do double buffering
	uint32_t imageCount = m_swapChainSupport.surfaceCapabilities.minImageCount + 1;
	// 0 is a special value for maxImageCount, that means that there is no maximum
	if (m_swapChainSupport.surfaceCapabilities.maxImageCount > 0 && imageCount > m_swapChainSupport.surfaceCapabilities.maxImageCount)
	{
		imageCount = m_swapChainSupport.surfaceCapabilities.maxImageCount;
	}
	VkImageUsageFlags swapchainUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	VkSwapchainCreateInfoKHR swapChainCreateInfo = SwapChainUtil::basicSwapChainCreateInfo(
		m_surface, imageCount, m_surfaceFormat.format, m_surfaceFormat.colorSpace,
		extent, 1, swapchainUsage,
		m_swapChainSupport.surfaceCapabilities.currentTransform,
		VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		m_presentMode,
		VK_NULL_HANDLE);

	if (m_queueFamilyIndices[QueueFlags::Graphics] != m_queueFamilyIndices[QueueFlags::Present])
	{
		// Images can be used across multiple queue families without explicit ownership transfers
		swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swapChainCreateInfo.queueFamilyIndexCount = 2;
		unsigned int indices[] = { m_queueFamilyIndices[QueueFlags::Graphics], m_queueFamilyIndices[QueueFlags::Present] };
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

	SwapChainUtil::createSwapChain(m_logicalDevice, swapChainCreateInfo, m_swapChain);

	// --- Retrieve swap chain images ---
	vkGetSwapchainImagesKHR(m_logicalDevice, m_swapChain, &imageCount, nullptr);
	m_swapChainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(m_logicalDevice, m_swapChain, &imageCount, m_swapChainImages.data());

	m_swapChainImageFormat = m_surfaceFormat.format;
	m_swapChainExtent = extent;
}
void VulkanManager::createSwapChainImageViews()
{
	m_swapChainImageViews.resize(m_swapChainImages.size());
	for (int i = 0; i < m_swapChainImages.size(); i++)
	{
		ImageUtil::createImageView(m_logicalDevice, m_swapChainImages[i], &m_swapChainImageViews[i],
			VK_IMAGE_VIEW_TYPE_2D, m_swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1, nullptr);
	}
}

void VulkanManager::createSyncObjects()
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
	m_imageAvailableSemaphores.resize(maxFramesInFlight);
	m_renderFinishedSemaphores.resize(maxFramesInFlight);
	m_inFlightFences.resize(maxFramesInFlight);

	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo = {};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t i = 0; i < maxFramesInFlight; i++)
	{
		if (vkCreateSemaphore(m_logicalDevice, &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i]) != VK_SUCCESS ||
			vkCreateSemaphore(m_logicalDevice, &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i]) != VK_SUCCESS ||
			vkCreateFence(m_logicalDevice, &fenceInfo, nullptr, &m_inFlightFences[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create synchronization objects for a frame");
		}
	}
}


// -------------------------------
// Vulkan Devices Helper Functions
// -------------------------------

bool VulkanManager::isPhysicalDeviceSuitable(VkPhysicalDevice pDevice, std::vector<const char*> deviceExtensions,
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
		deviceFeatures.samplerAnisotropy &&
		deviceFeatures.fragmentStoresAndAtomics);

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
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(pDevice, vkSurface, &m_swapChainSupport.surfaceCapabilities);

		// Let's figure out the surface formats supported for the surface
		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(pDevice, vkSurface, &formatCount, nullptr);

		if (formatCount != 0)
		{
			m_swapChainSupport.surfaceFormats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(pDevice, vkSurface, &formatCount, m_swapChainSupport.surfaceFormats.data());
		}

		// Let's figure out the presentation modes supported for the surface
		uint32_t presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(pDevice, vkSurface, &presentModeCount, nullptr);

		if (presentModeCount != 0)
		{
			m_swapChainSupport.presentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(pDevice, vkSurface, &presentModeCount, m_swapChainSupport.presentModes.data());
		}
	}

	bool deviceExtensionsSupported = VulkanDevicesUtil::checkDeviceExtensionSupport(pDevice, deviceExtensions);

	// Swap chain support is sufficient if:
	//		- For the Surface we have there is:
	//			-- one supported image format 
	//			-- one supported presentation mode 
	//		- the surface has a maxImageCount of atleast 2 to support double buffering
	bool swapChainSupportAdequate = ( !requiredQueues[QueueFlags::Present] || 
									( !m_swapChainSupport.surfaceFormats.empty() && !m_swapChainSupport.presentModes.empty()) );
	swapChainSupportAdequate = ( swapChainSupportAdequate && 
							   ( m_swapChainSupport.surfaceCapabilities.maxImageCount > 1) );

	return queueSupport & swapChainSupportAdequate  & desiredFeaturesSupported & deviceExtensionsSupported;
}

SwapChainSupportDetails VulkanManager::querySwapChainSupport()
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


// -------------------------------
// Vulkan Instance Debug Functions
// -------------------------------
void VulkanManager::initDebugReport()
{
	if (ENABLE_VALIDATION)
	{
		// Specify details for callback
		VkDebugUtilsMessengerCreateInfoEXT  createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		createInfo.pfnUserCallback = debugCallback;

		if (createDebugUtilsMessengerEXT(m_instance, &createInfo, nullptr, &m_debugMessenger) != VK_SUCCESS) {
			throw std::runtime_error("failed to set up debug messenger!");
		}
	}
}

std::vector<const char*> VulkanManager::getRequiredExtensions()
{
	std::vector<const char*> extensions;

	if (ENABLE_VALIDATION)
	{
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	return extensions;
}

bool VulkanManager::checkValidationLayerSupport()
{
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	for (const char* layerName : validationLayers)
	{
		bool layerFound = false;

		for (const auto& layerProperties : availableLayers)
		{
			if (strcmp(layerName, layerProperties.layerName) == 0)
			{
				layerFound = true;
				break;
			}
		}

		if (!layerFound)
		{
			return false;
		}
	}

	return true;
}

VkResult VulkanManager::createDebugUtilsMessengerEXT(VkInstance instance,
	const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator,
	VkDebugUtilsMessengerEXT* pDebugMessenger)
{
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr)
	{
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}
	else
	{
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void VulkanManager::destroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
	const VkAllocationCallbacks* pAllocator)
{
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr)
	{
		func(instance, debugMessenger, pAllocator);
	}
}

VKAPI_ATTR VkBool32 VKAPI_CALL VulkanManager::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData)
{
	std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

	return VK_FALSE;
}