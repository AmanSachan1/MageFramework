#pragma once

#include <vulkan/vulkan.h>
#include <bitset>
#include <vector>
#include <stdexcept>
#include <set>

#include "VulkanDevice.h"

extern const bool ENABLE_VALIDATION;

class VulkanInstance 
{
public:
    VulkanInstance() = delete;
    VulkanInstance(const char* applicationName, unsigned int additionalExtensionCount = 0, const char** additionalExtensions = nullptr);
	~VulkanInstance();

    VkInstance GetVkInstance();

private:
	void initDebugReport();

	VkInstance instance;
	VkDebugReportCallbackEXT debugReportCallback;
};