#pragma once
#include <vector>
#include <vulkan/vulkan.h>

namespace VulkanInitializers
{
	//--------------------------------------------------------
	//					  Swap Chain
	//--------------------------------------------------------

	inline VkSwapchainCreateInfoKHR basicSwapChainCreateInfo(VkSurfaceKHR vkSurface,
		uint32_t minImageCount, VkFormat imageFormat, VkColorSpaceKHR imageColorSpace,
		VkExtent2D imageExtent, uint32_t imageArrayLayers, VkImageUsageFlags imageUsage,
		VkSurfaceTransformFlagBitsKHR preTransform, VkCompositeAlphaFlagBitsKHR compositeAlpha,
		VkPresentModeKHR presentMode, VkSwapchainKHR oldSwapchain)
	{
		// --- Create logical device ---
		VkSwapchainCreateInfoKHR swapchainCreateInfo = {};
		swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		// specify the surface the swapchain will be tied to
		swapchainCreateInfo.surface = vkSurface;

		swapchainCreateInfo.minImageCount = minImageCount;
		swapchainCreateInfo.imageFormat = imageFormat;
		swapchainCreateInfo.imageColorSpace = imageColorSpace;
		swapchainCreateInfo.imageExtent = imageExtent;
		swapchainCreateInfo.imageArrayLayers = imageArrayLayers;
		swapchainCreateInfo.imageUsage = imageUsage;

		// Specify transform on images in the swap chain
		// VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR == no transformations
		swapchainCreateInfo.preTransform = preTransform;
		// Specify alpha channel usage 
		swapchainCreateInfo.compositeAlpha = compositeAlpha;
		swapchainCreateInfo.presentMode = presentMode;
		// Reference to old swap chain in case current one becomes invalid
		swapchainCreateInfo.oldSwapchain = oldSwapchain;

		return swapchainCreateInfo;
	}

	inline void createSwapChain(VkDevice logicalDevice, VkSwapchainCreateInfoKHR& swapchainCreateInfo, VkSwapchainKHR& vkSwapChain)
	{
		if (vkCreateSwapchainKHR(logicalDevice, &swapchainCreateInfo, nullptr, &vkSwapChain) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create swap chain");
		}
	}

	inline VkImageViewCreateInfo basicImageViewCreateInfo(VkImage& image, VkImageViewType viewType, VkFormat format)
	{
		VkImageViewCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = image;

		createInfo.viewType = viewType;
		createInfo.format = format;

		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

		//No Mipmapping and no multiple targets
		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;

		return createInfo;
	}

	inline void createImageView(VkDevice logicalDevice, VkImageView* imageView, const VkImageViewCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator)
	{
		if (vkCreateImageView(logicalDevice, pCreateInfo, pAllocator, imageView) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create image views!");
		}
	}

	//--------------------------------------------------------
	//			Descriptor Sets and Descriptor Layouts
	// Reference: https://vulkan-tutorial.com/Uniform_buffers
	//--------------------------------------------------------

