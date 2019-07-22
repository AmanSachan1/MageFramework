#pragma once
#include <global.h>
#include <Utilities/imageUtility.h>

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

	inline void createFrameBuffer(VkDevice& logicalDevice, VkFramebuffer& frameBuffer, VkExtent2D imageExtent,
		VkRenderPass& renderPass, uint32_t attachmentCount, const VkImageView* pAttachments, uint32_t layers = 1)
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

namespace DescriptorUtil
{
	inline VkDescriptorSetLayoutBinding createDescriptorSetLayoutBinding(uint32_t binding,
		VkDescriptorType descriptorType, uint32_t descriptorCount,
		VkShaderStageFlags stageFlags, const VkSampler* pImmutableSampler)
	{
		VkDescriptorSetLayoutBinding l_UBOLayoutBinding = {};
		// Binding --> used in shader
		// Descriptor Type --> type of descriptor
		// Descriptor Count --> Shader variable can represent an array of UBO's, descriptorCount specifies number of values in the array
		// Stage Flags --> which shader you're referencing this descriptor from 
		// pImmutableSamplers --> for image sampling related descriptors
		l_UBOLayoutBinding.binding = binding;
		l_UBOLayoutBinding.descriptorType = descriptorType;
		l_UBOLayoutBinding.descriptorCount = descriptorCount;
		l_UBOLayoutBinding.stageFlags = stageFlags;
		l_UBOLayoutBinding.pImmutableSamplers = pImmutableSampler;

		return l_UBOLayoutBinding;
	}

	inline void createDescriptorSetLayout(VkDevice& logicalDevice, VkDescriptorSetLayout& descriptorSetLayout,
		uint32_t bindingCount, VkDescriptorSetLayoutBinding* data)
	{
		VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {};
		descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		descriptorSetLayoutCreateInfo.pNext = nullptr;
		descriptorSetLayoutCreateInfo.bindingCount = bindingCount;
		descriptorSetLayoutCreateInfo.pBindings = data;

		if (vkCreateDescriptorSetLayout(logicalDevice, &descriptorSetLayoutCreateInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create descriptor set layout!");
		}
	}

	inline VkDescriptorPoolSize descriptorPoolSize(VkDescriptorType type, uint32_t descriptorCount)
	{
		VkDescriptorPoolSize  l_poolSize = {};
		l_poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		l_poolSize.descriptorCount = descriptorCount;

		return l_poolSize;
	}

	inline void createDescriptorPool(VkDevice& logicalDevice, uint32_t maxSets, uint32_t poolSizeCount, VkDescriptorPoolSize* data, VkDescriptorPool& descriptorPool)
	{
		VkDescriptorPoolCreateInfo descriptorPoolInfo = {};
		descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		descriptorPoolInfo.pNext = nullptr;
		descriptorPoolInfo.flags = 0; // Change if you're going to modify the descriptor set after its creation
		descriptorPoolInfo.poolSizeCount = poolSizeCount;
		descriptorPoolInfo.pPoolSizes = data;
		descriptorPoolInfo.maxSets = maxSets; // max number of descriptor sets allowed

		if (vkCreateDescriptorPool(logicalDevice, &descriptorPoolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create descriptor pool");
		}
	}

	inline void createDescriptorSets(VkDevice& logicalDevice, VkDescriptorPool descriptorPool, uint32_t descriptorSetCount,
		VkDescriptorSetLayout* descriptorSetLayouts, VkDescriptorSet* descriptorSetData)
	{
		VkDescriptorSetAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = descriptorPool;
		allocInfo.descriptorSetCount = descriptorSetCount;
		allocInfo.pSetLayouts = descriptorSetLayouts;

		if (vkAllocateDescriptorSets(logicalDevice, &allocInfo, descriptorSetData) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to allocate descriptor sets!");
		}
	}

	inline VkDescriptorBufferInfo createDescriptorBufferInfo(VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range)
	{
		VkDescriptorBufferInfo l_descriptorBufferInfo = {};
		l_descriptorBufferInfo.buffer = buffer;
		l_descriptorBufferInfo.offset = offset;
		l_descriptorBufferInfo.range = range;
		return l_descriptorBufferInfo;
	}

	inline VkDescriptorImageInfo createDescriptorImageInfo(VkSampler& sampler, VkImageView& imageView, VkImageLayout imageLayout)
	{
		VkDescriptorImageInfo l_descriptorImageInfo = {};
		l_descriptorImageInfo.sampler = sampler;
		l_descriptorImageInfo.imageView = imageView;
		l_descriptorImageInfo.imageLayout = imageLayout;
		return l_descriptorImageInfo;
	}

	inline VkWriteDescriptorSet writeDescriptorSet(
		VkDescriptorSet dstSet, uint32_t dstBinding, uint32_t descriptorCount, VkDescriptorType descriptorType,
		const VkDescriptorBufferInfo* pBufferInfo, uint32_t dstArrayElement = 0)
	{
		VkWriteDescriptorSet l_descriptorWrite = {};
		l_descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		l_descriptorWrite.dstSet = dstSet;
		l_descriptorWrite.dstBinding = dstBinding;
		l_descriptorWrite.dstArrayElement = dstArrayElement;
		l_descriptorWrite.descriptorCount = descriptorCount;
		l_descriptorWrite.descriptorType = descriptorType;
		l_descriptorWrite.pImageInfo = nullptr;
		l_descriptorWrite.pBufferInfo = pBufferInfo;
		l_descriptorWrite.pTexelBufferView = nullptr;
		return l_descriptorWrite;
	}

	inline VkWriteDescriptorSet writeDescriptorSet(
		VkDescriptorSet dstSet, uint32_t dstBinding, uint32_t descriptorCount, VkDescriptorType descriptorType,
		const VkDescriptorImageInfo* pImageInfo = nullptr, uint32_t dstArrayElement = 0)
	{
		VkWriteDescriptorSet l_descriptorWrite = {};
		l_descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		l_descriptorWrite.dstSet = dstSet;
		l_descriptorWrite.dstBinding = dstBinding;
		l_descriptorWrite.dstArrayElement = dstArrayElement;
		l_descriptorWrite.descriptorCount = descriptorCount;
		l_descriptorWrite.descriptorType = descriptorType;
		l_descriptorWrite.pImageInfo = pImageInfo;
		return l_descriptorWrite;
	}
}