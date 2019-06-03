#pragma once
#include <global.h>

extern const bool ENABLE_VALIDATION;

const std::vector<const char*> validationLayers = {
	"VK_LAYER_LUNARG_standard_validation"
};

class VulkanInstance 
{
public:
    VulkanInstance() = delete;
    VulkanInstance(const char* applicationName, unsigned int additionalExtensionCount = 0, const char** additionalExtensions = nullptr);
	~VulkanInstance();

    VkInstance getVkInstance();

private:
	// Debug Functions
	void initDebugReport();
	bool checkValidationLayerSupport();

	// Get the required list of extensions based on whether validation layers are enabled
	std::vector<const char*> getRequiredExtensions();

	// Callback function to allow messages from validation layers to be received
	VkResult createDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
		const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
	void destroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
		const VkAllocationCallbacks* pAllocator);

	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData);


	// Variables
	VkInstance m_instance;
	VkDebugUtilsMessengerEXT m_debugMessenger;
};