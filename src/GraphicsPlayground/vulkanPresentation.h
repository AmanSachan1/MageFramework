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

	void Acquire();
	void Present();
	
	void Create(uint32_t  width, uint32_t height);
	void Recreate(uint32_t  width, uint32_t height);

	//Creates SwapChain and store a handle to the images that make up the swapchain
	void createSwapChain(VkExtent2D extent);
	void createImageViews();

	VkSwapchainKHR GetVulkanSwapChain() const;
	VkImage GetVkImage(uint32_t index) const;
	VkImageView GetVkImageView(uint32_t index) const;
	VkFormat GetVkImageFormat() const;
	VkExtent2D GetVkExtent() const;
	uint32_t GetCount() const;
	uint32_t GetIndex() const;
	VkSemaphore GetImageAvailableVkSemaphore() const;
	VkSemaphore GetRenderFinishedVkSemaphore() const;

public:
	VulkanDevices* vulkanDevices;
	VkSurfaceKHR vkSurface;

	VkSwapchainKHR vkSwapChain;
	std::vector<VkImage> vkSwapChainImages;
	std::vector<VkImageView> vkSwapChainImageViews;

	VkFormat vkSwapChainImageFormat;
	VkExtent2D vkSwapChainExtent;
	uint32_t imageIndex = 0;

	VkSemaphore imageAvailableSemaphore;
	VkSemaphore renderFinishedSemaphore;
};