	inline void CreateDescriptorPool(VkDevice& logicalDevice, uint32_t poolSizeCount, VkDescriptorPoolSize* data, VkDescriptorPool& descriptorPool)
	{
		VkDescriptorPoolCreateInfo descriptorPoolInfo = {};
		descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		descriptorPoolInfo.pNext = nullptr;
		descriptorPoolInfo.poolSizeCount = poolSizeCount;
		descriptorPoolInfo.pPoolSizes = data;
		descriptorPoolInfo.maxSets = poolSizeCount;

		if (vkCreateDescriptorPool(logicalDevice, &descriptorPoolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create descriptor pool");
		}
	}

	inline void CreateDescriptorSetLayout(VkDevice& logicalDevice, uint32_t bindingCount, VkDescriptorSetLayoutBinding* data, VkDescriptorSetLayout& descriptorSetLayout)
	{
		VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {};
		descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		descriptorSetLayoutCreateInfo.pNext = nullptr;
		descriptorSetLayoutCreateInfo.bindingCount = bindingCount;
		descriptorSetLayoutCreateInfo.pBindings = data;

		if (vkCreateDescriptorSetLayout(logicalDevice, &descriptorSetLayoutCreateInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
			throw std::runtime_error("failed to create descriptor set layout!");
		}
	}

	inline VkDescriptorSet CreateDescriptorSet(VkDevice& logicalDevice, VkDescriptorPool descriptorPool, VkDescriptorSetLayout descriptorSetLayout)
	{
		VkDescriptorSetAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = descriptorPool;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &descriptorSetLayout;

		VkDescriptorSet descriptorSet;
		vkAllocateDescriptorSets(logicalDevice, &allocInfo, &descriptorSet);
		return descriptorSet;
	}
};

//--------------------------------------------------------
//			Logical Device, Physical Device, Queues
//--------------------------------------------------------

namespace VulkanDevicesUtil
{
	inline VkDeviceQueueCreateInfo deviceQueueCreateInfo(VkStructureType sType, uint32_t queueFamilyIndex, uint32_t queueCount, float* pQueuePriorities)
	{
		VkDeviceQueueCreateInfo deviceQueueCreateInfo{};
		deviceQueueCreateInfo.sType = sType;
		deviceQueueCreateInfo.queueFamilyIndex = queueFamilyIndex;
		deviceQueueCreateInfo.queueCount = queueCount;
		deviceQueueCreateInfo.pQueuePriorities = pQueuePriorities;

		return deviceQueueCreateInfo;
	}

	inline VkDeviceCreateInfo logicalDeviceCreateInfo(VkStructureType sType, uint32_t queueCreateInfoCount,
		VkDeviceQueueCreateInfo* queueCreateInfos, VkPhysicalDeviceFeatures* deviceFeatures,
		uint32_t deviceExtensionCount, const char** deviceExtensionNames,
		uint32_t validationLayerCount, const char* const* validationLayerNames)
	{
		// --- Create logical device ---
		VkDeviceCreateInfo logicalDeviceCreateInfo = {};
		logicalDeviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		logicalDeviceCreateInfo.queueCreateInfoCount = queueCreateInfoCount;
		logicalDeviceCreateInfo.pQueueCreateInfos = queueCreateInfos;
		logicalDeviceCreateInfo.pEnabledFeatures = deviceFeatures;

		// Enable device-specific extensions and validation layers
		logicalDeviceCreateInfo.enabledExtensionCount = deviceExtensionCount;
		logicalDeviceCreateInfo.ppEnabledExtensionNames = deviceExtensionNames;
		logicalDeviceCreateInfo.enabledLayerCount = validationLayerCount;
		logicalDeviceCreateInfo.ppEnabledLayerNames = validationLayerNames;

		return logicalDeviceCreateInfo;
	}

	inline void createLogicalDevice(VkPhysicalDevice &physicalDevice, VkDevice& logicalDevice, VkDeviceCreateInfo& logicalDeviceCreateInfo)
	{
		if (vkCreateDevice(physicalDevice, &logicalDeviceCreateInfo, nullptr, &logicalDevice) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create logical device");
		}
	}
}

//--------------------------------------------------------
//			Miscellaneous Vulkan Structures
//--------------------------------------------------------

namespace VulkanUtil
{
	inline VkViewport createViewport(float x, float y, float width, float height, float minDepth, float maxDepth)
	{
		return { x, y, width, height, minDepth, maxDepth };
	}

	inline VkRect2D createRectangle(VkOffset2D offset, VkExtent2D extent)
	{
		return { offset, extent };
	}

	inline void submitToGraphicsQueue( VkQueue graphicsQueue,
		uint32_t waitSemaphoreCount, const VkSemaphore* pWaitSemaphores, 
		const VkPipelineStageFlags* pWaitDstStageMask,
		uint32_t commandBufferCount, const VkCommandBuffer* pCommandBuffers,
		uint32_t signalSemaphoreCount, const VkSemaphore* pSignalSemaphores,
		VkFence inFlightFence)
	{
		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		// These parameters specify which semaphores to wait on before execution begins and in which stage(s) of the pipeline to wait
		// Each entry in the waitStages array corresponds to the semaphore with the same index in pWaitSemaphores
		submitInfo.waitSemaphoreCount = waitSemaphoreCount;
		submitInfo.pWaitSemaphores = pWaitSemaphores;
		submitInfo.pWaitDstStageMask = pWaitDstStageMask;

		submitInfo.commandBufferCount = commandBufferCount;
		submitInfo.pCommandBuffers = pCommandBuffers;
		submitInfo.signalSemaphoreCount = signalSemaphoreCount;
		submitInfo.pSignalSemaphores = pSignalSemaphores;

		if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFence) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to submit graphics command buffer to graphics queue!");
		}
	}
};

//--------------------------------------------------------
//			Command Pools and Buffers
//--------------------------------------------------------

