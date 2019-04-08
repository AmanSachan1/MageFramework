#pragma once
#include <global.h>
#include <forward.h>
#include <utility.h>
#include "vulkanInstance.h"
#include "vulkanInitializers.h"

class VulkanDevices
{
	friend class VulkanPresentation;

public:
	VulkanDevices() = delete;
	VulkanDevices(VulkanInstance* _instance, std::vector<const char*> deviceExtensions, 
		QueueFlagBits& requiredQueues, VkSurfaceKHR& vkSurface);
	~VulkanDevices();

	void pickPhysicalDevice(std::vector<const char*> deviceExtensions, QueueFlagBits& requiredQueues, VkSurfaceKHR& surface);
	void createLogicalDevice(QueueFlagBits requiredQueues);

	bool isPhysicalDeviceSuitable(VkPhysicalDevice device, std::vector<const char*> deviceExtensions,
								QueueFlagBits requiredQueues, VkSurfaceKHR vkSurface = VK_NULL_HANDLE);
	SwapChainSupportDetails querySwapChainSupport();

	VulkanInstance* getInstance();
	VkDevice getLogicalDevice();
	VkPhysicalDevice getPhysicalDevice();
	VkQueue getQueue(QueueFlags flag);
	unsigned int getQueueIndex(QueueFlags flag);
	QueueFamilyIndices getQueueFamilyIndices();

	//Public member variables
	VkSurfaceCapabilitiesKHR surfaceCapabilities;
	std::vector<VkSurfaceFormatKHR> surfaceFormats;
	std::vector<VkPresentModeKHR> presentModes;
	VkPhysicalDeviceMemoryProperties deviceMemoryProperties;

private:
	VulkanInstance* m_vulkanInstance;
	VkSurfaceKHR m_surface;

	// The physical device is the GPU and the logical device interfaces with the physical device.
	// Reference: https://vulkan-tutorial.com/Drawing_a_triangle/Presentation
	VkDevice m_logicalDevice;
	VkPhysicalDevice m_physicalDevice;
	
	// Queues are required to submit commands
	Queues m_queues;

	QueueFamilyIndices m_queueFamilyIndices;
	std::vector<const char*> m_deviceExtensions;
};