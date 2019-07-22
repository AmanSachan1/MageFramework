#include "renderPassManager.h"

RenderPassManager::RenderPassManager(VulkanDevices* devices, VkQueue& queue, VkCommandPool& cmdPool,
	VulkanPresentation* presentationObject, int numSwapChainImages, VkExtent2D windowExtents)
	: m_devices(devices), m_logicalDevice(devices->getLogicalDevice()), m_pDevice(devices->getPhysicalDevice()),
	m_graphicsQueue(queue), m_cmdPool(cmdPool), m_presentationObj(presentationObject),
	m_numSwapChainImages(numSwapChainImages), m_windowExtents(windowExtents)
{
	createDepthResources();
	createRenderPasses();
	createFrameBuffers();
}
RenderPassManager::~RenderPassManager()
{
	//Destroy Renderpasses
	vkDestroyRenderPass(m_logicalDevice, m_toDisplayRenderPass.renderPass, nullptr);
	vkDestroyRenderPass(m_logicalDevice, m_geomRasterPass.renderPass, nullptr);
	//vkDestroyRenderPass(m_logicalDevice, m_32bitPostProcessPasses.renderPass, nullptr);
	//vkDestroyRenderPass(m_logicalDevice, m_8bitPostProcessPasses.renderPass, nullptr);


}
void RenderPassManager::cleanup()
{
	// Destroy Depth Image Common to every render pass
	vkDestroyImage(m_logicalDevice, m_depth.image, nullptr);
	vkFreeMemory(m_logicalDevice, m_depth.memory, nullptr);
	vkDestroyImageView(m_logicalDevice, m_depth.view, nullptr);

	for (uint32_t i = 0; i < m_numSwapChainImages; i++)
	{
		// Destroy Framebuffers
		vkDestroyFramebuffer(m_logicalDevice, m_toDisplayRenderPass.frameBuffers[i], nullptr);
		vkDestroyFramebuffer(m_logicalDevice, m_geomRasterPass.frameBuffers[i], nullptr);

		// Destroy the color attachments
		{
			// Don't need to do this for m_toDisplayRenderPass
			// m_geomRasterPass.color
			vkDestroyImage(m_logicalDevice, m_geomRasterPass.color[i].image, nullptr);
			vkDestroyImageView(m_logicalDevice, m_geomRasterPass.color[i].view, nullptr);
			vkFreeMemory(m_logicalDevice, m_geomRasterPass.color[i].memory, nullptr);
		}
	}
	
	// Destroy Samplers
	vkDestroySampler(m_logicalDevice, m_geomRasterPass.sampler, nullptr);
}
void RenderPassManager::recreate(VkExtent2D windowExtents)
{
	m_windowExtents = windowExtents;
	createDepthResources();
	createFrameBuffers();
}

