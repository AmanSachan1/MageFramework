#pragma once
#include <global.h>
#include <Utilities/bufferUtility.h>
#include <Utilities/generalUtility.h>
#include <Utilities/commandUtility.h>

namespace ImageUtil
{
	inline VkImageSubresourceRange createImageSubResourceRange(VkImageAspectFlags aspectMask,
		uint32_t baseMipLevel, uint32_t levelCount, uint32_t baseArrayLayer, uint32_t layerCount)
	{
		VkImageSubresourceRange l_imageSubresourceRange = {};
		l_imageSubresourceRange.aspectMask = aspectMask;
		l_imageSubresourceRange.baseMipLevel = baseMipLevel;
		l_imageSubresourceRange.levelCount = levelCount;
		l_imageSubresourceRange.baseArrayLayer = baseArrayLayer;
		l_imageSubresourceRange.layerCount = layerCount;

		return l_imageSubresourceRange;
	}

	inline void createImage(VkDevice& logicalDevice, VkPhysicalDevice& pDevice, VkImage& image, VkDeviceMemory& imageMemory,
		VkImageType imageType, VkFormat format, uint32_t width, uint32_t height, uint32_t depth,
		VkImageUsageFlags usage, VkSampleCountFlagBits samples, VkImageTiling tiling,
		uint32_t mipLevels, uint32_t arrayLayers, VkImageLayout initialLayout, VkSharingMode sharingMode)
	{
		VkImageCreateInfo l_imageCreationInfo = {};
		l_imageCreationInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		l_imageCreationInfo.flags = 0;

		// imageType defines what kind of coordinate system the texels in the image are going to be addressed as. 
		// It is possible to create 1D, 2D and 3D images.
		l_imageCreationInfo.imageType = imageType;
		// The image format should be the same as the pixels in the buffer, otherwise the copy operation will fail.
		// It is possible that the format is not supported by the graphics hardware. 
		// You should ideally have a list of acceptable alternatives and go with the best one that is supported.
		// VK_FORMAT_R8G8B8A8_UNORM has wide spread acceptance almost everywhere
		l_imageCreationInfo.format = format;
		l_imageCreationInfo.extent.width = width;
		l_imageCreationInfo.extent.height = height;
		l_imageCreationInfo.extent.depth = depth;

		l_imageCreationInfo.usage = usage;
		// The samples flag is related to multisampling.
		l_imageCreationInfo.samples = samples;

		// The tiling field can have one of two values:
		// VK_IMAGE_TILING_LINEAR: Texels are laid out in row - major order like our pixels array
		// VK_IMAGE_TILING_OPTIMAL : Texels are laid out in an implementation defined order for optimal access

		// Unlike the layout of an image, the tiling mode cannot be changed at a later time.
		// If you want to be able to directly access texels in the memory of the image, then you must use VK_IMAGE_TILING_LINEAR
		l_imageCreationInfo.tiling = tiling;

		l_imageCreationInfo.mipLevels = mipLevels;
		l_imageCreationInfo.arrayLayers = arrayLayers;

		// There are only two possible values for the initialLayout of an image:
		// VK_IMAGE_LAYOUT_UNDEFINED: Not usable by the GPU and the very first transition will discard the texels.
		// VK_IMAGE_LAYOUT_PREINITIALIZED : Not usable by the GPU, but the first transition will preserve the texels.
		l_imageCreationInfo.initialLayout = initialLayout;
		l_imageCreationInfo.sharingMode = sharingMode;

		if (vkCreateImage(logicalDevice, &l_imageCreationInfo, nullptr, &image) != VK_SUCCESS) 
		{
			throw std::runtime_error("failed to create image!");
		}

		VkMemoryRequirements memRequirements;
		vkGetImageMemoryRequirements(logicalDevice, image, &memRequirements);

		VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = BufferUtil::findMemoryType(pDevice, memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		if (vkAllocateMemory(logicalDevice, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to allocate image memory!");
		}

		vkBindImageMemory(logicalDevice, image, imageMemory, 0);
	}

	inline void createImageView(VkDevice& logicalDevice, VkImage& image, VkImageView* imageView,
		VkImageViewType viewType, VkFormat format, VkImageAspectFlags aspectMask, const VkAllocationCallbacks* pAllocator)
	{
		VkImageViewCreateInfo l_createInfo = {};
		l_createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		l_createInfo.image = image;

		l_createInfo.viewType = viewType;
		l_createInfo.format = format;

		l_createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		l_createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		l_createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		l_createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		
		//No Mipmapping and no multiple targets
		l_createInfo.subresourceRange = createImageSubResourceRange(aspectMask, 0, 1, 0, 1);
		
		if (vkCreateImageView(logicalDevice, &l_createInfo, pAllocator, imageView) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create image views!");
		}
	}

	inline void createImageSampler(VkDevice& logicalDevice, VkSampler& imageSampler,
		VkFilter magFilter, VkFilter minFilter, VkSamplerAddressMode addressMode,
		VkSamplerMipmapMode mipmapMode, float mipLodBias, float minLod,	float maxLod,
		float maxAnisotropy = 16, VkCompareOp compareOp = VK_COMPARE_OP_NEVER)
	{
		// It is possible for shaders to read texels directly from images, but that is not very common when they are used as textures. 
		// Textures are usually accessed through samplers, which will apply filtering and transformations to compute the final color that is retrieved.

		VkSamplerCreateInfo l_samplerInfo = {};
		l_samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

		// The magFilter and minFilter fields specify how to interpolate texels that are magnified or minified. 
		l_samplerInfo.magFilter = magFilter;
		l_samplerInfo.minFilter = minFilter;

		l_samplerInfo.addressModeU = addressMode;
		l_samplerInfo.addressModeV = addressMode;
		l_samplerInfo.addressModeW = addressMode;

		// Mipmap behaviour
		l_samplerInfo.mipmapMode = mipmapMode;
		l_samplerInfo.mipLodBias = mipLodBias;
		l_samplerInfo.minLod = minLod;
		l_samplerInfo.maxLod = maxLod;

		// Anisotropy
		if (maxAnisotropy > 1)
		{
			l_samplerInfo.anisotropyEnable = VK_TRUE;
			l_samplerInfo.maxAnisotropy = maxAnisotropy;

			if (maxAnisotropy > 16)
			{
				l_samplerInfo.maxAnisotropy = 16;
			}
		}
		else
		{
			l_samplerInfo.anisotropyEnable = VK_FALSE;
		}

		// Compare Operation
		// If a comparison function is enabled, then texels will first be compared to a value, 
		// and the result of that comparison is used in filtering operations. 
		// This is mainly used for percentage-closer filtering on shadow maps. 
		if (compareOp == VK_COMPARE_OP_NEVER)
		{
			l_samplerInfo.compareEnable = VK_FALSE;
			l_samplerInfo.compareOp = VK_COMPARE_OP_NEVER;
		}
		else
		{
			l_samplerInfo.compareEnable = VK_TRUE;
			l_samplerInfo.compareOp = compareOp;
		}

		l_samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		l_samplerInfo.unnormalizedCoordinates = VK_FALSE;

		if (vkCreateSampler(logicalDevice, &l_samplerInfo, nullptr, &imageSampler) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create image sampler!");
		}
	}

	inline VkImageMemoryBarrier createImageMemoryBarrier(VkImage& image, VkImageLayout oldLayout, VkImageLayout newLayout,
		VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, VkImageSubresourceRange& imgSubresourceRange,
		uint32_t srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED, uint32_t dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED)
	{
		// A pipeline barrier like VkImageMemoryBarrier is generally used to synchronize access to resources, 
		// like ensuring that a write to a buffer completes before reading from it, 
		// but it can also be used to transition image layouts and transfer queue family ownership when VK_SHARING_MODE_EXCLUSIVE is used. 
		// There is an equivalent VkBufferMemoryBarrier to do this for buffers.

		VkImageMemoryBarrier l_imageBarrier = {};
		l_imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		l_imageBarrier.image = image;

		l_imageBarrier.oldLayout = oldLayout;
		l_imageBarrier.newLayout = newLayout;

		l_imageBarrier.srcAccessMask = srcAccessMask;
		l_imageBarrier.dstAccessMask = dstAccessMask;

		// If you are using the barrier to transfer queue family ownership, then these next two fields should be the indices of the queue families. 
		// They must be set to VK_QUEUE_FAMILY_IGNORED if you don't want to do this (not the default value!).
		l_imageBarrier.srcQueueFamilyIndex = srcQueueFamilyIndex;
		l_imageBarrier.dstQueueFamilyIndex = dstQueueFamilyIndex;

		l_imageBarrier.subresourceRange = imgSubresourceRange;

		return l_imageBarrier;
	}

	inline void transitionImageLayout(VkDevice& logicalDevice, VkQueue& queue, VkCommandPool& cmdPool, 
		VkImage& image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
	{
		VkCommandBuffer cmdBuffer;
		VulkanCommandUtil::beginSingleTimeCommand(logicalDevice, cmdPool, cmdBuffer);

		// Set VkAccessMasks and VkPipelineStageFlags based on the layouts used in the transition
		VkAccessFlags srcAccessMask, dstAccessMask;
		VkPipelineStageFlags srcStageMask, dstStageMask;
		VkImageAspectFlags aspectMask;

		if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED)
		{
			srcAccessMask = 0;
			srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

			if (newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
			{
				dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
				aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			}
			else if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
			{
				dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
				dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;

				aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
				if (FormatUtil::hasStencilComponent(format))
				{
					aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
				}
			}
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && 
				 newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		{
			srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
			dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

			aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		}
		else
		{
			throw std::invalid_argument("unsupported layout transition!");
		}
		
		VkImageSubresourceRange imageSubresourceRange = createImageSubResourceRange(aspectMask, 0, 1, 0, 1);
		VkImageMemoryBarrier imageBarrier = createImageMemoryBarrier(image, oldLayout, newLayout, srcAccessMask, dstAccessMask, imageSubresourceRange);
		VulkanCommandUtil::pipelineBarrier(cmdBuffer, srcStageMask, dstStageMask, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier);
		
		VulkanCommandUtil::endAndSubmitSingleTimeCommand(logicalDevice, queue, cmdPool, cmdBuffer);
	}

	inline void copyBufferToImage(VkDevice& logicalDevice, VkQueue& queue, VkCommandPool& cmdPool, 
		VkBuffer& buffer, VkImage& image, VkImageLayout dstImageLayout, uint32_t width, uint32_t height)
	{
		VkCommandBuffer cmdBuffer;
		VulkanCommandUtil::beginSingleTimeCommand(logicalDevice, cmdPool, cmdBuffer);

		// VkBufferImageCopy specifies which part of the buffer is going to be copied to which part of the image.
		VkBufferImageCopy region = {};

		// bufferOffset specifies the byte offset in the buffer at which the pixel values start.
		region.bufferOffset = 0;

		// bufferRowLength and bufferImageHeight fields specify how the pixels are laid out in memory. 
		// For example, you could have some padding bytes between rows of the image. 
		// Specifying 0 for both indicates that the pixels are simply tightly packed like they are in our case. 
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;

		// imageSubresource, imageOffset and imageExtent fields indicate to which part of the image we want to copy the pixels.
		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;

		region.imageOffset = { 0, 0, 0 };
		region.imageExtent = { width, height, 1 };

		// The fourth parameter indicates which layout the image is currently using
		vkCmdCopyBufferToImage(	cmdBuffer, buffer, image, dstImageLayout, 1, &region );

		VulkanCommandUtil::endAndSubmitSingleTimeCommand(logicalDevice, queue, cmdPool, cmdBuffer);
	}
}