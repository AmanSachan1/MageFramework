#pragma once
#include <global.h>
#include <forward.h>
#include <Vulkan/Utilities/vSwapChainUtil.h>
#include <Vulkan/Utilities/vImageUtil.h>
#include <Vulkan/Utilities/vDeviceUtil.h>

#ifdef DEBUG_GRAPHICS_PLAYGROUND
static const bool ENABLE_VALIDATION = true;
#else
static const bool ENABLE_VALIDATION = false;
#endif

const std::vector<const char*> validationLayers = 
{
	"VK_LAYER_LUNARG_standard_validation"
};

struct VulkanDevices
{
	VkDevice lDevice; // Logical Device
	VkPhysicalDevice pDevice; // Physical Device
};

// This class will hold the state and regulate access to all things vulkan
class VulkanManager
{
public:
	VulkanManager() = delete;
	VulkanManager(GLFWwindow* _window, const char* applicationName);
	~VulkanManager();
	void cleanup();
	
	void recreate(GLFWwindow* window);
	void createPresentationObjects(GLFWwindow* window);

	// If acquireNextSwapChainImage() or presentImageToSwapChain(...) fail then 
	// the swapchain and everything associated with it should be recreated
	bool acquireNextSwapChainImage();
	bool presentImageToSwapChain();
	void advanceCurrentFrameIndex();

	// Fences
	void waitForAndResetInFlightFence();

	//---------
	// Getters
	//---------
	const VkInstance getVkInstance() const { return m_instance; }
	const VkSurfaceKHR getSurface() const { return m_surface; }

	const VkDevice getLogicalDevice() const { return m_logicalDevice; }
	const VkPhysicalDevice getPhysicalDevice() const { return m_physicalDevice; }
	const VulkanDevices getVulkanDevices() const { return { m_logicalDevice, m_physicalDevice }; }

	VkQueue getQueue(QueueFlags flag) const { return m_queues[flag]; }
	uint32_t getQueueIndex(QueueFlags flag) const { return m_queueFamilyIndices[flag]; }
	
	VkImageView getSwapChainImageView(uint32_t index) const { return m_swapChainImageViews[index]; }
	const VkFormat getSwapChainImageFormat() const { return m_swapChainImageFormat; }
	const uint32_t getSwapChainImageCount() const { return static_cast<uint32_t>(m_swapChainImages.size()); }
	const VkExtent2D getSwapChainVkExtent() const { return m_swapChainExtent; }
	const uint32_t getIndex() const { return m_imageIndex; }

	VkSemaphore getImageAvailableVkSemaphore() const { return m_imageAvailableSemaphores[m_currentFrameIndex]; }
	VkSemaphore getRenderFinishedVkSemaphore() const { return m_renderFinishedSemaphores[m_currentFrameIndex]; }
	VkFence getInFlightFence() const { return m_inFlightFences[m_currentFrameIndex]; }

	const VkSurfaceFormatKHR getSurfaceFormat() const { return m_surfaceFormat; }
	const VkPresentModeKHR getPresentMode() const { return m_presentMode; }

	
private:
	//-------------------------
	// Vulkan Instance related
	//-------------------------
	VkInstance m_instance;
	VkDebugUtilsMessengerEXT m_debugMessenger;
	
	//-----------------------
	// Vulkan Device related
	//-----------------------
	VkSurfaceKHR m_surface;

	// The physical device is the GPU and the logical device interfaces with the physical device.
	// Reference: https://vulkan-tutorial.com/Drawing_a_triangle/Presentation
	VkDevice m_logicalDevice;
	VkPhysicalDevice m_physicalDevice;
	VkPhysicalDeviceMemoryProperties deviceMemoryProperties;

	// Queues are required to submit commands
	Queues m_queues;
	QueueFamilyIndices m_queueFamilyIndices;
	std::vector<const char*> m_deviceExtensions;

	SwapChainSupportDetails m_swapChainSupport;
	VkSurfaceFormatKHR m_surfaceFormat;
	VkPresentModeKHR m_presentMode;

	//-----------------------------
	// Vulkan Presentation related
	//-----------------------------

	VkSwapchainKHR m_swapChain;
	std::vector<VkImage> m_swapChainImages;
	std::vector<VkImageView> m_swapChainImageViews;

	VkFormat m_swapChainImageFormat;
	VkExtent2D m_swapChainExtent;
	uint32_t m_imageIndex = 0;
	uint32_t m_currentFrameIndex = 0;

	std::vector<VkSemaphore> m_imageAvailableSemaphores;
	std::vector<VkSemaphore> m_renderFinishedSemaphores;
	std::vector<VkFence> m_inFlightFences;

	//----------
	// Settings
	//----------
	// MSAA
	VkSampleCountFlagBits m_msaaSamples = VK_SAMPLE_COUNT_1_BIT;

private:
	void initVulkanInstance(const char* applicationName, unsigned int additionalExtensionCount = 0, const char** additionalExtensions = nullptr);
	void pickPhysicalDevice(std::vector<const char*> deviceExtensions, QueueFlagBits& requiredQueues);
	void createLogicalDevice(QueueFlagBits requiredQueues);

	//-----------------------------------------
	// Helper Functions -- Vulkan Presentation
	//-----------------------------------------
	// Creates SwapChain and store a handle to the images that make up the swapchain
	void createSwapChain(GLFWwindow* window);
	void createSwapChainImageViews();
	void createSyncObjects();

	//------------------------------------
	// Helper Functions -- Vulkan Devices
	//------------------------------------
	bool isPhysicalDeviceSuitable(VkPhysicalDevice device, std::vector<const char*> deviceExtensions,
		QueueFlagBits requiredQueues, VkSurfaceKHR vkSurface = VK_NULL_HANDLE);
	SwapChainSupportDetails querySwapChainSupport();

	//-------------------------------------
	// Helper Functions -- Vulkan Instance
	//-------------------------------------
	void initDebugReport();
	bool checkValidationLayerSupport();

	// Get the required list of extensions based on whether validation layers are enabled
	std::vector<const char*> getRequiredExtensions();

	// Callback function to allow messages from validation layers to be received
	VkResult createDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
	void destroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);

	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);
};