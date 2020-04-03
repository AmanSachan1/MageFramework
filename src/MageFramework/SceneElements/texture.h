#pragma once

#include <Vulkan/vulkanManager.h>
#include <Vulkan/Utilities/vBufferUtil.h>
#include <Vulkan/Utilities/vImageUtil.h>
#include <Utilities/loadingUtilityForward.h>

class Texture
{
public:
	Texture() = delete;
	Texture(VkDevice lDevice, VkPhysicalDevice pDevice, VkFormat format, uint32_t layerCount, uint32_t mipLevels)
		: m_logicalDevice(lDevice), m_physicalDevice(pDevice), 
		m_format(format), m_layerCount(layerCount), m_mipLevels(mipLevels),
		m_image(VK_NULL_HANDLE), m_imageMemory(VK_NULL_HANDLE), m_imageView(VK_NULL_HANDLE), m_sampler(VK_NULL_HANDLE)
	{}
	~Texture()
	{
		vkDeviceWaitIdle(m_logicalDevice);

		vkDestroyImageView(m_logicalDevice, m_imageView, nullptr);
		vkDestroyImage(m_logicalDevice, m_image, nullptr);
		if (m_sampler)
		{
			vkDestroySampler(m_logicalDevice, m_sampler, nullptr);
		}
		vkFreeMemory(m_logicalDevice, m_imageMemory, nullptr);
	}

	void updateDescriptor()
	{
		m_descriptor.sampler = m_sampler;
		m_descriptor.imageView = m_imageView;
		m_descriptor.imageLayout = m_imageLayout;
	}

	void createViewSamplerAndUpdateDescriptor(bool isMipMapped, VkSamplerAddressMode samplerAddressMode, VkQueue& queue, VkCommandPool& cmdPool)
	{
		// Create 2D image View
		ImageUtil::createImageView(m_logicalDevice, m_image, &m_imageView, VK_IMAGE_VIEW_TYPE_2D, m_format, VK_IMAGE_ASPECT_COLOR_BIT, m_mipLevels, nullptr);

		// Create Texture Sampler
		ImageUtil::createImageSampler(m_logicalDevice, m_sampler, VK_FILTER_LINEAR, VK_FILTER_LINEAR, samplerAddressMode,
			VK_SAMPLER_MIPMAP_MODE_LINEAR, 0, 0, static_cast<float>(m_mipLevels), 16, VK_COMPARE_OP_NEVER);

		// Update descriptor image info member that can be used for setting up descriptor sets
		updateDescriptor();
	}

	void createEmptyTexture(
		uint32_t width, uint32_t height, uint32_t depth, uint32_t layerCount,
		VkQueue& queue, VkCommandPool& cmdPool,
		bool isMipMapped = false,
		VkSamplerAddressMode samplerAddressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL,
		VkImageUsageFlags usage = VK_IMAGE_USAGE_SAMPLED_BIT)
	{
		m_width = static_cast<uint32_t>(width);
		m_height = static_cast<uint32_t>(height);
		m_depth = static_cast<uint32_t>(depth);
		m_mipLevels = isMipMapped ? (static_cast<uint32_t>(std::floor(std::log2(std::max(m_width, m_height)))) + 1) : 1;

		VkExtent3D extent = { m_width, m_height, m_depth };
		ImageUtil::createImage(m_logicalDevice, m_physicalDevice, m_image, m_imageMemory, VK_IMAGE_TYPE_2D, m_format, extent, usage,
			VK_SAMPLE_COUNT_1_BIT, tiling, m_mipLevels, m_layerCount, VK_IMAGE_LAYOUT_UNDEFINED, VK_SHARING_MODE_EXCLUSIVE);

		m_imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		ImageUtil::transitionImageLayout_SingleTimeCommand(m_logicalDevice, queue, cmdPool, m_image, m_format, VK_IMAGE_LAYOUT_UNDEFINED, m_imageLayout, m_mipLevels);

		createViewSamplerAndUpdateDescriptor(isMipMapped, samplerAddressMode, queue, cmdPool);
	}

public:
	uint32_t m_width, m_height, m_depth;
	uint32_t m_mipLevels; // number of levels in the mip chain
	uint32_t m_layerCount;
	VkFormat m_format;
	VkImageLayout m_imageLayout;

	VkImage m_image = VK_NULL_HANDLE;
	VkDeviceMemory m_imageMemory = VK_NULL_HANDLE;
	VkImageView m_imageView = VK_NULL_HANDLE;
	VkSampler m_sampler = VK_NULL_HANDLE;

