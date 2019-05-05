#include "Texture.h"
#include <Utilities/stbUtility.h>

Texture::Texture(VulkanDevices* devices, VkQueue& queue, VkCommandPool& cmdPool, VkFormat format = VK_FORMAT_R8G8B8A8_SNORM)
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

void Texture::create2DTexture(std::string texturePath, VkImageTiling tiling, VkImageUsageFlags usage)
{	
	VkDeviceSize imageSize;
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	// Use STB library to load image into a buffer
	{
		int texWidth, texHeight, texChannels;
		unsigned char* pixels = stbUtil::loadImage(texturePath, &texWidth, &texHeight, &texChannels);

		imageSize = texWidth * texHeight * 4;
		m_width = static_cast<uint32_t>(texWidth);
		m_height = static_cast<uint32_t>(texHeight);
		m_depth = 1;

		BufferUtil::createStagingBuffer(m_physicalDevice, m_logicalDevice, pixels, stagingBuffer, stagingBufferMemory, imageSize);
		stbUtil::freeImage(pixels);
	}
	
	ImageUtil::createImage(m_logicalDevice, m_physicalDevice, m_image, m_imageMemory,
		VK_IMAGE_TYPE_2D, m_format, m_width, m_height, m_depth,	usage,
		VK_SAMPLE_COUNT_1_BIT, tiling, 1, 1, VK_IMAGE_LAYOUT_UNDEFINED, VK_SHARING_MODE_EXCLUSIVE);

	// vkCmdCopyBufferToImage will be used to copy the stagingBuffer into the m_textureImage, 
	// but this command requires the image to be in the right layout. So we perform an image Transition
	
	ImageUtil::transitionImageLayout(m_logicalDevice, m_graphicsQueue, m_cmdPool, m_image, m_format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	ImageUtil::copyBufferToImage(m_logicalDevice, m_graphicsQueue, m_cmdPool, stagingBuffer, m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, m_width, m_height);

	// To be able to start sampling from the texture image in the shader, we need to transition it to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	ImageUtil::transitionImageLayout(m_logicalDevice, m_graphicsQueue, m_cmdPool, m_image, m_format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	m_imageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

	// Destroy Staging Buffer
	vkDestroyBuffer(m_logicalDevice, stagingBuffer, nullptr);
	vkFreeMemory(m_logicalDevice, stagingBufferMemory, nullptr);

	// Create 2D image View
	ImageUtil::createImageView(m_logicalDevice, m_image, &m_imageView, VK_IMAGE_VIEW_TYPE_2D, m_format, nullptr);

	// Create Texture Sampler
	ImageUtil::createImageSampler(m_logicalDevice, m_sampler,
		VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT,
		VK_SAMPLER_MIPMAP_MODE_LINEAR, 0.0f, 0.0f, 0.0f, 16, VK_COMPARE_OP_NEVER);
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