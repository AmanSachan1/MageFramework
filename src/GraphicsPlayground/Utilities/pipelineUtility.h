#pragma once
#include <global.h>

namespace VulkanPipelineStructures
{
	inline VkVertexInputBindingDescription vertexInputBindingDesc(uint32_t binding, uint32_t stride)
	{
		VkVertexInputBindingDescription l_vertexInputBindingDesc{};
		l_vertexInputBindingDesc.binding = binding;
		l_vertexInputBindingDesc.stride = stride;
		l_vertexInputBindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		return l_vertexInputBindingDesc;
	}

	inline VkVertexInputAttributeDescription vertexInputAttributeDesc(uint32_t location, uint32_t binding, VkFormat format, uint32_t offset)
	{
		VkVertexInputAttributeDescription l_vertexInputAttributeDesc{};
		l_vertexInputAttributeDesc.location = location;
		l_vertexInputAttributeDesc.binding = binding;
		l_vertexInputAttributeDesc.format = format;
		l_vertexInputAttributeDesc.offset = offset;
		return l_vertexInputAttributeDesc;
	}

	inline VkPipelineVertexInputStateCreateInfo vertexInputInfo(
		uint32_t vertexBindingDescriptionCount, const VkVertexInputBindingDescription* pVertexBindingDescriptions, 
		uint32_t vertexAttributeDescriptionCount, const VkVertexInputAttributeDescription* pVertexAttributeDescriptions)
	{
		VkPipelineVertexInputStateCreateInfo l_vertexInputInfo{};
		l_vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		l_vertexInputInfo.vertexBindingDescriptionCount = vertexBindingDescriptionCount;
		l_vertexInputInfo.pVertexBindingDescriptions = pVertexBindingDescriptions;
		l_vertexInputInfo.vertexAttributeDescriptionCount = vertexAttributeDescriptionCount;
		l_vertexInputInfo.pVertexAttributeDescriptions = pVertexAttributeDescriptions;
		return l_vertexInputInfo;
	}

	inline VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreationInfo(VkPrimitiveTopology topology, VkBool32 primitiveRestartEnable)
	{
		VkPipelineInputAssemblyStateCreateInfo l_inputAssemblyStateCreationInfo{};
		l_inputAssemblyStateCreationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		l_inputAssemblyStateCreationInfo.topology = topology;
		l_inputAssemblyStateCreationInfo.primitiveRestartEnable = primitiveRestartEnable;
		return l_inputAssemblyStateCreationInfo;
	}

	inline VkPipelineViewportStateCreateInfo viewportStateCreationInfo(
		uint32_t viewportCount,	const VkViewport* pViewports,
		uint32_t scissorCount, const VkRect2D* pScissors)
	{
		VkPipelineViewportStateCreateInfo l_viewportStateCreationInfo{};
		l_viewportStateCreationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		l_viewportStateCreationInfo.viewportCount = viewportCount;
		l_viewportStateCreationInfo.pViewports = pViewports;
		l_viewportStateCreationInfo.scissorCount = scissorCount;
		l_viewportStateCreationInfo.pScissors = pScissors;
		return l_viewportStateCreationInfo;
	}

	inline VkPipelineRasterizationStateCreateInfo rasterizerCreationInfo(
		VkBool32 depthClampEnable, VkBool32 rasterizerDiscardEnable,
		VkPolygonMode polygonMode, float lineWidth, VkCullModeFlags cullMode, VkFrontFace frontFace,
		VkBool32 depthBiasEnable, float depthBiasConstantFactor, float depthBiasClamp, float depthBiasSlopeFactor)
	{
		VkPipelineRasterizationStateCreateInfo l_rasterizerCreationInfo{};
		l_rasterizerCreationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		l_rasterizerCreationInfo.depthClampEnable = depthClampEnable;
		l_rasterizerCreationInfo.rasterizerDiscardEnable = rasterizerDiscardEnable;
		l_rasterizerCreationInfo.polygonMode = polygonMode;
		l_rasterizerCreationInfo.lineWidth = lineWidth;
		l_rasterizerCreationInfo.cullMode = cullMode;
		l_rasterizerCreationInfo.frontFace = frontFace;
		l_rasterizerCreationInfo.depthBiasEnable = depthBiasEnable;
		l_rasterizerCreationInfo.depthBiasConstantFactor = depthBiasConstantFactor;
		l_rasterizerCreationInfo.depthBiasClamp = depthBiasClamp;
		l_rasterizerCreationInfo.depthBiasSlopeFactor = depthBiasSlopeFactor;
		return l_rasterizerCreationInfo;
	}