	VkDescriptorImageInfo m_descriptor;

protected:
	VkDevice m_logicalDevice;
	VkPhysicalDevice m_physicalDevice;
};

class Texture2D : public Texture
{
public:
	Texture2D() = delete;
	Texture2D(VkDevice lDevice, VkPhysicalDevice pDevice, VkQueue& queue, VkCommandPool& cmdPool,
		VkFormat format = VK_FORMAT_R8G8B8A8_UNORM, uint32_t mipLevels = 1)
		: Texture(lDevice, pDevice, format, 1, mipLevels)
	{
		m_depth = 1;
	}
	Texture2D(std::shared_ptr<VulkanManager> vulkanManager, VkQueue& queue, VkCommandPool& cmdPool,
		VkFormat format = VK_FORMAT_R8G8B8A8_UNORM, uint32_t mipLevels = 1)
		: Texture2D(vulkanManager->getLogicalDevice(), vulkanManager->getPhysicalDevice(), queue, cmdPool, format, mipLevels) 
	{}

	void create2DTexture(
		std::string texturePath,
		VkQueue& queue, VkCommandPool& cmdPool,
		bool isMipMapped = false,
		VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL,
		VkImageUsageFlags usage = VK_IMAGE_USAGE_SAMPLED_BIT); //VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | 

	void Texture2D::create2DTexture(
		ImageLoaderOutput& imgOut, VkQueue& queue, VkCommandPool& cmdPool,
		bool isMipMapped = false,
		VkSamplerAddressMode samplerAddressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL,
		VkImageUsageFlags usage = VK_IMAGE_USAGE_SAMPLED_BIT);

	void creationHelperPart1(ImageLoaderOutput& imgOut, VkQueue& queue, VkCommandPool& cmdPool,
		bool isMipMapped, VkImageTiling tiling, VkImageUsageFlags usage);
};

class Texture2DArray : public Texture
{
public:
	// Texture Arrays need all the individual layers need to have the same width and height
	Texture2DArray() = delete;
	Texture2DArray(std::shared_ptr<VulkanManager> vulkanManager, VkQueue& queue, VkCommandPool& cmdPool, 
		uint32_t layerCount, VkFormat format = VK_FORMAT_R8G8B8A8_UNORM, uint32_t mipLevels = 1)
		: Texture(vulkanManager->getLogicalDevice(), vulkanManager->getPhysicalDevice(), format, layerCount, mipLevels)
	{
		m_depth = 1;
	}
	
	// Use individual textures to create a 2D Texture Array
	// If addPadding it true then we additinally add black pixels to the smallest texture to make it the same size as the biggest texture in the 
	void create2DTextureArray(
		std::vector<std::string> texturePaths,
		VkQueue& queue, VkCommandPool& cmdPool,
		bool isMipMapped = false, 
		VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL,
		VkImageUsageFlags usage = VK_IMAGE_USAGE_SAMPLED_BIT);

	void create2DTextureArray( 
		ImageArrayLoaderOutput& imgArrayOut, 
		VkQueue& queue, VkCommandPool& cmdPool,
		bool isMipMapped = false,
		VkSamplerAddressMode samplerAddressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL,
		VkImageUsageFlags usage = VK_IMAGE_USAGE_SAMPLED_BIT);

	void creationHelperPart1(ImageArrayLoaderOutput& imgArrayOut, VkQueue& queue, VkCommandPool& cmdPool,
		bool isMipMapped, VkImageTiling tiling, VkImageUsageFlags usage);
};

class Texture3D : public Texture
{
public:
	Texture3D() = delete;
	Texture3D(std::shared_ptr<VulkanManager> vulkanManager, VkQueue& queue, VkCommandPool& cmdPool, 
		VkFormat format = VK_FORMAT_R8G8B8A8_UNORM, uint32_t mipLevels = 1)
		: Texture(vulkanManager->getLogicalDevice(), vulkanManager->getPhysicalDevice(), format, 1, mipLevels)
	{};

	void create3DTexture(
		std::string texturePath,
		VkQueue& queue, VkCommandPool& cmdPool,
		bool isMipMapped = false,
		VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL,
		VkImageUsageFlags usage = VK_IMAGE_USAGE_SAMPLED_BIT);

	void create3DTextureFromMany2DTextures(
		VkQueue& queue, VkCommandPool& cmdPool,
		int num2DImages, int numChannels,
		const std::string folder_path, const std::string textureBaseName, const std::string fileExtension);
};