#pragma once
#include <global.h>

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
		VkPipelineStageFlags dstStageMask, VkAccessFlags dstAccessMask)
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
};