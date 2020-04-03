#pragma once
#include <string>
#include <Vulkan/RendererBackend/vRendererBackend.h>

/// Render Passes and Frame Buffers

inline void VulkanRendererBackend::cleanupRenderPassesAndFrameResources()
{
	// Destroy Depth Image Common to every render pass
	vkDestroyImage(m_logicalDevice, m_depth.image, nullptr);
	vkFreeMemory(m_logicalDevice, m_depth.memory, nullptr);
	vkDestroyImageView(m_logicalDevice, m_depth.view, nullptr);

	for (uint32_t i = 0; i < m_numSwapChainImages; i++)
	{
		// Destroy Framebuffers
		vkDestroyFramebuffer(m_logicalDevice, m_compositeComputeOntoRasterRPI.frameBuffers[i], nullptr);
		vkDestroyFramebuffer(m_logicalDevice, m_rasterRPI.frameBuffers[i], nullptr);

		// Destroy the color attachments
		{
			// m_rasterRPI.color
			vkDestroyImage(m_logicalDevice, m_rasterRPI.color[i].image, nullptr);
			vkDestroyImageView(m_logicalDevice, m_rasterRPI.color[i].view, nullptr);
			vkFreeMemory(m_logicalDevice, m_rasterRPI.color[i].memory, nullptr);

			// m_compositeComputeOntoRasterRPI.color
			vkDestroyImage(m_logicalDevice, m_compositeComputeOntoRasterRPI.color[i].image, nullptr);
			vkDestroyImageView(m_logicalDevice, m_compositeComputeOntoRasterRPI.color[i].view, nullptr);
			vkFreeMemory(m_logicalDevice, m_compositeComputeOntoRasterRPI.color[i].memory, nullptr);
		}
	}

	// Destroy Samplers
	vkDestroySampler(m_logicalDevice, m_rasterRPI.sampler, nullptr);
	vkDestroySampler(m_logicalDevice, m_compositeComputeOntoRasterRPI.sampler, nullptr);

	// Destroy Renderpasses
	vkDestroyRenderPass(m_logicalDevice, m_compositeComputeOntoRasterRPI.renderPass, nullptr);
	vkDestroyRenderPass(m_logicalDevice, m_rasterRPI.renderPass, nullptr);
}
inline void VulkanRendererBackend::createRenderPasses(const VkImageLayout& beforeRenderPassExecuted, const VkImageLayout& afterRenderPassExecuted)
{
	// https://vulkan-tutorial.com/Drawing_a_triangle/Graphics_pipeline_basics/Render_passes
	// A VkRenderPass object tells us the following things:
	// - the framebuffer attachments that will be used while rendering
	// - how many color and depth buffers there will be
	// - how many samples to use for each of them 
	// - and how their contents should be handled throughout the rendering operations

	// A single renderpass can consist of multiple subpasses. 
	// Subpasses are subsequent rendering operations that depend on the contents of framebuffers in previous passes, 
	// for example a sequence of post-processing effects that are applied one after another. If you group these 
	// rendering operations into one render pass, then Vulkan is able to reorder the operations and 
	// conserve memory bandwidth for possibly better performance. 

	// Indices for attachment descriptions are based on the ordering of their respective attachments. Similar indexing applies to attachment references

	// Subpass dependencies define layout transitions between subpasses -- Vulkan automatically handles these transitions

	// --- Raster Render Pass --- 
	// Renders Geometry to an offscreen buffer
	{
		std::vector<VkSubpassDependency> subpassDependencies;
		{
			// Transition from tasks before this render pass (including runs through other pipelines before it, hence fragment shader bit)
			subpassDependencies.push_back(
				RenderPassUtil::subpassDependency(VK_SUBPASS_EXTERNAL, 0,
					VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT,
					VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT));

			//Transition from actual subpass to tasks after the renderpass -- to be read in the fragment shader of another render pass
			subpassDependencies.push_back(
				RenderPassUtil::subpassDependency(0, VK_SUBPASS_EXTERNAL,
					VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
					VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT));
		}
		RenderPassUtil::renderPassCreationHelper(m_logicalDevice, m_rasterRPI.renderPass,
			m_highResolutionRenderFormat, m_depthFormat, beforeRenderPassExecuted, afterRenderPassExecuted, subpassDependencies);
	}

	// --- Raster + Compute Composite Render Pass --- 
	// Composites previous Passes and renders result onto swapchain. 
	// It is the last pass before the UI render pass, which also renders to the swapchain image.
	// Has to exist, other passes are optional
	{
		std::vector<VkSubpassDependency> subpassDependencies;
		{
			// Transition from tasks before this render pass (including runs through other pipelines before it, hence bottom of pipe)
			subpassDependencies.push_back(
				RenderPassUtil::subpassDependency(VK_SUBPASS_EXTERNAL, 0,
					VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_ACCESS_MEMORY_READ_BIT,
					VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT));

			//Transition from actual subpass to tasks after the renderpass
			subpassDependencies.push_back(
				RenderPassUtil::subpassDependency(0, VK_SUBPASS_EXTERNAL,
					VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
					VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_ACCESS_MEMORY_READ_BIT));
		}
		RenderPassUtil::renderPassCreationHelper(m_logicalDevice, m_compositeComputeOntoRasterRPI.renderPass,
			m_highResolutionRenderFormat, m_depthFormat, beforeRenderPassExecuted, afterRenderPassExecuted, subpassDependencies);
	}
}
inline void VulkanRendererBackend::createDepthResources()
{
	// At this point in time I'm only creating one depth image but to take advantage of more parallelization,
	// you may need to have as many depth images as there are swapchain images.
	// Reddit Discussion here: https://www.reddit.com/r/vulkan/comments/9at7rz/api_without_secrets_the_practical_approach_to/e542onj/

	FrameResourcesUtil::createDepthAttachment(m_logicalDevice, m_physicalDevice,
		m_graphicsQueue, m_graphicsCommandPool, m_depth, m_depthFormat, m_windowExtents);
}
inline void VulkanRendererBackend::createFrameBuffers(
	const VkImageLayout& layoutBeforeImageCreation, 
	const VkImageLayout& layoutToTransitionImageToAfterCreation,
	const VkImageLayout& layoutAfterRenderPassExecuted )
{
	// Framebuffers are essentially ImageView objects with more meta data. 
	// Framebuffers are not limited to use by the swapchain, and can also represent intermediate stages that store the results of renderpasses.

	// The attachments specified during render pass creation are bound by wrapping them into a VkFramebuffer object. 
	// A framebuffer object references all of the VkImageView objects that represent the attachments. 
	// Think color attachment and depth attachment 

	const VkImageUsageFlags frameBufferUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

	// Render Geometry
	{
		m_rasterRPI.frameBuffers.resize(m_numSwapChainImages);
		m_rasterRPI.color.resize(m_numSwapChainImages);
		m_rasterRPI.imageSetInfo.resize(m_numSwapChainImages);
		m_rasterRPI.extents = m_windowExtents;

		FrameResourcesUtil::createFrameBufferAttachments(m_logicalDevice, m_physicalDevice, m_graphicsQueue, m_graphicsCommandPool,
			m_numSwapChainImages, m_rasterRPI.color, m_rasterRPI.sampler, m_highResolutionRenderFormat,
			layoutBeforeImageCreation, layoutToTransitionImageToAfterCreation, m_windowExtents, frameBufferUsage);

		const uint32_t numAttachments = 2;
		for (uint32_t i = 0; i < m_numSwapChainImages; i++)
		{
			std::array<VkImageView, numAttachments> attachments = { m_rasterRPI.color[i].view, m_depth.view };

			FrameResourcesUtil::createFrameBuffer(m_logicalDevice, 
				m_rasterRPI.frameBuffers[i], m_rasterRPI.renderPass,
				m_windowExtents, numAttachments, attachments.data());

			// Fill a descriptor for later use in a descriptor set 
			m_rasterRPI.imageSetInfo[i].imageLayout = layoutAfterRenderPassExecuted;
			m_rasterRPI.imageSetInfo[i].imageView = m_rasterRPI.color[i].view;
			m_rasterRPI.imageSetInfo[i].sampler = m_rasterRPI.sampler;
		}
	}

	// Composite Raster pass with Compute passes
	{
		// The framebuffers used for presentation are special and will be managed by the VulkanPresentation class, we just reference them here.
		m_compositeComputeOntoRasterRPI.frameBuffers.resize(m_numSwapChainImages);
		m_compositeComputeOntoRasterRPI.color.resize(m_numSwapChainImages);
		m_compositeComputeOntoRasterRPI.imageSetInfo.resize(m_numSwapChainImages);
		m_compositeComputeOntoRasterRPI.extents = m_windowExtents;

		FrameResourcesUtil::createFrameBufferAttachments(m_logicalDevice, m_physicalDevice, m_graphicsQueue, m_graphicsCommandPool,
			m_numSwapChainImages, m_compositeComputeOntoRasterRPI.color, m_compositeComputeOntoRasterRPI.sampler, m_highResolutionRenderFormat,
			layoutBeforeImageCreation, layoutToTransitionImageToAfterCreation, m_windowExtents, frameBufferUsage);

		const uint32_t numAttachments = 2;
		for (uint32_t i = 0; i < m_numSwapChainImages; i++)
		{
			std::array<VkImageView, numAttachments> attachments = { m_compositeComputeOntoRasterRPI.color[i].view, m_depth.view };

			FrameResourcesUtil::createFrameBuffer(m_logicalDevice,
				m_compositeComputeOntoRasterRPI.frameBuffers[i], m_compositeComputeOntoRasterRPI.renderPass,
				m_windowExtents, numAttachments, attachments.data());

			// Fill a descriptor for later use in a descriptor set 
			m_compositeComputeOntoRasterRPI.imageSetInfo[i].imageLayout = layoutAfterRenderPassExecuted;
			m_compositeComputeOntoRasterRPI.imageSetInfo[i].imageView = m_compositeComputeOntoRasterRPI.color[i].view;
			m_compositeComputeOntoRasterRPI.imageSetInfo[i].sampler = m_compositeComputeOntoRasterRPI.sampler;
		}
	}
}