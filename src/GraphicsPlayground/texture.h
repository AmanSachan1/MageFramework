#pragma once

#include "vulkanDevices.h"
#include "Utilities/bufferUtility.h"
#include "Utilities/imageUtility.h"

class Texture
{
public:
	Texture() = delete;
	Texture(VulkanDevices* devices, uint32_t width, uint32_t height, uint32_t depth, VkFormat format);
	~Texture();

	void createTexture(VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
						 VkSamplerAddressMode addressMode, float maxAnisotropy);
	void createTextureImage(VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties);
	void createTextureSampler(VkSamplerAddressMode addressMode, float maxAnisotropy);
	void createTextureImageView();
	void create3DTextureFromMany2DTextures(VkDevice logicalDevice, VkCommandPool commandPool,
		const std::string folder_path, const std::string textureBaseName, const std::string fileExtension,
		int num2DImages, int numChannels);

	uint32_t getWidth() const;
	uint32_t getHeight() const;
	uint32_t getDepth() const;
	VkFormat getTextureFormat() const;
	VkImageLayout getTextureLayout() const;
	VkImage getTextureImage();
	VkDeviceMemory getTextureImageMemory();
	VkImageView getTextureImageView();
	VkSampler getTextureSampler();

private:
	VkDevice m_logicalDevice;
	VkPhysicalDevice m_physicalDevice;

	uint32_t m_width, m_height, m_depth;
	VkFormat m_textureFormat;
	VkImageLayout m_textureLayout;

	VkImage m_textureImage = VK_NULL_HANDLE;
	VkDeviceMemory m_textureImageMemory = VK_NULL_HANDLE;
	VkImageView m_textureImageView = VK_NULL_HANDLE;
	VkSampler m_textureSampler = VK_NULL_HANDLE;
};