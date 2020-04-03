#include "Texture.h"
#include <Utilities/loadingUtility.h>

void Texture2D::create2DTexture(
	std::string texturePath, VkQueue& queue, VkCommandPool& cmdPool,
	bool isMipMapped, VkImageTiling tiling, VkImageUsageFlags usage)
{
	ImageLoaderOutput imgOut;
	loadingUtil::loadImageUsingSTB(texturePath, imgOut, m_logicalDevice, m_physicalDevice);
	create2DTexture( imgOut, queue, cmdPool, isMipMapped, VK_SAMPLER_ADDRESS_MODE_REPEAT, tiling, usage);
}

void Texture2D::create2DTexture(
	ImageLoaderOutput& imgOut, VkQueue& queue, VkCommandPool& cmdPool,
	bool isMipMapped, VkSamplerAddressMode samplerAddressMode, VkImageTiling tiling, VkImageUsageFlags usage)
{
	creationHelperPart1(imgOut, queue, cmdPool, isMipMapped, tiling, usage);
	createViewSamplerAndUpdateDescriptor(isMipMapped, samplerAddressMode, queue, cmdPool);
}

void Texture2D::creationHelperPart1(ImageLoaderOutput& imgOut, VkQueue& queue, VkCommandPool& cmdPool,
	bool isMipMapped, VkImageTiling tiling, VkImageUsageFlags usage)
{
	m_width = static_cast<uint32_t>(imgOut.imgWidth);
	m_height = static_cast<uint32_t>(imgOut.imgHeight);
	m_mipLevels = isMipMapped ? (static_cast<uint32_t>(std::floor(std::log2(std::max(m_width, m_height)))) + 1) : 1;

	VkExtent3D extent = { m_width, m_height, m_depth };
	ImageUtil::createImage(m_logicalDevice, m_physicalDevice, m_image, m_imageMemory, VK_IMAGE_TYPE_2D, m_format, extent, usage,
		VK_SAMPLE_COUNT_1_BIT, tiling, m_mipLevels, m_layerCount, VK_IMAGE_LAYOUT_UNDEFINED, VK_SHARING_MODE_EXCLUSIVE);

	// vkCmdCopyBufferToImage will be used to copy the stagingBuffer into the m_textureImage, 
	// but this command requires the image to be in the right layout. So we perform an image Transition
	ImageUtil::transitionImageLayout_SingleTimeCommand(m_logicalDevice, queue, cmdPool, m_image, m_format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, m_mipLevels);
	ImageUtil::copyBufferToImage_SingleTimeCommand(m_logicalDevice, queue, cmdPool, imgOut.stagingBuffer, m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, m_width, m_height);

	// Destroy Staging Buffer
	vkDestroyBuffer(m_logicalDevice, imgOut.stagingBuffer, nullptr);
	vkFreeMemory(m_logicalDevice, imgOut.stagingBufferMemory, nullptr);

	// Transition Image to final Layout
	m_imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	if (isMipMapped)
	{
		// Generate mipmaps --> also handles transitions of image to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL at each mipLevel
		ImageUtil::generateMipMaps(m_logicalDevice, m_physicalDevice, queue, cmdPool, m_image, m_format, m_width, m_height, m_depth, m_mipLevels);
	}
	else
	{
		ImageUtil::transitionImageLayout_SingleTimeCommand(m_logicalDevice, queue, cmdPool, m_image, m_format,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, m_imageLayout, m_mipLevels);
	}
}

//---------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------

void Texture2DArray::create2DTextureArray(
	std::vector<std::string> texturePaths, VkQueue& queue, VkCommandPool& cmdPool,
	bool isMipMapped, VkImageTiling tiling, VkImageUsageFlags usage)
{
#ifdef DEBUG_MAGE_FRAMEWORK
	assert(texturePaths.size() >= m_layerCount);
	std::cout << "\n Texture paths doesn't contain enough textures to fill the 2D texture array" << std::endl;
#endif

	ImageArrayLoaderOutput imgArrayOut;
	loadingUtil::loadArrayOfImageUsingSTB(texturePaths, imgArrayOut, m_logicalDevice, m_physicalDevice);
	create2DTextureArray(imgArrayOut, queue, cmdPool, isMipMapped, VK_SAMPLER_ADDRESS_MODE_REPEAT, tiling, usage);
}

void Texture2DArray::create2DTextureArray(
	ImageArrayLoaderOutput& imgArrayOut, VkQueue& queue, VkCommandPool& cmdPool,
	bool isMipMapped, VkSamplerAddressMode samplerAddressMode, VkImageTiling tiling, VkImageUsageFlags usage)
{
	creationHelperPart1(imgArrayOut, queue, cmdPool, isMipMapped, tiling, usage);
	createViewSamplerAndUpdateDescriptor(isMipMapped, samplerAddressMode, queue, cmdPool);
}

