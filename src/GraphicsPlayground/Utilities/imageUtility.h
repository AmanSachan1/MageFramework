#pragma once
#include <global.h>

namespace VulkanImageStructures
{
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
};