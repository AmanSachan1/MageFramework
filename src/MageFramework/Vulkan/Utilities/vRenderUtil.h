#pragma once
#include <global.h>
#include <Vulkan/Utilities/vImageUtil.h>

// Disable warning C26495: Uninitialized variables (type.6); In various structs below
#pragma warning( disable : 26495 )

struct FrameBufferAttachment
{
	// Each framebuffer attachment needs its VkImage and associated VkDeviceMemory even though they aren't directly referenced in our code, 
	// because they are intrinsically tied to the VkImageView which is referenced in the frame buffer.
	// FrameBufferAttachments aren't implemented as texture objects because then we'd unnecessarily create multiple sampler objects
	VkImage image = VK_NULL_HANDLE;
	VkDeviceMemory memory = VK_NULL_HANDLE;
	VkImageView view = VK_NULL_HANDLE;
	VkFormat format = VK_FORMAT_UNDEFINED;
};

struct RenderPassInfo
{
	VkExtent2D extents;
	VkRenderPass renderPass;
	std::vector<VkFramebuffer> frameBuffers;
	std::vector<FrameBufferAttachment> color;
	VkSampler sampler; // [optional] Sampler's are independent of actual images and can be used to sample any image
	std::vector<VkDescriptorImageInfo> imageSetInfo; // [optional] for use later in a descriptor set
};

// RPI -- Render Pass Information
struct PostProcessRPI
{
	int serialIndex; // Ordering in list of post process effects that exist in PostProcessManager 
	POST_PROCESS_TYPE postType;
	VkRenderPass renderPass;
	std::vector<VkFramebuffer> frameBuffers;
	std::vector<VkDescriptorImageInfo> imageSetInfo; // [optional] for use later in a descriptor set, stores output data
	std::vector<VkDescriptorSet> descriptors; // 3 * descriptors
};

struct PostProcessDescriptors
{
	int serialIndex;
	std::string descriptorName;
	VkDescriptorSetLayout postProcess_DSL;
	std::vector<VkDescriptorSet> postProcess_DSs;
};

struct PostProcessPushConstants
{
	// This can only be 128 bytes long
	// Updating any of these values requires rebuilding of commandbuffers
	float toneMap_WhitePoint = 10.0f;
	float toneMap_Exposure = 2.5f;
};

namespace RenderPassUtil
{
	inline VkSubpassDescription subpassDescription(
		VkPipelineBindPoint pipelineBindPoint,
		uint32_t inputAttachmentCount, const VkAttachmentReference* pInputAttachments,
		uint32_t colorAttachmentCount, const VkAttachmentReference* pColorAttachments,
		const VkAttachmentReference* pResolveAttachments,
		const VkAttachmentReference* pDepthStencilAttachment,
		uint32_t preserveAttachmentCount, const uint32_t* pPreserveAttachments)
	{
		// The following other types of attachments can be referenced by a subpass:
		// - pInputAttachments: Attachments that are read from a shader
		// - pResolveAttachments : Attachments used for multisampling color attachments
		// - pDepthStencilAttachment : Attachments for depth and stencil data
		// - pPreserveAttachments : Attachments that are not used by this subpass, but for which the data must be preserved

		VkSubpassDescription l_subpassDescription = {};
		l_subpassDescription.pipelineBindPoint = pipelineBindPoint;
		l_subpassDescription.inputAttachmentCount = inputAttachmentCount;
		l_subpassDescription.pInputAttachments = pInputAttachments;
		l_subpassDescription.colorAttachmentCount = colorAttachmentCount;
		l_subpassDescription.pColorAttachments = pColorAttachments;
		l_subpassDescription.pResolveAttachments = pResolveAttachments;
		l_subpassDescription.pDepthStencilAttachment = pDepthStencilAttachment;
		l_subpassDescription.preserveAttachmentCount = preserveAttachmentCount;
		l_subpassDescription.pPreserveAttachments = pPreserveAttachments;

		return l_subpassDescription;
	}

