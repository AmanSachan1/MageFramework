#pragma once
#include <global.h>

#pragma warning( disable : 26812 ) // C26812: Prefer 'enum class' over 'enum' (Enum.3); Because of Vulkan 

namespace Util
{
	inline VkViewport createViewport(float width, float height, float minDepth = 0.0f, float maxDepth = 1.0f, float x = 0.0f, float y = 1.0f)
	{
		return { x, y, width, height, minDepth, maxDepth };
	}

	inline VkRect2D createRectangle(VkExtent2D extent, VkOffset2D offset = { 0,0 })
	{
		return { offset, extent };
	}
};

namespace FormatUtil
{
	// Format Helper Functions
	inline VkFormat findSupportedFormat(VkPhysicalDevice& pDevice, const std::vector<VkFormat>& candidates,
		VkImageTiling tiling, VkFormatFeatureFlags features)
	{
		for (VkFormat format : candidates)
		{
			VkFormatProperties formatProperties;
			vkGetPhysicalDeviceFormatProperties(pDevice, format, &formatProperties);

			// The VkFormatProperties struct contains three fields:
			// - linearTilingFeatures  : Use cases that are supported with linear tiling
			// - optimalTilingFeatures : Use cases that are supported with optimal tiling
			// - bufferFeatures        : Use cases that are supported for buffers

			if ((tiling == VK_IMAGE_TILING_LINEAR && (formatProperties.linearTilingFeatures & features) == features) ||
				(tiling == VK_IMAGE_TILING_OPTIMAL && (formatProperties.optimalTilingFeatures & features) == features))
			{
				return format;
			}
		}

		throw std::runtime_error("failed to find supported format!");
	}

	inline VkFormat findDepthFormat(VkPhysicalDevice& pDevice)
	{
		return findSupportedFormat(pDevice,
			{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
			VK_IMAGE_TILING_OPTIMAL,
			VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
	}

	inline bool hasStencilComponent(VkFormat format)
	{
		return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
	}
}