namespace VulkanCommandUtil
{
	inline void beginRenderPass(VkCommandBuffer& cmdBuffer, VkRenderPass renderPass, VkFramebuffer framebuffer,	VkRect2D renderArea, const VkClearValue* clearValue, uint32_t clearValueCount)
	{
		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = renderPass;
		renderPassInfo.framebuffer = framebuffer;
		// The render area defines where shader loads and stores will take place. The pixels outside this region will have undefined values. 
		// It should match the size of the attachments for best performance.
		renderPassInfo.renderArea = renderArea;
		renderPassInfo.clearValueCount = clearValueCount;
		renderPassInfo.pClearValues = clearValue;

		// he final parameter of the vkCmdBeginRenderPass command controls how the drawing commands within the render pass will be provided.
		// It can have one of two values:
		// - VK_SUBPASS_CONTENTS_INLINE: The render pass commands will be embedded in the primary command buffer itself 
		//								and no secondary command buffers will be executed.
		// - VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS : The render pass commands will be executed from secondary command buffers.
		vkCmdBeginRenderPass(cmdBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	}

	inline void beginCommandBuffer(VkCommandBuffer& cmdBuffer)
	{
		// Begin recording a command buffer by calling vkBeginCommandBuffer
		
		// The flags parameter specifies how we're going to use the command buffer. The following values are available:
		// - VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT: The command buffer will be rerecorded right after executing it once.
		// - VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT : This is a secondary command buffer that will be entirely within a single render pass.
		// - VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT : The command buffer can be resubmitted while it is also already pending execution.

		// We use VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT because we may already be scheduling the drawing commands for the next frame 
		// while the last frame is not finished yet.
		
		// The pInheritanceInfo parameter is only relevant for secondary command buffers. 
		// It specifies which state to inherit from the calling primary command buffers.
		
		// If the command buffer was already recorded once, then a call to vkBeginCommandBuffer will implicitly reset it. 
		// It's not possible to append commands to a buffer at a later time.

		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
		beginInfo.pInheritanceInfo = nullptr; // Optional

		if (vkBeginCommandBuffer(cmdBuffer, &beginInfo) != VK_SUCCESS) {

			throw std::runtime_error("failed to begin recording command buffer!");
		}
	}

	inline void endCommandBuffer(VkCommandBuffer& cmdBuffer)
	{
		if (vkEndCommandBuffer(cmdBuffer) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to record the compute command buffer");
		}
	}

	inline void createCommandPool(VkDevice& logicalDevice, VkCommandPool& cmdPool, uint32_t queueFamilyIndex)
	{
		// Command pools manage the memory that is used to store the buffers and command buffers are allocated from them.

		VkCommandPoolCreateInfo cmdPoolCreateInfo = {};
		cmdPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		cmdPoolCreateInfo.queueFamilyIndex = queueFamilyIndex;
		cmdPoolCreateInfo.flags = 0; // Optional

		if (vkCreateCommandPool(logicalDevice, &cmdPoolCreateInfo, nullptr, &cmdPool) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create command pool");
		}
	}

	inline void allocateCommandBuffers(VkDevice& logicalDevice, VkCommandPool& cmdPool, std::vector<VkCommandBuffer> &cmdBuffers)
	{
		// Specify the command pool and number of buffers to allocate
		
		// The level parameter specifies if the allocated command buffers are primary or secondary command buffers.
		// VK_COMMAND_BUFFER_LEVEL_PRIMARY: Can be submitted to a queue for execution, but cannot be called from other command buffers.
		// VK_COMMAND_BUFFER_LEVEL_SECONDARY : Cannot be submitted directly, but can be called from primary command buffers.
		
		VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = cmdPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = (uint32_t)cmdBuffers.size();

		if (vkAllocateCommandBuffers(logicalDevice, &allocInfo, cmdBuffers.data()) != VK_SUCCESS) 
		{
			throw std::runtime_error("failed to allocate command buffers!");
		}
	}
}

//--------------------------------------------------------
//			Render Pass Structures and Creation
//--------------------------------------------------------

namespace RenderPassUtility
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

//--------------------------------------------------------
//			PipeLine State Info and Creation
//--------------------------------------------------------

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
		VkBool32 sampleShadingEnable, float minSampleShading,	const VkSampleMask* pSampleMask,
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
		VkBool32 depthTestEnable, VkBool32 depthWriteEnable, VkCompareOp depthCompareOp, VkBool32 depthBoundsTestEnable,
		VkBool32 stencilTestEnable, VkStencilOpState front, VkStencilOpState back, float minDepthBounds, float maxDepthBounds)
	{
		VkPipelineDepthStencilStateCreateInfo l_depthStencilStateCreationInfo{};

		l_depthStencilStateCreationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		l_depthStencilStateCreationInfo.depthTestEnable = depthTestEnable;
		l_depthStencilStateCreationInfo.depthWriteEnable = depthWriteEnable;
		l_depthStencilStateCreationInfo.depthCompareOp = depthCompareOp;
		l_depthStencilStateCreationInfo.depthBoundsTestEnable = depthBoundsTestEnable;
		l_depthStencilStateCreationInfo.stencilTestEnable = stencilTestEnable;
		l_depthStencilStateCreationInfo.front = front;
		l_depthStencilStateCreationInfo.back = back;
		l_depthStencilStateCreationInfo.minDepthBounds = minDepthBounds;
		l_depthStencilStateCreationInfo.maxDepthBounds = maxDepthBounds;
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

//--------------------------------------------------------
//			Image Stuff
//--------------------------------------------------------

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