	inline VkPipelineMultisampleStateCreateInfo multiSampleStateCreationInfo(
		VkSampleCountFlagBits rasterizationSamples,	
		VkBool32 sampleShadingEnable, float minSampleShading, const VkSampleMask* pSampleMask,
		VkBool32 alphaToCoverageEnable,	VkBool32 alphaToOneEnable)
	{
		// Multisampling is one of the ways to perform anti-aliasing. 
		// It works by combining the fragment shader results of multiple polygons that rasterize to the same pixel. This mainly occurs 
		// along edges, which is also where the most noticeable aliasing artifacts occur. Because it doesn't need to run the fragment 
		// shader multiple times if only one polygon maps to a pixel, it is significantly less expensive than simply rendering to a 
		// higher resolution and then downscaling. Enabling it requires enabling the respective GPU feature.

		VkPipelineMultisampleStateCreateInfo l_multiSampleStateCtreationInfo{};
		l_multiSampleStateCtreationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		l_multiSampleStateCtreationInfo.sampleShadingEnable = sampleShadingEnable;
		l_multiSampleStateCtreationInfo.rasterizationSamples = rasterizationSamples;
		l_multiSampleStateCtreationInfo.minSampleShading = minSampleShading;
		l_multiSampleStateCtreationInfo.pSampleMask = pSampleMask;
		l_multiSampleStateCtreationInfo.alphaToCoverageEnable = alphaToCoverageEnable;
		l_multiSampleStateCtreationInfo.alphaToOneEnable = alphaToOneEnable;
		return l_multiSampleStateCtreationInfo;
	}

	inline VkPipelineDepthStencilStateCreateInfo depthStencilStateCreationInfo(
		VkBool32 depthTestEnable, VkBool32 depthWriteEnable, VkCompareOp depthCompareOp, 
		VkBool32 depthBoundsTestEnable,	float minDepthBounds, float maxDepthBounds,
		VkBool32 stencilTestEnable, VkStencilOpState front, VkStencilOpState back)
	{
		VkPipelineDepthStencilStateCreateInfo l_depthStencilStateCreationInfo{};

		l_depthStencilStateCreationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		l_depthStencilStateCreationInfo.depthTestEnable = depthTestEnable;
		l_depthStencilStateCreationInfo.depthWriteEnable = depthWriteEnable;
		l_depthStencilStateCreationInfo.depthCompareOp = depthCompareOp;

		l_depthStencilStateCreationInfo.depthBoundsTestEnable = depthBoundsTestEnable;
		l_depthStencilStateCreationInfo.minDepthBounds = minDepthBounds;
		l_depthStencilStateCreationInfo.maxDepthBounds = maxDepthBounds;
		
		l_depthStencilStateCreationInfo.stencilTestEnable = stencilTestEnable;
		l_depthStencilStateCreationInfo.front = front;
		l_depthStencilStateCreationInfo.back = back;

		return l_depthStencilStateCreationInfo;
	}

	inline VkPipelineColorBlendAttachmentState colorBlendAttachmentStateInfo(
		VkBool32 blendEnable, VkBlendOp colorBlendOp, VkBlendOp alphaBlendOp, VkColorComponentFlags colorWriteMask,
		VkBlendFactor srcColorBlendFactor, VkBlendFactor dstColorBlendFactor,
		VkBlendFactor srcAlphaBlendFactor, VkBlendFactor dstAlphaBlendFactor )
	{
		// Configuration per attached framebuffer (To set this globally use VkPipelineColorBlendStateCreateInfo)
		VkPipelineColorBlendAttachmentState l_colorBlendAttachmentStateInfo{};
		l_colorBlendAttachmentStateInfo.blendEnable = blendEnable;
		l_colorBlendAttachmentStateInfo.colorBlendOp = colorBlendOp;
		l_colorBlendAttachmentStateInfo.alphaBlendOp = alphaBlendOp;
		l_colorBlendAttachmentStateInfo.colorWriteMask = colorWriteMask;
		l_colorBlendAttachmentStateInfo.srcColorBlendFactor = srcColorBlendFactor;
		l_colorBlendAttachmentStateInfo.dstColorBlendFactor = dstColorBlendFactor;
		l_colorBlendAttachmentStateInfo.srcAlphaBlendFactor = srcAlphaBlendFactor;
		l_colorBlendAttachmentStateInfo.dstAlphaBlendFactor = dstAlphaBlendFactor;
		return l_colorBlendAttachmentStateInfo;
	}

	inline VkPipelineColorBlendStateCreateInfo colorBlendStateCreationInfo(
		VkBool32 logicOpEnable,	VkLogicOp logicOp, 
		uint32_t attachmentCount, const VkPipelineColorBlendAttachmentState* pAttachments,
		float blendConstant0, float blendConstant1, float blendConstant2, float blendConstant3)
	{
		// Configuration for entire pipeline (use VkPipelineColorBlendAttachmentState to configure per-framebuffer)
		VkPipelineColorBlendStateCreateInfo l_colorBlendStateCreationInfo{};
		l_colorBlendStateCreationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		l_colorBlendStateCreationInfo.logicOpEnable = logicOpEnable;
		l_colorBlendStateCreationInfo.logicOp = logicOp;
		l_colorBlendStateCreationInfo.attachmentCount = attachmentCount;
		l_colorBlendStateCreationInfo.pAttachments = pAttachments;
		l_colorBlendStateCreationInfo.blendConstants[0] = blendConstant0;
		l_colorBlendStateCreationInfo.blendConstants[1] = blendConstant1;
		l_colorBlendStateCreationInfo.blendConstants[2] = blendConstant2;
		l_colorBlendStateCreationInfo.blendConstants[3] = blendConstant3;
		return l_colorBlendStateCreationInfo;
	}

