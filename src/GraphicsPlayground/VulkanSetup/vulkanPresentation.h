#pragma once
#include <global.h>
#include <forward.h>
#include <Utilities/swapChainUtility.h>
#include <Utilities/imageUtility.h>
#include "vulkanInstance.h"
#include "vulkanDevices.h"

const static unsigned int MAX_FRAMES_IN_FLIGHT = 2;

class VulkanPresentation
{
public:
	VulkanPresentation(VulkanDevices* _devices, VkSurfaceKHR& vkSurface, GLFWwindow* _window);
	~VulkanPresentation();

	// If it fails then the swapchain and everything associated with it should be recreated
	bool acquireNextSwapChainImage(VkDevice& logicalDevice);
	// If it fails then the swapchain and everything associated with it should be recreated
	bool presentImageToSwapChain(VkDevice& logicalDevice, bool frameBufferResized);
	
	void create(GLFWwindow* window);
	void cleanup();

	//Creates SwapChain and store a handle to the images that make up the swapchain
	void createSwapChain(GLFWwindow* window);
	void createImageViews();
	void createSyncObjects();

	VkSwapchainKHR getVulkanSwapChain() const;
	VkImage getVkImage(uint32_t index) const;
	VkImageView getVkImageView(uint32_t index) const;
	VkFormat getSwapChainImageFormat() const;
	VkExtent2D getVkExtent() const;
	uint32_t getCount() const;
	uint32_t getIndex() const;
	uint32_t getFrameNumber() const;
	VkSemaphore getImageAvailableVkSemaphore() const;
	VkSemaphore getRenderFinishedVkSemaphore() const;
	VkFence getInFlightFence() const;

	void advanceCurrentFrameIndex();

public:
	VulkanDevices* m_vulkanDevices;
	VkSurfaceKHR m_surface;

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
};