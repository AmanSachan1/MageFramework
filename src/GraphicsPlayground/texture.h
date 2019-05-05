#pragma once

#include "vulkanDevices.h"
#include <Utilities/bufferUtility.h>
#include <Utilities/imageUtility.h>

class Texture
{
public:
	Texture() = delete;
	Texture(VulkanDevices* devices, VkQueue& queue, VkCommandPool& cmdPool, VkFormat format);
	~Texture();

	void create2DTexture(std::string texturePath, VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL, VkImageUsageFlags usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
	void create3DTexture(std::string texturePath, VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL, VkImageUsageFlags usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);

	void create3DTextureFromMany2DTextures(VkDevice logicalDevice, VkCommandPool commandPool, int num2DImages, int numChannels,
		const std::string folder_path, const std::string textureBaseName, const std::string fileExtension );
	
	uint32_t getWidth() const;
	uint32_t getHeight() const;
	uint32_t getDepth() const;
	VkFormat getFormat() const;
	VkImageLayout getImageLayout() const;

	VkImage& getImage();
	VkDeviceMemory& getImageMemory();
	VkImageView& getImageView();
	VkSampler& getSampler();

private:
	VkDevice m_logicalDevice;
	VkPhysicalDevice m_physicalDevice;

	// We keep handles to the graphicsQueue and commandPool because we create and submit commands outside of the constructor
	// These commands are for things like image transitions, copying buffers into VkImage's, etc
	VkQueue m_graphicsQueue;
	VkCommandPool m_cmdPool;

	uint32_t m_width, m_height, m_depth;
	VkFormat m_format;
	VkImageLayout m_imageLayout;

	VkImage m_image = VK_NULL_HANDLE;
	VkDeviceMemory m_imageMemory = VK_NULL_HANDLE;
	VkImageView m_imageView = VK_NULL_HANDLE;
	VkSampler m_sampler = VK_NULL_HANDLE;
};