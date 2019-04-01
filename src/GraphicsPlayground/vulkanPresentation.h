#pragma once
#include <global.h>
#include <utility.h>
#include "vulkanInstance.h"
#include "vulkanDevices.h"

class VulkanPresentation
{
public:
	VulkanPresentation(VulkanDevices* _devices, VkSurfaceKHR& vkSurface, uint32_t  width, uint32_t height);
	~VulkanPresentation();

	void acquireNextSwapChainImage(VkDevice& logicalDevice);
	void presentImageToSwapChain(VkDevice& logicalDevice);
	
	void create(uint32_t  width, uint32_t height);
	void recreate(uint32_t  width, uint32_t height);

	//Creates SwapChain and store a handle to the images that make up the swapchain
	void createSwapChain(VkExtent2D extent);
	void createImageViews();
	void createSemaphores();

	VkSwapchainKHR getVulkanSwapChain() const;
	VkImage getVkImage(uint32_t index) const;
	VkImageView getVkImageView(uint32_t index) const;
	VkFormat getVkImageFormat() const;
	VkExtent2D getVkExtent() const;
	uint32_t getCount() const;
	uint32_t getIndex() const;
	VkSemaphore getImageAvailableVkSemaphore() const;
	VkSemaphore getRenderFinishedVkSemaphore() const;

public:
	VulkanDevices* m_vulkanDevices;
	VkSurfaceKHR m_surface;

	VkSwapchainKHR m_swapChain;
	std::vector<VkImage> m_swapChainImages;
	std::vector<VkImageView> m_swapChainImageViews;

	VkFormat m_swapChainImageFormat;
	VkExtent2D m_swapChainExtent;
	uint32_t m_imageIndex = 0;

	VkSemaphore m_imageAvailableSemaphore;
	VkSemaphore m_renderFinishedSemaphore;
};