	inline VkSubpassDependency subpassDependency(
		uint32_t srcSubpass, uint32_t dstSubpass,
		VkPipelineStageFlags srcStageMask, VkAccessFlags srcAccessMask,
		VkPipelineStageFlags dstStageMask, VkAccessFlags dstAccessMask, 
		VkDependencyFlags dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT)
	{
		// Subpasses in a render pass automatically take care of image layout transitions.
		// These transitions are controlled by subpass dependencies, which specify memory and execution dependencies between subpasses.
		VkSubpassDependency  l_subpassDependency = {};

		// Specify the indices of the dependency and the dependent subpass
		// VK_SUBPASS_EXTERNAL refers to the implicit subpass before or after the render pass depending on whether 
		// it is specified in srcSubpass or dstSubpass.
		// dstSubpass must always be higher than srcSubpass to prevent cycles in the dependency graph.
		l_subpassDependency.srcSubpass = srcSubpass;
		l_subpassDependency.dstSubpass = dstSubpass;

		// Specify the operations to wait on and the stages in which these operations occur.
		l_subpassDependency.srcStageMask = srcStageMask;
		l_subpassDependency.srcAccessMask = srcAccessMask;

		// Specify the operations that wait for the subpass to execute
		// These settings will prevent the transition from happening until it's actually necessary (and allowed)
		l_subpassDependency.dstStageMask = dstStageMask;
		l_subpassDependency.dstAccessMask = dstAccessMask;

		// Bitmask specifying how execution and memory dependencies are formed
		l_subpassDependency.dependencyFlags = dependencyFlags;

		return l_subpassDependency;
	}

