#pragma once
#include <global.h>
#include <Vulkan/Utilities/vBufferUtil.h>
#include <Vulkan/Utilities/vCommandUtil.h>
#include <Utilities\generalUtility.h>

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
		VkImageType imageType, VkFormat format, VkExtent3D extents,
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
		l_imageCreationInfo.extent = extents;

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
		VkImageViewType viewType, VkFormat format, VkImageAspectFlags aspectMask, uint32_t mipLevels, const VkAllocationCallbacks* pAllocator)
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
		l_createInfo.subresourceRange = createImageSubResourceRange(aspectMask, 0, mipLevels, 0, 1);

		if (vkCreateImageView(logicalDevice, &l_createInfo, pAllocator, imageView) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create image views!");
		}
	}

	inline void createImageSampler(VkDevice& logicalDevice, VkSampler& imageSampler,
		VkFilter magFilter, VkFilter minFilter, VkSamplerAddressMode addressMode,
		VkSamplerMipmapMode mipmapMode, float mipLodBias, float minLod, float maxLod,
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

	inline void transitionImageLayout(VkDevice& logicalDevice, VkQueue& queue, VkCommandPool& cmdPool, VkCommandBuffer& cmdBuffer,
		VkImage& image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels)
	{
		// Set VkAccessMasks and VkPipelineStageFlags based on the layouts used in the transition
		VkAccessFlags srcAccessMask, dstAccessMask;
		VkPipelineStageFlags srcStageMask, dstStageMask;
		VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;;
		bool flag_ImageLayoutSupported = false;

		srcAccessMask = 0;
		dstAccessMask = 0;
		srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dstStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

		switch (oldLayout)
		{
		case VK_IMAGE_LAYOUT_UNDEFINED:
			srcAccessMask = 0;
			srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

			flag_ImageLayoutSupported = true;
			break;

		case VK_IMAGE_LAYOUT_GENERAL:
			srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT; // Because general?
			srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

			flag_ImageLayoutSupported = true;
			break;

		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
			srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;

			flag_ImageLayoutSupported = true;
			break;

		case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
			srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
			srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

			flag_ImageLayoutSupported = true;
			break;
		}

		switch (newLayout)
		{
		case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
			dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;

			flag_ImageLayoutSupported = true;
			break;

		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
			dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;

			flag_ImageLayoutSupported = true;
			break;

		case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
			dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;

			aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
			if (FormatUtil::hasStencilComponent(format))
			{
				aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
			}

			flag_ImageLayoutSupported = true;
			break;

		case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
			dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

			flag_ImageLayoutSupported = true;
			break;

		case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
			dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

			flag_ImageLayoutSupported = true;
			break;
		}

		if (!flag_ImageLayoutSupported)
		{
			throw std::invalid_argument("unsupported layout transition!");
		}

		VkImageSubresourceRange imageSubresourceRange = createImageSubResourceRange(aspectMask, 0, mipLevels, 0, 1);
		VkImageMemoryBarrier imageBarrier = createImageMemoryBarrier(image, oldLayout, newLayout, srcAccessMask, dstAccessMask, imageSubresourceRange);
		VulkanCommandUtil::pipelineBarrier(cmdBuffer, srcStageMask, dstStageMask, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier);
	}

	inline void transitionImageLayout_SingleTimeCommand(VkDevice& logicalDevice, VkQueue& queue, VkCommandPool& cmdPool,
		VkImage& image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels)
	{
		VkCommandBuffer cmdBuffer;
		VulkanCommandUtil::beginSingleTimeCommand(logicalDevice, cmdPool, cmdBuffer);
		transitionImageLayout(logicalDevice, queue, cmdPool, cmdBuffer,	image, format, oldLayout, newLayout, mipLevels);
		VulkanCommandUtil::endAndSubmitSingleTimeCommand(logicalDevice, queue, cmdPool, cmdBuffer);
	}

	inline void copyBufferToImage_SingleTimeCommand(VkDevice& logicalDevice, VkQueue& queue, VkCommandPool& cmdPool,
		VkBuffer& buffer, VkImage& image, VkImageLayout dstImageLayout, uint32_t width, uint32_t height)
	{
		VkCommandBuffer cmdBuffer;
		VulkanCommandUtil::beginSingleTimeCommand(logicalDevice, cmdPool, cmdBuffer);

		const uint32_t regionCount = 1;
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
		vkCmdCopyBufferToImage(cmdBuffer, buffer, image, dstImageLayout, regionCount, &region);

		VulkanCommandUtil::endAndSubmitSingleTimeCommand(logicalDevice, queue, cmdPool, cmdBuffer);
	}

	inline void copyImageToImage(VkDevice& logicalDevice, VkQueue& queue, VkCommandPool& cmdPool, VkCommandBuffer cmdBuffer,
		VkImage& srcImage, VkImageLayout srcImageLayout, VkImage& dstImage, VkImageLayout dstImageLayout,
		uint32_t width, uint32_t height, uint32_t depth = 1)
	{
		const uint32_t regionCount = 1;
		VkImageCopy region = {};

		// imageSubresource, imageOffset and imageExtent fields indicate to which part of the image we want to copy the pixels.
		region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.srcSubresource.mipLevel = 0;
		region.srcSubresource.baseArrayLayer = 0;
		region.srcSubresource.layerCount = 1;

		region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.dstSubresource.mipLevel = 0;
		region.dstSubresource.baseArrayLayer = 0;
		region.dstSubresource.layerCount = 1;

		// srcOffset and dstOffset select the initial x, y, and z offsets in texels of the sub - regions of the source and destination image data.
		region.srcOffset = { 0, 0, 0 };
		region.dstOffset = { 0, 0, 0 };

		// extent is the size in texels of the image to copy 
		region.extent = { width, height, depth };

		// The fourth parameter indicates which layout the image is currently using
		vkCmdCopyImage(cmdBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, &region);
	}

	inline VkImageBlit imageBlit(int32_t mipWidth, int32_t mipHeight, int32_t mipDepth, uint32_t srcMipLevel, uint32_t dstMipLevel)
	{
		// specify the regions that will be used in the blit operation.
		VkImageBlit l_blit = {};

		// srcOffsets array determine the 3D region that data will be blitted from		
		l_blit.srcOffsets[0] = { 0, 0, 0 };
		l_blit.srcOffsets[1] = { mipWidth, mipHeight, mipDepth };

		l_blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		l_blit.srcSubresource.mipLevel = srcMipLevel; //i-1
		l_blit.srcSubresource.baseArrayLayer = 0;
		l_blit.srcSubresource.layerCount = 1;

		// dstOffsets determines the region that data will be blitted to
		// dimensions of the dstOffsets[1] are divided by two since each mip level is half the size of the previous level.
		l_blit.dstOffsets[0] = { 0, 0, 0 };
		l_blit.dstOffsets[1] = { mipWidth  > 1 ? mipWidth / 2 : 1,
			mipHeight > 1 ? mipHeight / 2 : 1,
			mipDepth  > 1 ? mipDepth / 2 : 1 };

		l_blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		l_blit.dstSubresource.mipLevel = dstMipLevel; //i
		l_blit.dstSubresource.baseArrayLayer = 0;
		l_blit.dstSubresource.layerCount = 1;

		return l_blit;
	}

	inline void generateMipMaps(VkDevice& logicalDevice, VkPhysicalDevice& pDevice, VkQueue& queue, VkCommandPool& cmdPool,
		VkImage& image, VkFormat imgFormat, int32_t imgWidth, int32_t imgHeight, int32_t imgDepth, uint32_t mipLevels)
	{
		// Our texture image has multiple mip levels, but the staging buffer can only be used to fill mip level 0. 
		// The other levels are still undefined. To fill these levels we need to generate the data from the single level that we have.

		// Use the vkCmdBlitImage command. This command performs copying, scaling, and filtering operations.
		// We will call this multiple times to blit data to each level of our texture image.

		// VkCmdBlit is considered a transfer operation, use the texture image as both the source and destination of a transfer.
		// This is why VK_IMAGE_USAGE_TRANSFER_SRC_BIT is a part of the texture's default usage flags

		// For optimal performance, the source image should be in VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL 
		// and the destination image should be in VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL. 
		// Vulkan allows us to transition each mip level of an image independently. 
		// Each blit will only deal with two mip levels at a time, 
		// so we can transition each level into the optimal layout between blits commands.

		// vkCmdBlitImage is not guaranteed to be supported on all platforms
		// It requires the texture image format we use to support linear filtering, 

		// Check if image format supports linear blitting
		VkFormatProperties formatProperties;
		vkGetPhysicalDeviceFormatProperties(pDevice, imgFormat, &formatProperties);

		// We create texture images with the optimal tiling format, so we need to check optimalTilingFeatures
		if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
		{
			throw std::runtime_error("texture image format does not support linear blitting!");
		}

		VkCommandBuffer cmdBuffer;
		VulkanCommandUtil::beginSingleTimeCommand(logicalDevice, cmdPool, cmdBuffer);

		VkAccessFlags srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		VkAccessFlags dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		VkImageLayout oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		VkImageLayout newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		VkImageSubresourceRange imageSubresourceRange = createImageSubResourceRange(VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1);
		VkImageMemoryBarrier imageBarrier = createImageMemoryBarrier(image, oldLayout, newLayout, srcAccessMask, dstAccessMask, imageSubresourceRange);

		int32_t mipWidth = imgWidth;
		int32_t mipHeight = imgHeight;
		int32_t mipDepth = imgDepth;

		// Reuse VkImageMemoryBarrier
		// imageSubresourceRange.miplevel, oldLayout, newLayout, srcAccessMask, and dstAccessMask will be changed for each transition.
		for (uint32_t i = 1; i < mipLevels; i++)
		{
			// The source mip level was just transitioned to VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL 
			// and the destination level is still in VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
			imageBarrier.subresourceRange.baseMipLevel = i - 1;
			imageBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			imageBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			imageBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			imageBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

			VulkanCommandUtil::pipelineBarrier(cmdBuffer,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
				0, nullptr,
				0, nullptr,
				1, &imageBarrier);

			VkImageBlit imgBlit = imageBlit(mipWidth, mipHeight, mipDepth, i - 1, i);

			VulkanCommandUtil::blitImage(cmdBuffer,
				image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				imgBlit, VK_FILTER_LINEAR);

			// Use the barrier to transition mip level i - 1 to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL.
			// This transition waits on the current blit command to finish
			imageBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			imageBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			imageBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			VulkanCommandUtil::pipelineBarrier(cmdBuffer,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
				0, nullptr,
				0, nullptr,
				1, &imageBarrier);

			// For next mipLevel
			if (mipWidth > 1) { mipWidth /= 2; }
			if (mipHeight > 1) { mipHeight /= 2; }
			if (mipDepth > 1) { mipDepth /= 2; }
		}

		// This barrier transitions the last mip level from VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL. 
		// This wasn't handled by the loop, since the last mip level is never blitted from.
		imageBarrier.subresourceRange.baseMipLevel = mipLevels - 1;
		imageBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		imageBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		imageBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		VulkanCommandUtil::pipelineBarrier(cmdBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
			0, nullptr,
			0, nullptr,
			1, &imageBarrier);

		VulkanCommandUtil::endAndSubmitSingleTimeCommand(logicalDevice, queue, cmdPool, cmdBuffer);
	}
}