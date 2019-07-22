#pragma once

#include <global.h>
#include <Utilities/bufferUtility.h>
#include <Utilities/renderUtility.h>
#include <Utilities/generalUtility.h>

#include "VulkanSetup/vulkanDevices.h"
#include "VulkanSetup/vulkanPresentation.h"

struct FrameBufferAttachment
{
	// Each framebuffer attachment needs its VkImage and associated VkDeviceMemory even though they aren't directly referenced in our code, 
	// because they are intrinsically tied to the VkImageView which is referenced in the frame buffer.
	// FrameBufferAttachments aren't implemented as texture objects because then we'd unnecessarily create multiple sampler objects
	VkImage image = VK_NULL_HANDLE;
	VkDeviceMemory memory = VK_NULL_HANDLE;
	VkImageView view = VK_NULL_HANDLE;
	VkFormat format;
};

struct RenderPassInfo
{
	VkExtent2D extents;
	VkRenderPass renderPass;
	std::vector<VkFramebuffer> frameBuffers;
	std::vector<FrameBufferAttachment> color;
	VkSampler sampler; // [optional] Sampler's are independent of actual images and can be used to sample any image
	std::vector<VkDescriptorImageInfo> imageSetInfo; //[optional] for use later in a descriptor set
};

// Renderpass that has subpasses
struct PostProcessRP
{
	VkRenderPass renderPass;
	std::vector<VkFramebuffer> inOutImage;  // read from and write to
};
struct PostProcessRPStandAlone
{
	VkRenderPass renderPass;
	std::vector<VkFramebuffer> framebuffer;  // write to
	std::vector<FrameBufferAttachment> inputImage; // read from
};

struct PostProcessRenderPasses
{
	VkExtent2D extents;
	VkSampler sampler; // should be able to continue to use a single sampler for all the images as long as theey are the same format and size

	PostProcessRP standard; // has subpasses, i.e. chains together post process effects that don't need to sample adjacent pixels
	std::vector<PostProcessRPStandAlone> standalonePasses; //passes that have to be stand alone because they rely on adjacent pixel data
	
	std::vector<VkDescriptorImageInfo> imageSetInfo_StandardRP; //for use later in a descriptor set
	std::vector<VkDescriptorImageInfo> imageSetInfo_StandAloneRP; //for use later in a descriptor set
};


// This class will manage renderPasses their subpasses and the associated frameBuffers and frameBufferAttachments
// It should help abstract away that detail from the renderer class and prevent the renderpass stuff from being coupled with other things
class RenderPassManager
{
public:
	RenderPassManager() = delete;
	RenderPassManager(VulkanDevices* devices, VkQueue& queue, VkCommandPool& cmdPool,
		VulkanPresentation* presentationObject,	int numSwapChainImages, VkExtent2D windowExtents);
	~RenderPassManager();
	void cleanup();
	void recreate(VkExtent2D windowExtents);

private:
	void createDepthResources();
	void createRenderPasses();
	void createFrameBuffers();

	void createRenderPass_ToDisplay(VkFormat swapChainImageFormat);
	void createRenderPass_geomRasterPass(VkFormat colorFormat);
	void createRenderPass_32bitPostProcess(VkFormat colorFormat32bit, VkFormat colorFormat8bit);
	void createRenderPass_8bitPostProcess(VkFormat colorFormat8bit);
	void renderPassCreationHelper_typical(
		VkFormat colorFormat, VkFormat depthFormat, VkImageLayout finalLayout,
		VkSubpassDescription &subpassDescriptions,
		std::vector<VkAttachmentDescription> &attachments, 
		std::vector<VkAttachmentReference> &attachmentReferences);
	void postProcessRPHelper_standard(VkFormat colorFormat, VkFormat depthFormat,
		VkSubpassDescription &subpassDescriptions,
		std::vector<VkAttachmentDescription> &attachments,
		std::vector<VkAttachmentReference> &attachmentReferences);
	void postProcessRPHelper_standalone(
		VkFormat colorFormat, VkFormat depthFormat, VkImageLayout finalLayout,
		VkSubpassDescription &subpassDescriptions,
		std::vector<VkAttachmentDescription> &attachments, 
		std::vector<VkAttachmentReference> &attachmentReferences);

	void createFrameBufferAttachment(std::vector<FrameBufferAttachment>& fbAttachments,
		VkSampler& sampler, VkFormat imgFormat, VkExtent2D& imageExtent);
	void createDepthAttachment(FrameBufferAttachment& depthAttachment, VkExtent2D& imageExtent);

private:
	VulkanDevices* m_devices;
	VkDevice m_logicalDevice;
	VkPhysicalDevice m_pDevice;
	VkQueue m_graphicsQueue;
	VkCommandPool m_cmdPool; //graphics command pool
	VulkanPresentation* m_presentationObj;
	uint32_t m_numSwapChainImages;
	VkExtent2D m_windowExtents;

	// Depth is going to be common to the scene across render passes as well
	FrameBufferAttachment m_depth;

public:
	// Render Passes
	RenderPassInfo m_toDisplayRenderPass; //renders to actual swapChain Images -- Composite pass
	RenderPassInfo m_geomRasterPass; // Renders to an offscreen framebuffer
	PostProcessRenderPasses m_32bitPostProcessPasses;
	PostProcessRenderPasses m_8bitPostProcessPasses;
};