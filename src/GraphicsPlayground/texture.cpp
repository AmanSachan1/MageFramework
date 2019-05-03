#include "Texture.h"

Texture::Texture(VulkanDevices* devices, uint32_t width, uint32_t height, uint32_t depth, VkFormat format)
	: m_logicalDevice(devices->getLogicalDevice()), m_physicalDevice(devices->getPhysicalDevice()), 
	m_width(width), m_height(height), m_depth(depth), m_textureFormat(format)
{}

Texture::~Texture()
{
	if (m_textureSampler != VK_NULL_HANDLE) 
	{
		vkDestroySampler(m_logicalDevice, m_textureSampler, nullptr);
	}
	if (m_textureImageView != VK_NULL_HANDLE) 
	{
		vkDestroyImageView(m_logicalDevice, m_textureImageView, nullptr);
	}
	if (m_textureImage != VK_NULL_HANDLE) 
	{
		vkDestroyImage(m_logicalDevice, m_textureImage, nullptr);
	}
	if (m_textureImageMemory != VK_NULL_HANDLE) 
	{
		vkFreeMemory(m_logicalDevice, m_textureImageMemory, nullptr);
	}
}

void Texture::createTextureImage(VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties)
{
	int texWidth, texHeight, texChannels;
	// The pointer that is returned is the first element in an array of pixel values. 
	// The pixels are laid out row by row with 4 bytes per pixel in the case of STBI_rgba_alpha for a total of texWidth * texHeight * 4 values.
	stbi_uc* pixels = stbi_load("textures/texture.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

	VkDeviceSize imageSize = texWidth * texHeight * 4;

	if (!pixels) 
	{
		throw std::runtime_error("failed to load texture image!");
	}

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	BufferUtil::createBuffer(m_physicalDevice, m_logicalDevice, stagingBuffer, stagingBufferMemory, imageSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_SHARING_MODE_EXCLUSIVE,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	void* mappedData;
	vkMapMemory(m_logicalDevice, stagingBufferMemory, 0, imageSize, 0, &mappedData);
	memcpy(mappedData, pixels, static_cast<size_t>(imageSize));
	vkUnmapMemory(m_logicalDevice, stagingBufferMemory);

	stbi_image_free(pixels);

	VkImage textureImage;
	VkDeviceMemory textureImageMemory;
}