#include "Texture.h"
#include <Utilities/loadingUtility.h>

Texture::Texture(VulkanDevices* devices, VkQueue& queue, VkCommandPool& cmdPool, VkFormat format = VK_FORMAT_R8G8B8A8_UNORM)
	: m_logicalDevice(devices->getLogicalDevice()), m_physicalDevice(devices->getPhysicalDevice()), 
	m_graphicsQueue(queue), m_cmdPool(cmdPool), m_format(format),
	m_image(VK_NULL_HANDLE), m_imageMemory(VK_NULL_HANDLE), m_imageView(VK_NULL_HANDLE), m_sampler(VK_NULL_HANDLE)
{}

Texture::~Texture()
{
	vkDestroySampler(m_logicalDevice, m_sampler, nullptr);
	vkDestroyImageView(m_logicalDevice, m_imageView, nullptr);
	vkDestroyImage(m_logicalDevice, m_image, nullptr);
	vkFreeMemory(m_logicalDevice, m_imageMemory, nullptr);
}

void Texture::create2DTexture(std::string texturePath, bool isMipMapped, VkImageTiling tiling, VkImageUsageFlags usage)
{
	VkDeviceSize imageSize;
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	// Use STB library to load image into a buffer
	{
		int texWidth, texHeight, texChannels;
		unsigned char* pixels = loadingUtil::loadImage(texturePath, &texWidth, &texHeight, &texChannels);

		imageSize = texWidth * texHeight * 4;
		m_width = static_cast<uint32_t>(texWidth);
		m_height = static_cast<uint32_t>(texHeight);
		m_depth = 1;
		if (isMipMapped)
		{
			m_mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;
		}
		else
		{
			m_mipLevels = 1;
		}

		BufferUtil::createStagingBuffer(m_physicalDevice, m_logicalDevice, pixels, stagingBuffer, stagingBufferMemory, imageSize);
		loadingUtil::freeImage(pixels);
	}

	VkExtent3D extent = { m_width, m_height, m_depth };
	ImageUtil::createImage(m_logicalDevice, m_physicalDevice, m_image, m_imageMemory, VK_IMAGE_TYPE_2D, m_format, extent, usage,
		VK_SAMPLE_COUNT_1_BIT, tiling, m_mipLevels, 1, VK_IMAGE_LAYOUT_UNDEFINED, VK_SHARING_MODE_EXCLUSIVE);

	// vkCmdCopyBufferToImage will be used to copy the stagingBuffer into the m_textureImage, 
	// but this command requires the image to be in the right layout. So we perform an image Transition

	ImageUtil::transitionImageLayout(m_logicalDevice, m_graphicsQueue, m_cmdPool, m_image, m_format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, m_mipLevels);
	ImageUtil::copyBufferToImage(m_logicalDevice, m_graphicsQueue, m_cmdPool, stagingBuffer, m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, m_width, m_height);

	m_imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	if (isMipMapped)
	{
		// To be able to start sampling from the texture image in the shader, we need to transition it to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		// This transition was done while generating mipmaps
	}
	else
	{
		ImageUtil::transitionImageLayout(m_logicalDevice, m_graphicsQueue, m_cmdPool, m_image, m_format,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_mipLevels);
	}

	// Destroy Staging Buffer
	vkDestroyBuffer(m_logicalDevice, stagingBuffer, nullptr);
	vkFreeMemory(m_logicalDevice, stagingBufferMemory, nullptr);

	// Create 2D image View
	ImageUtil::createImageView(m_logicalDevice, m_image, &m_imageView, VK_IMAGE_VIEW_TYPE_2D, m_format, VK_IMAGE_ASPECT_COLOR_BIT, m_mipLevels, nullptr);

	// Create Texture Sampler
	ImageUtil::createImageSampler(m_logicalDevice, m_sampler,
		VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT,
		VK_SAMPLER_MIPMAP_MODE_LINEAR, 0, 0, static_cast<float>(m_mipLevels), 16, VK_COMPARE_OP_NEVER);

	if (isMipMapped)
	{
		// Generate mipmaps --> also handles transitions of image to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL at each mipLevel
		ImageUtil::generateMipMaps(m_logicalDevice, m_physicalDevice, m_graphicsQueue, m_cmdPool, m_image, m_format, m_width, m_height, m_depth, m_mipLevels);
	}
}

void Texture::createEmpty2DTexture(uint32_t width, uint32_t height, uint32_t depth, bool isMipMapped, 
	VkSamplerAddressMode samplerAddressMode, VkImageTiling tiling, VkImageUsageFlags usage)
{
	m_width = static_cast<uint32_t>(width);
	m_height = static_cast<uint32_t>(height);
	m_depth = 1;
	if (isMipMapped)
	{
		m_mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(m_width, m_height)))) + 1;
	}
	else
	{
		m_mipLevels = 1;
	}

	VkExtent3D extent = { m_width, m_height, m_depth };
	ImageUtil::createImage(m_logicalDevice, m_physicalDevice, m_image, m_imageMemory, VK_IMAGE_TYPE_2D, m_format, extent, usage,
		VK_SAMPLE_COUNT_1_BIT, tiling, m_mipLevels, 1, VK_IMAGE_LAYOUT_UNDEFINED, VK_SHARING_MODE_EXCLUSIVE);

	m_imageLayout = VK_IMAGE_LAYOUT_GENERAL;
	ImageUtil::transitionImageLayout(m_logicalDevice, m_graphicsQueue, m_cmdPool, m_image, m_format, 
		VK_IMAGE_LAYOUT_UNDEFINED, m_imageLayout, m_mipLevels);
	
	// Create 2D image View
	ImageUtil::createImageView(m_logicalDevice, m_image, &m_imageView, VK_IMAGE_VIEW_TYPE_2D, m_format, VK_IMAGE_ASPECT_COLOR_BIT, m_mipLevels, nullptr);

	// Create Texture Sampler
	ImageUtil::createImageSampler(m_logicalDevice, m_sampler, VK_FILTER_LINEAR, VK_FILTER_LINEAR, samplerAddressMode,
		VK_SAMPLER_MIPMAP_MODE_LINEAR, 0, 0, static_cast<float>(m_mipLevels), 16, VK_COMPARE_OP_NEVER);
}

// Getters
uint32_t Texture::getWidth() const
{
	return m_width;
}
uint32_t Texture::getHeight() const
{
	return m_height;
}
uint32_t Texture::getDepth() const
{
	return m_depth;
}
VkFormat Texture::getFormat() const
{
	return m_format;
}
VkImageLayout Texture::getImageLayout() const
{
	return m_imageLayout;
}

VkImage& Texture::getImage()
{
	return m_image;
}
VkDeviceMemory& Texture::getImageMemory()
{
	return m_imageMemory;
}
VkImageView& Texture::getImageView()
{
	return m_imageView;
}
VkSampler& Texture::getSampler()
{
	return m_sampler;
}