	inline void createRenderPass(VkDevice& logicalDevice, VkRenderPass& renderPass,
		uint32_t attachmentCount, const VkAttachmentDescription* pAttachments,
		uint32_t subpassCount, const VkSubpassDescription* pSubpasses,
		uint32_t dependencyCount, const VkSubpassDependency* pDependencies)
	{
		VkRenderPassCreateInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = attachmentCount;
		renderPassInfo.pAttachments = pAttachments;
		renderPassInfo.subpassCount = subpassCount;
		renderPassInfo.pSubpasses = pSubpasses;
		renderPassInfo.dependencyCount = dependencyCount;
		renderPassInfo.pDependencies = pDependencies;

		if (vkCreateRenderPass(logicalDevice, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create render pass!");
		}
	}

	inline VkAttachmentDescription attachmentDescription(
		VkFormat format, VkSampleCountFlagBits samples,
		VkAttachmentLoadOp loadOp, VkAttachmentStoreOp storeOp,
		VkAttachmentLoadOp stencilLoadOp, VkAttachmentStoreOp stencilStoreOp,
		VkImageLayout initialLayout, VkImageLayout finalLayout)
	{
		VkAttachmentDescription l_attachmentDescription{};
		l_attachmentDescription.format = format;
		l_attachmentDescription.samples = samples;
		// The loadOp and storeOp determine what to do with the data in the attachment before and after rendering
		l_attachmentDescription.loadOp = loadOp;
		l_attachmentDescription.storeOp = storeOp;
		// The loadOp and storeOp apply to color and depth data, and stencilLoadOp / stencilStoreOp apply to stencil data. 
		l_attachmentDescription.stencilLoadOp = stencilLoadOp;
		l_attachmentDescription.stencilStoreOp = stencilStoreOp;
		// VkImage objects can be optimized by storing them in certain memory layouts depending on their usecase.
		l_attachmentDescription.initialLayout = initialLayout;
		l_attachmentDescription.finalLayout = finalLayout;

		return l_attachmentDescription;
	}

	inline VkAttachmentReference attachmentReference(uint32_t attachment, VkImageLayout layout)
	{
		VkAttachmentReference l_attachmentRef = {};
		// The attachment parameter specifies which attachment to reference by its index in the attachment descriptions array.
		l_attachmentRef.attachment = attachment;
		// The layout specifies which layout we would like the attachment to have during a subpass that uses this reference.
		l_attachmentRef.layout = layout;

		return l_attachmentRef;
	}

	// If the colorFormat or the depthFormat is VK_FORMAT_UNDEFINED then that particular attachment is excluded
	inline void setupAttachmentsAndSubpassDescription(
		const VkFormat colorFormat, const VkFormat depthFormat, 
		const VkImageLayout initialLayout, const VkImageLayout finalLayout,
		VkSubpassDescription &subpassDescriptions,
		std::vector<VkAttachmentDescription> &attachments,
		std::vector<VkAttachmentReference> &attachmentReferences)
	{
		const bool addColorAttachment = (colorFormat != VK_FORMAT_UNDEFINED) ? true : false;
		const bool addDepthAttachment = (depthFormat != VK_FORMAT_UNDEFINED) ? true : false;

		// Create Empty Subpass Description, then fill it based on which attachments need to be added
		subpassDescriptions =
			RenderPassUtil::subpassDescription(VK_PIPELINE_BIND_POINT_GRAPHICS,
				0, nullptr, //input attachments
				0, nullptr, //color attachments
				nullptr, //resolve attachments -- only comes into play when you have multiple samples (think MSAA)
				nullptr, //depth and stencil
				0, nullptr); //preserve attachments

		// Color
		if (addColorAttachment)
		{
			// The initialLayout and finalLayout passed in define thelayout the image will be in before and after the renderpass.
			// The layout used by the VkAttachmentReference, i.e. VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 
			// is the layout we want the image to be in during the subpass.

			// Color Attachment 
			attachments.push_back(
				RenderPassUtil::attachmentDescription(colorFormat, VK_SAMPLE_COUNT_1_BIT,
					VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE,			  //color data
					VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE,    //stencil data
					initialLayout, finalLayout)  //initial and final layout
			);
			// Color Attachment Reference
			int attachmentReferenceIndex = static_cast<int>(attachmentReferences.size());
			attachmentReferences.push_back(RenderPassUtil::attachmentReference(0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL));

			// DOUBLE CHECK IF ERROR,
			subpassDescriptions.colorAttachmentCount = 1;
			subpassDescriptions.pColorAttachments = &attachmentReferences[attachmentReferenceIndex];
		}

		// Depth
		if (addDepthAttachment)
		{
			// Depth Attachment 
			attachments.push_back(
				RenderPassUtil::attachmentDescription(depthFormat, VK_SAMPLE_COUNT_1_BIT,
					VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_DONT_CARE,				 //depth data
					VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE,			 //stencil data
					VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) //initial and final layout
			);
			// Depth Attachment Reference
			int attachmentReferenceIndex = static_cast<int>(attachmentReferences.size());
			attachmentReferences.push_back(RenderPassUtil::attachmentReference(1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL));

			subpassDescriptions.pDepthStencilAttachment = &attachmentReferences[attachmentReferenceIndex];
		}
	}

	inline void renderPassCreationHelper(VkDevice& logicalDevice, VkRenderPass& renderPass,
		const VkFormat colorFormat, const VkFormat depthFormat, 
		const VkImageLayout beforeRenderPassExecuted, const VkImageLayout afterRenderPassExecuted,
		std::vector<VkSubpassDependency> &subpassDependencies)
	{
		const uint32_t numSubpassDependencies = static_cast<uint32_t>(subpassDependencies.size());
		const uint32_t numSubpasses = numSubpassDependencies-1;
		uint32_t numAttachments = 0;
		numAttachments += (colorFormat != VK_FORMAT_UNDEFINED) ? 1 : 0;
		numAttachments += (depthFormat != VK_FORMAT_UNDEFINED) ? 1 : 0;

		VkSubpassDescription subpassDescriptions;
		std::vector<VkAttachmentDescription> attachments; attachments.reserve(numAttachments);
		std::vector<VkAttachmentReference> attachmentReferences; attachmentReferences.reserve(numAttachments);

		//attachment references, and a subpass description
		setupAttachmentsAndSubpassDescription(colorFormat, depthFormat, 
			beforeRenderPassExecuted, afterRenderPassExecuted, subpassDescriptions, attachments, attachmentReferences);

		RenderPassUtil::createRenderPass(logicalDevice, renderPass,
			numAttachments, attachments.data(),
			numSubpasses, &subpassDescriptions,
			numSubpassDependencies, subpassDependencies.data());
	}
};

namespace FrameResourcesUtil 
{
	inline void createFrameBuffer(VkDevice& logicalDevice, VkFramebuffer& frameBuffer, VkRenderPass& renderPass, 
		VkExtent2D imageExtent, uint32_t attachmentCount, const VkImageView* pAttachments, uint32_t layers = 1)
	{
		VkFramebufferCreateInfo l_framebufferInfo = {};
		l_framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		l_framebufferInfo.renderPass = renderPass;
		l_framebufferInfo.attachmentCount = attachmentCount;
		l_framebufferInfo.pAttachments = pAttachments;
		l_framebufferInfo.width = imageExtent.width;
		l_framebufferInfo.height = imageExtent.height;
		l_framebufferInfo.layers = layers;

		if (vkCreateFramebuffer(logicalDevice, &l_framebufferInfo, nullptr, &frameBuffer) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create framebuffer!");
		}
	}

