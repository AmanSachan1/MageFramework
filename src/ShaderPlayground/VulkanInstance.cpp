#include "VulkanInstance.h"

#ifdef NDEBUG
const bool ENABLE_VALIDATION = false;
#else
const bool ENABLE_VALIDATION = true;
#endif

namespace 
{
	const std::vector<const char*> validationLayers = {
		"VK_LAYER_LUNARG_standard_validation"
	};

	// Get the required list of extensions based on whether validation layers are enabled
	std::vector<const char*> getRequiredExtensions()
	{
		std::vector<const char*> extensions;

		if (ENABLE_VALIDATION)
		{
			extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
		}

		return extensions;
	}

	// Callback function to allow messages from validation layers to be received
	VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugReportFlagsEXT flags,
		VkDebugReportObjectTypeEXT objType,
		uint64_t obj,
		size_t location,
		int32_t code,
		const char* layerPrefix,
		const char* msg,
		void *userData)
	{
		fprintf(stderr, "Validation layer: %s\n", msg);
		return VK_FALSE;
	}
}

VulkanInstance::VulkanInstance(const char* applicationName, unsigned int additionalExtensionCount, const char** additionalExtensions) 
{
    // --- Specify details about our application ---
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = applicationName;
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
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

	if (ENABLE_VALIDATION && !checkValidationLayerSupport()) {
		throw std::runtime_error("validation layers requested, but not available!");
	}

    // Create instance
    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) 
	{
        throw std::runtime_error("Failed to create instance");
    }

	initDebugReport();
}

VulkanInstance::~VulkanInstance() 
{
    if (ENABLE_VALIDATION) 
	{
        auto func = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
        if (func != nullptr) 
		{
            func(instance, debugReportCallback, nullptr);
        }
    }

    vkDestroyInstance(instance, nullptr);
}

VkInstance VulkanInstance::GetVkInstance()
{
	return instance;
}

void VulkanInstance::initDebugReport()
{
	if (ENABLE_VALIDATION)
	{
		// Specify details for callback
		VkDebugReportCallbackCreateInfoEXT createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
		createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
		createInfo.pfnCallback = debugCallback;

		if ([&]() {
			auto func = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
			if (func != nullptr) 
			{
				return func(instance, &createInfo, nullptr, &debugReportCallback);
			}
			else 
			{
				return VK_ERROR_EXTENSION_NOT_PRESENT;
			}
		}() != VK_SUCCESS) {
			throw std::runtime_error("Failed to set up debug callback");
		}
	}
}