	inline VkPipelineDynamicStateCreateInfo dynamicStateCreationInfo(uint32_t dynamicStateCount, const VkDynamicState* pDynamicStates)
	{
		VkPipelineDynamicStateCreateInfo l_dynamicStateCreationInfo{};
		l_dynamicStateCreationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		l_dynamicStateCreationInfo.dynamicStateCount = dynamicStateCount;
		l_dynamicStateCreationInfo.pDynamicStates = pDynamicStates;
		return l_dynamicStateCreationInfo;
	}

	inline VkGraphicsPipelineCreateInfo graphicsPipelineCreationInfo(
		uint32_t stageCount, const VkPipelineShaderStageCreateInfo* pStages,
		const VkPipelineVertexInputStateCreateInfo* pVertexInputState,
		const VkPipelineInputAssemblyStateCreateInfo* pInputAssemblyState,
		const VkPipelineTessellationStateCreateInfo* pTessellationState,
		const VkPipelineViewportStateCreateInfo* pViewportState,
		const VkPipelineRasterizationStateCreateInfo* pRasterizationState,
		const VkPipelineMultisampleStateCreateInfo* pMultisampleState,
		const VkPipelineDepthStencilStateCreateInfo* pDepthStencilState,
		const VkPipelineColorBlendStateCreateInfo* pColorBlendState,
		const VkPipelineDynamicStateCreateInfo* pDynamicState,
		VkPipelineLayout layout,
		VkRenderPass renderPass, uint32_t subpass,
		VkPipeline basePipelineHandle, int32_t basePipelineIndex)
	{
		VkGraphicsPipelineCreateInfo l_graphicsPipelineCreationInfo{};		
		l_graphicsPipelineCreationInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		// Shader stages refer to the vertex, fragment, tesselation, geometry shaders (maybe more with RTX stuff) 
		// associated with the pipeline.
		// stage count refers to all the stages being used in the pipeline
		l_graphicsPipelineCreationInfo.stageCount = stageCount;
		l_graphicsPipelineCreationInfo.pStages = pStages;
		l_graphicsPipelineCreationInfo.pVertexInputState = pVertexInputState;
		l_graphicsPipelineCreationInfo.pInputAssemblyState = pInputAssemblyState;
		l_graphicsPipelineCreationInfo.pTessellationState = pTessellationState;
		l_graphicsPipelineCreationInfo.pViewportState = pViewportState;
		l_graphicsPipelineCreationInfo.pRasterizationState = pRasterizationState;
		l_graphicsPipelineCreationInfo.pMultisampleState = pMultisampleState;
		l_graphicsPipelineCreationInfo.pDepthStencilState = pDepthStencilState;
		l_graphicsPipelineCreationInfo.pColorBlendState = pColorBlendState;
		l_graphicsPipelineCreationInfo.pDynamicState = pDynamicState;
		l_graphicsPipelineCreationInfo.layout = layout;
		l_graphicsPipelineCreationInfo.renderPass = renderPass;
		l_graphicsPipelineCreationInfo.subpass = subpass;
		// Vulkan allows you to create a new graphics pipeline by deriving from an existing pipeline.
		// Specify the basePipelineHandle and basePipelineIndex to derive a pipeline from an existing one.
		l_graphicsPipelineCreationInfo.basePipelineHandle = basePipelineHandle;
		l_graphicsPipelineCreationInfo.basePipelineIndex = basePipelineIndex;
		return l_graphicsPipelineCreationInfo;
	}
};

namespace VulkanPipelineCreation
{
	inline VkPipelineLayout createPipelineLayout(
		VkDevice& logicalDevice, std::vector<VkDescriptorSetLayout> descriptorSetLayouts,
		uint32_t pushConstantRangeCount, const VkPushConstantRange* pPushConstantRanges)
	{
		VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
		pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
		pipelineLayoutInfo.pushConstantRangeCount = pushConstantRangeCount;
		pipelineLayoutInfo.pPushConstantRanges = pPushConstantRanges;

		VkPipelineLayout pipelineLayout;
		if (vkCreatePipelineLayout(logicalDevice, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create pipeline layout");
		}

		return pipelineLayout;
	}
	
	inline void createPipelineCache(VkDevice& logicalDevice, VkPipelineCache &pipelineCache)
	{
		VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
		pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
		if (vkCreatePipelineCache(logicalDevice, &pipelineCacheCreateInfo, nullptr, &pipelineCache) != VK_SUCCESS) 
		{
			throw std::runtime_error("Failed to create pipeline cache");
		}
	}

	inline bool createGraphicsPipelines(
		VkDevice& logicalDevice, VkPipelineCache pipelineCache, 
		uint32_t createInfoCount, const VkGraphicsPipelineCreateInfo* pCreateInfos, VkPipeline* pPipelines)
	{
		if (vkCreateGraphicsPipelines(logicalDevice, pipelineCache, createInfoCount, pCreateInfos, nullptr, pPipelines) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create pipeline(s)");
			return false;
		}
		return true;
	}
};