	inline void createFrameBufferAttachments(VkDevice& logicalDevice, VkPhysicalDevice& pDevice, VkQueue& graphicsQueue, VkCommandPool& cmdPool,
		const uint32_t numAttachments, std::vector<FrameBufferAttachment>& fbAttachments, const VkFormat& imgFormat,
		const VkImageLayout initialLayout, const VkImageLayout finalLayout, const VkExtent2D& imageExtent,
		const VkImageUsageFlags frameBufferUsage, const uint32_t mipLevels = 1, const uint32_t arrayLayers = 1, const float anisotropy = 16.0f)
	{
		const VkExtent3D extents = { imageExtent.width, imageExtent.height , 1 };

		for (uint32_t i = 0; i < numAttachments; i++)
		{
			fbAttachments[i].format = imgFormat;

			// Create 2D Image
			ImageUtil::createImage(logicalDevice, pDevice, fbAttachments[i].image, fbAttachments[i].memory,
				VK_IMAGE_TYPE_2D, fbAttachments[i].format, extents, frameBufferUsage,
				VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TILING_OPTIMAL, mipLevels, arrayLayers,
				initialLayout, VK_SHARING_MODE_EXCLUSIVE);

			// Create 2D image View
			ImageUtil::createImageView(logicalDevice, fbAttachments[i].image, &fbAttachments[i].view, VK_IMAGE_VIEW_TYPE_2D,
				fbAttachments[i].format, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels, nullptr);

			if (initialLayout != finalLayout)
			{
				ImageUtil::transitionImageLayout_SingleTimeCommand(logicalDevice, graphicsQueue, cmdPool, 
					fbAttachments[i].image, imgFormat, initialLayout, finalLayout, mipLevels);
			}
		}
	}

	inline void createFrameBufferAttachments(VkDevice& logicalDevice, VkPhysicalDevice& pDevice, VkQueue& graphicsQueue, VkCommandPool& cmdPool,
		const uint32_t numAttachments, std::vector<FrameBufferAttachment>& fbAttachments, VkSampler& sampler, const VkFormat& imgFormat, 
		const VkImageLayout initialLayout, const VkImageLayout finalLayout, const VkExtent2D& imageExtent,
		const VkImageUsageFlags frameBufferUsage, const VkSamplerAddressMode samplerAddressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		const uint32_t mipLevels = 1, const uint32_t arrayLayers = 1, const float anisotropy = 16.0f)
	{
		// Create a set of Images and Image Views
		FrameResourcesUtil::createFrameBufferAttachments(logicalDevice, pDevice, graphicsQueue, cmdPool,
			numAttachments, fbAttachments, imgFormat, initialLayout, finalLayout, imageExtent, frameBufferUsage);

		// Create sampler
		ImageUtil::createImageSampler(logicalDevice, sampler, VK_FILTER_LINEAR, VK_FILTER_LINEAR, samplerAddressMode,
			VK_SAMPLER_MIPMAP_MODE_LINEAR, 0, 0, static_cast<float>(mipLevels), anisotropy, VK_COMPARE_OP_NEVER);
	}

	inline void createDepthAttachment(VkDevice& logicalDevice, VkPhysicalDevice& pDevice,  
		VkQueue& graphicsQueue, VkCommandPool& cmdPool, 
		FrameBufferAttachment& depthAttachment, VkFormat& depthFormat,
		const VkExtent2D imageExtent, const uint32_t mipLevels = 1, const uint32_t arrayLayers = 1)
	{
		const VkExtent3D extents = { imageExtent.width, imageExtent.height , 1 };
		
		ImageUtil::createImage(logicalDevice, pDevice, depthAttachment.image, depthAttachment.memory,
			VK_IMAGE_TYPE_2D, depthFormat, extents,
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
			VK_SAMPLE_COUNT_1_BIT,
			VK_IMAGE_TILING_OPTIMAL,
			mipLevels, arrayLayers, VK_IMAGE_LAYOUT_UNDEFINED, VK_SHARING_MODE_EXCLUSIVE);

		ImageUtil::createImageView(logicalDevice, depthAttachment.image, &depthAttachment.view,
			VK_IMAGE_VIEW_TYPE_2D, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, mipLevels, nullptr);

		ImageUtil::transitionImageLayout_SingleTimeCommand(logicalDevice, graphicsQueue, cmdPool, depthAttachment.image, depthFormat,
			VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, mipLevels);
	}
}