#include "vulkanPresentation.h"

VulkanPresentation::VulkanPresentation(VulkanDevices* _devices, VkSurfaceKHR& _vkSurface, uint32_t  width, uint32_t height)
	: vulkanDevices(_devices), vkSurface(_vkSurface)
{
	Create(width, height);
}

VulkanPresentation::~VulkanPresentation()
{

}

void VulkanPresentation::Create(uint32_t  width, uint32_t height)
{
	const auto& surfaceCapabilities = vulkanDevices->surfaceCapabilities;
	VkSurfaceFormatKHR surfaceFormat = SwapChainUtils::chooseSwapSurfaceFormat(vulkanDevices->surfaceFormats);
	VkPresentModeKHR presentMode = SwapChainUtils::chooseSwapPresentMode(vulkanDevices->presentModes);
	VkExtent2D extent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
}