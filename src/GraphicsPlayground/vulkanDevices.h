#pragma once
#include <global.h>
#include <utility.h>
#include "vulkanInstance.h"
#include "vulkanInitializers.h"

class VulkanDevices
{
public:
	VulkanDevices() = delete;
	VulkanDevices(VulkanInstance* _instance, std::vector<const char*> deviceExtensions, 
					QueueFlagBits& requiredQueues, VkSurfaceKHR& vkSurface);
	~VulkanDevices();

	void pickPhysicalDevice(std::vector<const char*> deviceExtensions, QueueFlagBits& requiredQueues, VkSurfaceKHR& surface);
	void createLogicalDevice(QueueFlagBits requiredQueues);

	bool isPhysicalDeviceSuitable(VkPhysicalDevice device, std::vector<const char*> deviceExtensions,
								QueueFlagBits requiredQueues, VkSurfaceKHR vkSurface = VK_NULL_HANDLE);

	VulkanInstance* GetInstance();
	VkDevice GetLogicalDevice();
	VkPhysicalDevice GetPhysicalDevice();
	VkQueue GetQueue(QueueFlags flag);
	unsigned int GetQueueIndex(QueueFlags flag);

	//Public member variables
	VkSurfaceCapabilitiesKHR surfaceCapabilities;
	std::vector<VkSurfaceFormatKHR> surfaceFormats;
	std::vector<VkPresentModeKHR> presentModes;
	VkPhysicalDeviceMemoryProperties deviceMemoryProperties;

private:
	VulkanInstance* vulkanInstance;

	// The physical device is the GPU and the logical device interfaces with the physical device.
	// Reference: https://vulkan-tutorial.com/Drawing_a_triangle/Presentation
	VkDevice logicalDevice; 
	VkPhysicalDevice physicalDevice;
	
	// Queues are required to submit commands
	Queues queues;

	QueueFamilyIndices queueFamilyIndices;
	std::vector<const char*> deviceExtensions;
};