void Texture2DArray::creationHelperPart1(ImageArrayLoaderOutput& imgArrayOut, VkQueue& queue, VkCommandPool& cmdPool,
	bool isMipMapped, VkImageTiling tiling, VkImageUsageFlags usage)
{
	// The texture array has the same width and height for all the individual textures
	m_width = imgArrayOut.imgWidths[0];
	m_height = imgArrayOut.imgHeights[0];
	m_mipLevels = isMipMapped ? (static_cast<uint32_t>(std::floor(std::log2(std::max(m_width, m_height)))) + 1) : 1;

	// Setup buffer copy regions for each layer including all of its miplevels
	uint32_t bufferOffset = 0;
	const uint32_t imageSize_single = m_width * m_height * 4;
	std::vector<VkBufferImageCopy> bufferCopyRegions;
	for (uint32_t layer = 0; layer < m_layerCount; layer++)
	{
#ifdef DEBUG_MAGE_FRAMEWORK
		if (isMipMapped)
		{
			uint32_t mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(imgArrayOut.imgWidths[layer], imgArrayOut.imgHeights[layer])))) + 1;
			assert(mipLevels == m_mipLevels);
			std::cout << "\n Mipmap Levels of loaded texture don't match ones described in constructor" << std::endl;
		}
		if (imgArrayOut.imgWidths[layer] != m_width && imgArrayOut.imgHeights[layer] != m_height)
		{
			throw std::runtime_error("\n Textures in a texture array need to be of the same size");
		}
#endif

		for (uint32_t level = 0; level < m_mipLevels; level++)
		{
			VkBufferImageCopy bufferCopyRegion = {};
			bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			bufferCopyRegion.imageSubresource.mipLevel = level;
			bufferCopyRegion.imageSubresource.baseArrayLayer = layer;
			bufferCopyRegion.imageSubresource.layerCount = 1;
			bufferCopyRegion.imageExtent.width = m_width;
			bufferCopyRegion.imageExtent.height = m_height;
			bufferCopyRegion.imageExtent.depth = m_depth;
			bufferCopyRegion.bufferOffset = bufferOffset;

			bufferCopyRegions.push_back(bufferCopyRegion);
		}
		bufferOffset += static_cast<uint32_t>(imageSize_single);
	}

	// Make all the textures the same size by adding padding to the smallest ones
	// Assuming all textures are stored from left to right and top to bottom
	// If you do something like this do it during in the load function
	// ImageUtil::fixImagesForTextureArray(m_layerCount, FixTextureFlag::SCALE_UP, m_width, m_height, imgWidths, imgHeights);

	VkExtent3D extent = { m_width, m_height, m_depth };
	ImageUtil::createImage(m_logicalDevice, m_physicalDevice, m_image, m_imageMemory, VK_IMAGE_TYPE_2D, m_format, extent, usage,
		VK_SAMPLE_COUNT_1_BIT, tiling, m_mipLevels, m_layerCount, VK_IMAGE_LAYOUT_UNDEFINED, VK_SHARING_MODE_EXCLUSIVE);

	// vkCmdCopyBufferToImage will be used to copy the stagingBuffer into the m_textureImage, 
	// but this command requires the image to be in the right layout. So we perform an image Transition

	ImageUtil::transitionImageLayout_SingleTimeCommand(m_logicalDevice, queue, cmdPool, m_image, m_format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, m_mipLevels);
	ImageUtil::copyBufferToImage_SingleTimeCommand(m_logicalDevice, queue, cmdPool, imgArrayOut.stagingBuffer, m_image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, static_cast<uint32_t>(bufferCopyRegions.size()), bufferCopyRegions.data());

	// Destroy Staging Buffer
	vkDestroyBuffer(m_logicalDevice, imgArrayOut.stagingBuffer, nullptr);
	vkFreeMemory(m_logicalDevice, imgArrayOut.stagingBufferMemory, nullptr);

	// Transition Images to final Layout
	m_imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	if (isMipMapped)
	{
		// Generate mipmaps --> also handles transitions of image to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL at each mipLevel
		ImageUtil::generateMipMaps(m_logicalDevice, m_physicalDevice, queue, cmdPool, m_image, m_format, m_width, m_height, m_depth, m_mipLevels);
	}
	else
	{
		ImageUtil::transitionImageLayout_SingleTimeCommand(m_logicalDevice, queue, cmdPool, m_image, m_format,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, m_imageLayout, m_mipLevels);
	}
}

//---------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------

void Texture3D::create3DTexture(
	std::string texturePath, VkQueue& queue, VkCommandPool& cmdPool,
	bool isMipMapped, VkImageTiling tiling, VkImageUsageFlags usage)
{

}

void Texture3D::create3DTextureFromMany2DTextures(
	VkQueue& queue, VkCommandPool& cmdPool,	int num2DImages, int numChannels,
	const std::string folder_path, const std::string textureBaseName, const std::string fileExtension)
{

}