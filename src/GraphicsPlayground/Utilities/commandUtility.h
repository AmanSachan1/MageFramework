#pragma once
#include <global.h>

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