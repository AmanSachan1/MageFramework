#pragma once
#include <global.h>
#include <Utilities/generalUtility.h>

#pragma warning( disable : 26812 ) // C26812: Prefer 'enum class' over 'enum' (Enum.3); Because of Vulkan 

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
		l_vertexInputInfo.flags = 0;
		return l_vertexInputInfo;
	}

	inline VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreationInfo(VkPrimitiveTopology topology, VkBool32 primitiveRestartEnable)
	{
		VkPipelineInputAssemblyStateCreateInfo l_inputAssemblyStateCreationInfo{};
		l_inputAssemblyStateCreationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		l_inputAssemblyStateCreationInfo.topology = topology;
		l_inputAssemblyStateCreationInfo.primitiveRestartEnable = primitiveRestartEnable;
		l_inputAssemblyStateCreationInfo.flags = 0;
		return l_inputAssemblyStateCreationInfo;
	}

	inline VkPipelineViewportStateCreateInfo viewportStateCreationInfo(
		uint32_t viewportCount, const VkViewport* pViewports,
		uint32_t scissorCount, const VkRect2D* pScissors)
	{
		VkPipelineViewportStateCreateInfo l_viewportStateCreationInfo{};
		l_viewportStateCreationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		l_viewportStateCreationInfo.viewportCount = viewportCount;
		l_viewportStateCreationInfo.pViewports = pViewports;
		l_viewportStateCreationInfo.scissorCount = scissorCount;
		l_viewportStateCreationInfo.pScissors = pScissors;
		l_viewportStateCreationInfo.flags = 0;
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
		l_rasterizerCreationInfo.flags = 0;
		return l_rasterizerCreationInfo;
	}

	inline VkPipelineMultisampleStateCreateInfo multiSampleStateCreationInfo(
		VkSampleCountFlagBits rasterizationSamples,
		VkBool32 sampleShadingEnable, float minSampleShading, const VkSampleMask* pSampleMask,
		VkBool32 alphaToCoverageEnable, VkBool32 alphaToOneEnable)
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
		l_multiSampleStateCtreationInfo.flags = 0;
		return l_multiSampleStateCtreationInfo;
	}

	inline VkPipelineDepthStencilStateCreateInfo depthStencilStateCreationInfo(
		VkBool32 depthTestEnable, VkBool32 depthWriteEnable, VkCompareOp depthCompareOp,
		VkBool32 depthBoundsTestEnable, float minDepthBounds, float maxDepthBounds,
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

		l_depthStencilStateCreationInfo.flags = 0;

		return l_depthStencilStateCreationInfo;
	}

	inline VkPipelineColorBlendAttachmentState colorBlendAttachmentStateInfo(
		VkBool32 blendEnable, VkBlendOp colorBlendOp, VkBlendOp alphaBlendOp, VkColorComponentFlags colorWriteMask,
		VkBlendFactor srcColorBlendFactor, VkBlendFactor dstColorBlendFactor,
		VkBlendFactor srcAlphaBlendFactor, VkBlendFactor dstAlphaBlendFactor)
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
		VkBool32 logicOpEnable, VkLogicOp logicOp,
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
		l_colorBlendStateCreationInfo.flags = 0;
		return l_colorBlendStateCreationInfo;
	}

	inline VkPipelineDynamicStateCreateInfo dynamicStateCreationInfo(uint32_t dynamicStateCount, const VkDynamicState* pDynamicStates)
	{
		VkPipelineDynamicStateCreateInfo l_dynamicStateCreationInfo{};
		l_dynamicStateCreationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		l_dynamicStateCreationInfo.dynamicStateCount = dynamicStateCount;
		l_dynamicStateCreationInfo.pDynamicStates = pDynamicStates;
		l_dynamicStateCreationInfo.flags = 0;
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

		l_graphicsPipelineCreationInfo.flags = 0;
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
		pipelineLayoutInfo.flags = 0;

		VkPipelineLayout pipelineLayout;
		if (vkCreatePipelineLayout(logicalDevice, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create pipeline layout");
		}

		return pipelineLayout;
	}

	inline VkPipelineLayout createPipelineLayout(
		VkDevice& logicalDevice, VkDescriptorSetLayout* descriptorSetLayouts,
		uint32_t pushConstantRangeCount, const VkPushConstantRange* pPushConstantRanges)
	{
		VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts;
		pipelineLayoutInfo.pushConstantRangeCount = pushConstantRangeCount;
		pipelineLayoutInfo.pPushConstantRanges = pPushConstantRanges;
		pipelineLayoutInfo.flags = 0;

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

	inline bool createComputePipelines(VkDevice& logicalDevice, VkPipelineCache pipelineCache,
		const uint32_t createInfoCount, const VkComputePipelineCreateInfo* pCreateInfos, VkPipeline* pPipelines)
	{
		if (vkCreateComputePipelines(logicalDevice, pipelineCache, createInfoCount, pCreateInfos, nullptr, pPipelines) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create pipeline");
			return false;
		}
		return true;
	}

	inline bool createComputePipeline(VkDevice& logicalDevice, VkPipelineShaderStageCreateInfo compShaderStageInfo,
		VkPipeline& computePipeline, VkPipelineLayout computePipelineLayout)
	{
		VkComputePipelineCreateInfo pipelineInfo = {};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		pipelineInfo.stage = compShaderStageInfo;
		pipelineInfo.layout = computePipelineLayout;
		
		return VulkanPipelineCreation::createComputePipelines(logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, &computePipeline);
	}

	inline bool createGraphicsPipelines(
		VkDevice& logicalDevice, VkPipelineCache pipelineCache,
		const uint32_t createInfoCount, const VkGraphicsPipelineCreateInfo* pCreateInfos, VkPipeline* pPipelines)
	{
		if (vkCreateGraphicsPipelines(logicalDevice, pipelineCache, createInfoCount, pCreateInfos, nullptr, pPipelines) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create pipeline(s)");
			return false;
		}
		return true;
	}

	inline bool createGraphicsPipeline(
		VkDevice& logicalDevice, 
		VkPipeline& pipeline, VkPipelineLayout& pipelineLayout, 
		VkRenderPass& renderPass, const uint32_t subpass,
		const uint32_t stageCount, const VkPipelineShaderStageCreateInfo* stages,
		VkPipelineVertexInputStateCreateInfo& vertexInput, 
		const VkExtent2D swapChainExtents)
	{
		// Reference: https://vulkan-tutorial.com/Drawing_a_triangle/Graphics_pipeline_basics/Fixed_functions

		// -------- Input assembly --------
		// The VkPipelineInputAssemblyStateCreateInfo struct describes two things: what kind of geometry will be drawn 
		// from the vertices and if primitive restart should be enabled.

		VkPipelineInputAssemblyStateCreateInfo inputAssembly =
			VulkanPipelineStructures::inputAssemblyStateCreationInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FALSE);

		// -------- Tesselation State ---------
		// Set up Tesselation state here

		// -------- Viewport State ---------
		// Viewports and Scissors (rectangles that define in which regions pixels are stored)
		const float viewportWidth = static_cast<float>(swapChainExtents.width);
		const float viewportHeight = static_cast<float>(swapChainExtents.height);
		VkViewport viewport = Util::createViewport(viewportWidth, viewportHeight);

		// While viewports define the transformation from the image to the framebuffer, 
		// scissor rectangles define in which regions pixels will actually be stored.
		// we simply want to draw to the entire framebuffer, so we'll specify a scissor rectangle that covers the framebuffer entirely
		VkRect2D scissor = Util::createRectangle(swapChainExtents);

		// Now this viewport and scissor rectangle need to be combined into a viewport state. It is possible to use 
		// multiple viewports and scissor rectangles. Using multiple requires enabling a GPU feature.
		VkPipelineViewportStateCreateInfo viewportState =
			VulkanPipelineStructures::viewportStateCreationInfo(1, &viewport, 1, &scissor);

		// -------- Rasterize --------
		// -- The rasterizer takes the geometry that is shaped by the vertices from the vertex shader and turns
		// it into fragments to be colored by the fragment shader.
		// -- It also performs depth testing, face culling and the scissor test, and it can be configured to output 
		// fragments that fill entire polygons or just the edges (wireframe rendering).
		// -- If rasterizerDiscardEnable is set to VK_TRUE, then geometry never passes through the rasterizer stage. 
		// This basically disables any output to the framebuffer.
		// -- depthBiasEnable: The rasterizer can alter the depth values by adding a constant value or biasing 
		// them based on a fragment's slope. This is sometimes used for shadow mapping.
		VkPipelineRasterizationStateCreateInfo rasterizer =
			VulkanPipelineStructures::rasterizerCreationInfo(VK_FALSE, VK_FALSE, VK_POLYGON_MODE_FILL, 1.0f,
				VK_CULL_MODE_NONE, 
				//VK_CULL_MODE_BACK_BIT,
				VK_FRONT_FACE_CLOCKWISE,
				VK_FALSE, 0.0f, 0.0f, 0.0f);

		// -------- Multisampling --------
		// (turned off here)
		VkPipelineMultisampleStateCreateInfo multisampling =
			VulkanPipelineStructures::multiSampleStateCreationInfo(VK_SAMPLE_COUNT_1_BIT, VK_FALSE, 1.0f, nullptr, VK_FALSE, VK_FALSE);

		// -------- Depth and Stencil Testing --------
		VkPipelineDepthStencilStateCreateInfo depthAndStencil =
			VulkanPipelineStructures::depthStencilStateCreationInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS, VK_FALSE, 0.0f, 1.0f, VK_FALSE, {}, {});

		// -------- Color Blending ---------
		VkPipelineColorBlendAttachmentState colorBlendAttachment =
			VulkanPipelineStructures::colorBlendAttachmentStateInfo(
				VK_FALSE, VK_BLEND_OP_ADD, VK_BLEND_OP_ADD,
				VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
				VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
				VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO);

		// Global color blending settings 
		// --> set using colorBlendAttachment (add multiple attachments for multiple framebuffers with different blend settings)
		VkPipelineColorBlendStateCreateInfo colorBlending =
			VulkanPipelineStructures::colorBlendStateCreationInfo(VK_FALSE, VK_LOGIC_OP_COPY, 1, &colorBlendAttachment, 0.0f, 0.0f, 0.0f, 0.0f);

		// -------- Dynamic States ---------
		// Set up dynamic states here

		// ----------------------------------------------------------------------------------------------------
		// -------- Actually create the graphics pipeline below ---------
		// ----------------------------------------------------------------------------------------------------

		// -------- Create Pipeline Info Struct ---------
		VkGraphicsPipelineCreateInfo pipelineInfo =
			VulkanPipelineStructures::graphicsPipelineCreationInfo(
				stageCount, stages, // each pipeline will add it's own shaders
				&vertexInput,
				&inputAssembly,
				nullptr, // tessellation
				&viewportState,
				&rasterizer,
				&multisampling,
				&depthAndStencil,
				&colorBlending,
				nullptr, // dynamicState
				pipelineLayout, // pipeline Layout
				renderPass, subpass, // renderpass and subpass
				VK_NULL_HANDLE, -1); // basePipelineHandle  and basePipelineIndex

		// -------- Create Pipeline ---------
		return VulkanPipelineCreation::createGraphicsPipelines(logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, &pipeline);
	}

	inline bool createPostProcessPipeline(
		VkDevice& logicalDevice,
		VkPipeline& pipeline, VkPipelineLayout& pipelineLayout,
		VkRenderPass& renderPass, const uint32_t subpass,
		const uint32_t stageCount, const VkPipelineShaderStageCreateInfo* stages,
		const VkExtent2D swapChainExtents)
	{
		// Reference: https://vulkan-tutorial.com/Drawing_a_triangle/Graphics_pipeline_basics/Fixed_functions

		// -------- Empty Vertex Input --------
		VkPipelineVertexInputStateCreateInfo vertexInput = VulkanPipelineStructures::vertexInputInfo(0, nullptr, 0, nullptr);

		// -------- Create Pipeline ---------
		return createGraphicsPipeline( logicalDevice,
			pipeline, pipelineLayout,
			renderPass, subpass,
			stageCount, stages, vertexInput, swapChainExtents);
	}
};