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

	inline VkSwapchainKHR GetVulkanSwapChain() const;
	inline VkImage GetVkImage(uint32_t index) const;
	inline VkImageView GetVkImageView(uint32_t index) const;
	inline VkFormat GetVkImageFormat() const;
	inline VkExtent2D GetVkExtent() const;
	inline uint32_t GetCount() const;
	inline uint32_t GetIndex() const;
	inline VkSemaphore GetImageAvailableVkSemaphore() const;
	inline VkSemaphore GetRenderFinishedVkSemaphore() const;

private:
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