void RenderPassManager::createDepthResources()
{
	// At this point in time I'm only creating one depth image but to take advantage of more parallelization,
	// you may need to have as many depth images as there are swapchain images.
	// Reddit Discussion here: https://www.reddit.com/r/vulkan/comments/9at7rz/api_without_secrets_the_practical_approach_to/e542onj/
	createDepthAttachment(m_depth, m_presentationObj->getVkExtent());
}
void RenderPassManager::createRenderPasses()
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
	
	VkFormat swapChainImageFormat = m_presentationObj->getSwapChainImageFormat();
	VkFormat color32bitFormat = m_presentationObj->getSwapChainImageFormat();

	createRenderPass_ToDisplay(swapChainImageFormat); // Displays to swapchain framebuffer; Has to exist, other passes are optional
	createRenderPass_geomRasterPass(swapChainImageFormat); // Renders Geometery onto a set of offscreen framebuffers
	createRenderPass_32bitPostProcess(color32bitFormat, swapChainImageFormat);
	createRenderPass_8bitPostProcess(swapChainImageFormat);
}
void RenderPassManager::createFrameBuffers()
{
	// Framebuffers are essentially ImageView objects with more meta data. 
	// Framebuffers are not limited to use by the swapchain, and can also represent intermediate stages that store the results of renderpasses.

	// The attachments specified during render pass creation are bound by wrapping them into a VkFramebuffer object. 
	// A framebuffer object references all of the VkImageView objects that represent the attachments. 
	// Think color attachment and depth attachment 

	VkFormat swapChainImageFormat = m_presentationObj->getSwapChainImageFormat();

	// To Display
	{
		// The framebuffers used for presentation are special and will be managed by the VulkanPresentation class, we just reference them here.
		m_toDisplayRenderPass.frameBuffers.resize(m_numSwapChainImages);
		m_toDisplayRenderPass.extents = m_windowExtents;
		
		for (uint32_t i = 0; i < m_numSwapChainImages; i++)
		{
			// This one is different from the other render passes because it presents to the swap chain which has already created the
			// image, imageview, image memory, format, and sampler as part of the swapchain which we just reference here.

			std::array<VkImageView, 2> attachments = { m_presentationObj->getVkImageView(i), m_depth.view };

			RenderPassUtil::createFrameBuffer(m_logicalDevice, m_toDisplayRenderPass.frameBuffers[i], m_windowExtents,
				m_toDisplayRenderPass.renderPass, 2, attachments.data());

		}
	}
	// Render Geometry
	{
		m_geomRasterPass.frameBuffers.resize(m_numSwapChainImages);
		m_geomRasterPass.color.resize(m_numSwapChainImages);
		m_geomRasterPass.imageSetInfo.resize(m_numSwapChainImages);
		m_geomRasterPass.extents = m_windowExtents;

		createFrameBufferAttachment(m_geomRasterPass.color, m_geomRasterPass.sampler, swapChainImageFormat, m_windowExtents);

		for (uint32_t i = 0; i < m_numSwapChainImages; i++)
		{				
			std::array<VkImageView, 2> attachments = { m_geomRasterPass.color[i].view, m_depth.view };

			RenderPassUtil::createFrameBuffer(m_logicalDevice, m_geomRasterPass.frameBuffers[i], m_windowExtents,
				m_geomRasterPass.renderPass, static_cast<uint32_t>(attachments.size()), attachments.data());

			// Fill a descriptor for later use in a descriptor set 
			m_geomRasterPass.imageSetInfo[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			m_geomRasterPass.imageSetInfo[i].imageView = m_geomRasterPass.color[i].view;
			m_geomRasterPass.imageSetInfo[i].sampler = m_geomRasterPass.sampler;
		}
	}
	// 32 Bit Post Process Passes
	{
	}
	// 8 Bit Passes
	{
	}
}

void RenderPassManager::createRenderPass_ToDisplay(VkFormat swapChainImageFormat)
{
	// Render to the actual swapchain 
	//--------------------------------

	const unsigned int numAttachments = 2;
	const uint32_t numSubpasses = 1;
	const uint32_t numSubpassDependencies = 2;
	const VkImageLayout finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	VkSubpassDescription subpassDescriptions;
	std::vector<VkAttachmentDescription> attachments; attachments.reserve(numAttachments);
	std::vector<VkAttachmentReference> attachmentReferences; attachmentReferences.reserve(numAttachments);	

	//attachment references, and a subpass description
	renderPassCreationHelper_typical(swapChainImageFormat, m_depth.format, finalLayout, subpassDescriptions, attachments, attachmentReferences);

	std::array<VkSubpassDependency, numSubpassDependencies> subpassDependencies = {};
	{
		// Transition from tasks before this render pass (including runs through other pipelines before it, hence bottom of pipe)
		subpassDependencies[0] =
			RenderPassUtil::subpassDependency(VK_SUBPASS_EXTERNAL, 0,
				VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_ACCESS_MEMORY_READ_BIT,
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);

		//Transition from actual subpass to tasks after the renderpass
		subpassDependencies[1] =
			RenderPassUtil::subpassDependency(0, VK_SUBPASS_EXTERNAL,
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
				VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_ACCESS_MEMORY_READ_BIT);
	}

	RenderPassUtil::createRenderPass(m_logicalDevice, 
		m_toDisplayRenderPass.renderPass,
		static_cast<uint32_t>(attachments.size()), attachments.data(),
		numSubpasses, &subpassDescriptions,
		numSubpassDependencies, subpassDependencies.data());
}
void RenderPassManager::createRenderPass_geomRasterPass(VkFormat colorFormat)
{
	// Render Geometry to an offscreen buffer
	//---------------------------------------

	const unsigned int numAttachments = 2;
	const uint32_t numSubpasses = 1;
	const VkImageLayout finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	VkSubpassDescription subpassDescriptions;
	std::vector<VkAttachmentDescription> attachments; attachments.reserve(numAttachments);
	std::vector<VkAttachmentReference> attachmentReferences; attachmentReferences.reserve(numAttachments);
	
	// Create attachments, attachment references, and a subpass description
	renderPassCreationHelper_typical(colorFormat, m_depth.format, finalLayout, subpassDescriptions, attachments, attachmentReferences);

	std::array<VkSubpassDependency, 2> subpassDependencies = {};
	{
		// Transition from tasks before this render pass (including runs through other pipelines before it, hence fragment shader bit)
		subpassDependencies[0] =
			RenderPassUtil::subpassDependency(VK_SUBPASS_EXTERNAL, 0,
				VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT,
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);

		//Transition from actual subpass to tasks after the renderpass -- to be read in the fragment shader of another render pass
		subpassDependencies[1] =
			RenderPassUtil::subpassDependency(0, VK_SUBPASS_EXTERNAL,
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
				VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT);
	}

	RenderPassUtil::createRenderPass(m_logicalDevice,
		m_geomRasterPass.renderPass,
		static_cast<uint32_t>(attachments.size()), attachments.data(),
		numSubpasses, &subpassDescriptions,
		static_cast<uint32_t>(subpassDependencies.size()), subpassDependencies.data());
}
void RenderPassManager::createRenderPass_32bitPostProcess(VkFormat colorFormat32bit, VkFormat colorFormat8bit)
{
	// The last render Pass here must be a tone map pass to convert images to a 8bit format

	// Apply post process effects that absolutely must happen in 32bit space
	//-----------------------------------------------------------------------

	//// Standard Passess -- one pass and multiple subpasses
	//{
	//	const unsigned int numAttachments = 2;
	//	const uint32_t numSubpasses = 1;
	//	VkSubpassDescription subpassDescriptions;
	//	std::vector<VkAttachmentDescription> attachments; attachments.reserve(numAttachments);
	//	std::vector<VkAttachmentReference> attachmentReferences; attachmentReferences.reserve(numAttachments);

	//	// Create attachments, attachment references, and a subpass description
	//	//postProcessRPHelper_standard(colorFormat32bit, m_depth.format, subpassDescriptions, attachments, attachmentReferences);
	//}
	//// Stand Alone Passes -- Multiple renderpasses each with a single subpass
	//{
	//	const unsigned int numAttachments = 2;
	//	const uint32_t numSubpasses = 1;
	//	VkSubpassDescription subpassDescriptions;
	//	std::vector<VkAttachmentDescription> attachments; attachments.reserve(numAttachments);
	//	std::vector<VkAttachmentReference> attachmentReferences; attachmentReferences.reserve(numAttachments);

	//	// Create attachments, attachment references, and a subpass description
	//	//renderPassCreationHelper_typical(colorFormat, m_depth.format, finalLayout, subpassDescriptions, attachments, attachmentReferences);
	//}
}
void RenderPassManager::createRenderPass_8bitPostProcess(VkFormat colorFormat8bit)
{
	// Apply post process effects that happen in 8bit space
	//------------------------------------------------------

	//// Standard Passess -- one pass and multiple subpasses
	//{
	//	const unsigned int numAttachments = 2;
	//	const uint32_t numSubpasses = 1;
	//	VkSubpassDescription subpassDescriptions;
	//	std::vector<VkAttachmentDescription> attachments; attachments.reserve(numAttachments);
	//	std::vector<VkAttachmentReference> attachmentReferences; attachmentReferences.reserve(numAttachments);

	//	// Create attachments, attachment references, and a subpass description
	//	//renderPassCreationHelper_typical(colorFormat, m_depth.format, finalLayout, subpassDescriptions, attachments, attachmentReferences);
	//}
	//// Stand Alone Passes -- Multiple renderpasses each with a single subpass
	//{
	//	const unsigned int numAttachments = 2;
	//	const uint32_t numSubpasses = 1;
	//	VkSubpassDescription subpassDescriptions;
	//	std::vector<VkAttachmentDescription> attachments; attachments.reserve(numAttachments);
	//	std::vector<VkAttachmentReference> attachmentReferences; attachmentReferences.reserve(numAttachments);

	//	// Create attachments, attachment references, and a subpass description
	//	//renderPassCreationHelper_typical(colorFormat, m_depth.format, finalLayout, subpassDescriptions, attachments, attachmentReferences);
	//}
}

void RenderPassManager::renderPassCreationHelper_typical(
	VkFormat colorFormat, VkFormat depthFormat, VkImageLayout finalLayout,
	VkSubpassDescription &subpassDescriptions, 
	std::vector<VkAttachmentDescription> &attachments, 
	std::vector<VkAttachmentReference> &attachmentReferences)
{
	// Create Attachments and Attacment References
	{
		// Color Attachment 
		attachments.push_back( 
			RenderPassUtil::attachmentDescription(colorFormat, VK_SAMPLE_COUNT_1_BIT,
				VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE,			  //color data
				VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE,    //stencil data
				VK_IMAGE_LAYOUT_UNDEFINED, finalLayout)  //initial and final layout
		);

		// Depth Attachment 
		attachments.push_back(
			RenderPassUtil::attachmentDescription(depthFormat, VK_SAMPLE_COUNT_1_BIT,
				VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_DONT_CARE,				 //depth data
				VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE,			 //stencil data
				VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) //initial and final layout
		);
	}

	// Create Attachment References	
	{
		// Color
		attachmentReferences.push_back(RenderPassUtil::attachmentReference(0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL));
		// Depth
		attachmentReferences.push_back(RenderPassUtil::attachmentReference(1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL));
	}
	
	// Subpass -- Forward Render Pass
	subpassDescriptions = 
		RenderPassUtil::subpassDescription(VK_PIPELINE_BIND_POINT_GRAPHICS,
			0, nullptr, //input attachments
			1, &attachmentReferences[0], //color attachments
			nullptr, //resolve attachments -- only comes into play when you have multiple samples (think MSAA)
			&attachmentReferences[1], //depth and stencil
			0, nullptr); //preserve attachments
}

void postProcessRPHelper_standard( VkFormat colorFormat, VkFormat depthFormat,
	VkSubpassDescription &subpassDescriptions,
	std::vector<VkAttachmentDescription> &attachments,
	std::vector<VkAttachmentReference> &attachmentReferences )
{
	const VkImageLayout imgLayout = VK_IMAGE_LAYOUT_GENERAL; // because subpasses will be reading and writing to the same image

	// Create Attachments and Attacment References
	{
		// Color Attachment 
		attachments.push_back(
			RenderPassUtil::attachmentDescription(colorFormat, VK_SAMPLE_COUNT_1_BIT,
				VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE,			  //color data
				VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE,    //stencil data
				VK_IMAGE_LAYOUT_UNDEFINED, imgLayout)  //initial and final layout
		);

		// Depth Attachment 
		attachments.push_back(
			RenderPassUtil::attachmentDescription(depthFormat, VK_SAMPLE_COUNT_1_BIT,
				VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_DONT_CARE,				 //depth data
				VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE,			 //stencil data
				VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) //initial and final layout
		);
	}

	// Create Attachment References	
	{
		// Color
		attachmentReferences.push_back(RenderPassUtil::attachmentReference(0, imgLayout));
		// Depth
		attachmentReferences.push_back(RenderPassUtil::attachmentReference(1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL));
	}

	// Subpasses
	subpassDescriptions =
		RenderPassUtil::subpassDescription(VK_PIPELINE_BIND_POINT_GRAPHICS,
			1, &attachmentReferences[0], //input attachment
			1, &attachmentReferences[0], //color attachment
			nullptr, //resolve attachments -- only comes into play when you have multiple samples (think MSAA)
			&attachmentReferences[1], //depth and stencil
			0, nullptr); //preserve attachments
}
void postProcessRPHelper_standalone(
	VkFormat colorFormat, VkFormat depthFormat, VkImageLayout finalLayout,
	VkSubpassDescription &subpassDescriptions,
	std::vector<VkAttachmentDescription> &attachments,
	std::vector<VkAttachmentReference> &attachmentReferences)
{

}


void RenderPassManager::createFrameBufferAttachment(std::vector<FrameBufferAttachment>& fbAttachments, 
	VkSampler& sampler, VkFormat imgFormat, VkExtent2D& imageExtent)
{
	const uint32_t mipLevels = 1;
	const uint32_t arrayLayers = 1;
	const float anisotropy = 16;
	const VkExtent3D extents = { imageExtent.width, imageExtent.height , 1 };
	const VkSamplerAddressMode samplerAddressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

	for (uint32_t i = 0; i < m_numSwapChainImages; i++)
	{
		fbAttachments[i].format = imgFormat;

		// Create 2D Image
		ImageUtil::createImage(m_logicalDevice, m_pDevice, fbAttachments[i].image, fbAttachments[i].memory,
			VK_IMAGE_TYPE_2D, fbAttachments[i].format, extents,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TILING_OPTIMAL, mipLevels, arrayLayers,
			VK_IMAGE_LAYOUT_UNDEFINED, VK_SHARING_MODE_EXCLUSIVE);

		// Create 2D image View
		ImageUtil::createImageView(m_logicalDevice, fbAttachments[i].image, &fbAttachments[i].view, VK_IMAGE_VIEW_TYPE_2D,
			fbAttachments[i].format, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels, nullptr);
	}

	// Create sampler
	ImageUtil::createImageSampler(m_logicalDevice, sampler, VK_FILTER_LINEAR, VK_FILTER_LINEAR, samplerAddressMode,
		VK_SAMPLER_MIPMAP_MODE_LINEAR, 0, 0, static_cast<float>(mipLevels), anisotropy, VK_COMPARE_OP_NEVER);
}
void RenderPassManager::createDepthAttachment(FrameBufferAttachment& depthAttachment, VkExtent2D& imageExtent)
{
	const uint32_t mipLevels = 1;
	const uint32_t arrayLayers = 1;
	const float anisotropy = 16;
	const VkExtent3D extents = { imageExtent.width, imageExtent.height , 1 };

	depthAttachment.format = FormatUtil::findDepthFormat(m_pDevice);

	ImageUtil::createImage(m_logicalDevice, m_pDevice, m_depth.image, m_depth.memory,
		VK_IMAGE_TYPE_2D, m_depth.format, extents,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		VK_SAMPLE_COUNT_1_BIT,
		VK_IMAGE_TILING_OPTIMAL,
		1, 1, VK_IMAGE_LAYOUT_UNDEFINED, VK_SHARING_MODE_EXCLUSIVE);

	ImageUtil::createImageView(m_logicalDevice, depthAttachment.image, &depthAttachment.view,
		VK_IMAGE_VIEW_TYPE_2D, depthAttachment.format, VK_IMAGE_ASPECT_DEPTH_BIT, 1, nullptr);

	ImageUtil::transitionImageLayout(m_logicalDevice, m_graphicsQueue, m_cmdPool, depthAttachment.image, depthAttachment.format,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1);
}