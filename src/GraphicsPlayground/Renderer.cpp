#include "Renderer.h"

Renderer::Renderer(GLFWwindow* window, VulkanDevices* devices, VulkanPresentation* presentation,
	Camera* camera, uint32_t width, uint32_t height)
	: m_window(window), 
	m_devices(devices), m_logicalDevice(devices->getLogicalDevice()), m_physicalDevice(devices->getPhysicalDevice()),
	m_presentationObject(presentation),
	m_camera(camera),
	m_windowWidth(width),
	m_windowHeight(height)
{
	initialize();
}
Renderer::~Renderer()
{
	cleanup();

	// Command Pools
	vkDestroyCommandPool(m_logicalDevice, m_graphicsCommandPool, nullptr);
	vkDestroyCommandPool(m_logicalDevice, m_computeCommandPool, nullptr);
}

void Renderer::initialize()
{
	createCommandPoolsAndBuffers();
	createRenderPass();
	createAllPipelines();
	createFrameBuffers();	
	recordAllCommandBuffers();
}
void Renderer::renderLoop()
{
	// Wait for the the frame to be finished before working on it
	VkFence inFlightFence = m_presentationObject->getInFlightFence();

	// The VK_TRUE we pass in vkWaitForFences indicates that we want to wait for all fences.
	vkWaitForFences(m_logicalDevice, 1, &inFlightFence, VK_TRUE, std::numeric_limits<uint64_t>::max());

	// Acquire image from swapchain
	bool flag_recreateSwapChain = m_presentationObject->acquireNextSwapChainImage(m_logicalDevice);
	if (!flag_recreateSwapChain) { recreate(); };	

	//-------------------------------------
	//	Submit Commands To Graphics Queue
	//-------------------------------------
	VkSemaphore waitSemaphores[] = { m_presentationObject->getImageAvailableVkSemaphore() };
	// We want to wait with writing colors to the image until it's available, so we're specifying the stage of the graphics pipeline 
	// that writes to the color attachment. That means that theoretically the implementation can already start executing our vertex 
	// shader and such while the image is not available yet.
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	VkSemaphore signalSemaphores[] = { m_presentationObject->getRenderFinishedVkSemaphore() };
	VkQueue graphicsQueue = m_devices->getQueue(QueueFlags::Graphics);
	VkCommandBuffer* graphicsCommandBuffer = &m_graphicsCommandBuffers[m_presentationObject->getIndex()];

	vkResetFences(m_logicalDevice, 1, &inFlightFence);
	VulkanUtil::submitToGraphicsQueue(graphicsQueue, 1, waitSemaphores, waitStages, 1, graphicsCommandBuffer, 1, signalSemaphores, inFlightFence);

	// Return the image to the swapchain for presentation
	flag_recreateSwapChain = m_presentationObject->presentImageToSwapChain(m_logicalDevice, m_resizeFrameBuffer);
	if (!flag_recreateSwapChain) { recreate(); };

	m_presentationObject->advanceCurrentFrameIndex();
}

void Renderer::recreate()
{
	m_resizeFrameBuffer = false;
	// This while loop handles the case of minimization of the window
	int width = 0, height = 0;
	while (width == 0 || height == 0) 
	{
		glfwGetFramebufferSize(m_window, &width, &height);
		glfwWaitEvents();
	}

	cleanup();

	m_presentationObject->create(m_window);
	createRenderPass();
	createAllPipelines();
	createFrameBuffers();

	m_graphicsCommandBuffers.resize(m_presentationObject->getCount());
	m_computeCommandBuffers.resize(m_presentationObject->getCount());
	VulkanCommandUtil::allocateCommandBuffers(m_logicalDevice, m_graphicsCommandPool, m_graphicsCommandBuffers);
	VulkanCommandUtil::allocateCommandBuffers(m_logicalDevice, m_computeCommandPool, m_computeCommandBuffers);
	recordAllCommandBuffers();

	m_resizeFrameBuffer = false;
}
void Renderer::cleanup()
{
	vkDeviceWaitIdle(m_logicalDevice);

	for (size_t i = 0; i < m_frameBuffers.size(); i++)
	{
		vkDestroyFramebuffer(m_logicalDevice, m_frameBuffers[i], nullptr);
	}

	vkFreeCommandBuffers(m_logicalDevice, m_graphicsCommandPool, static_cast<uint32_t>(m_graphicsCommandBuffers.size()), m_graphicsCommandBuffers.data());
	vkFreeCommandBuffers(m_logicalDevice, m_computeCommandPool, static_cast<uint32_t>(m_computeCommandBuffers.size()), m_computeCommandBuffers.data());
	
	vkDestroyPipeline(m_logicalDevice, m_graphicsPipeline, nullptr);
	vkDestroyPipelineLayout(m_logicalDevice, m_graphicsPipelineLayout, nullptr);
	vkDestroyRenderPass(m_logicalDevice, m_renderPass, nullptr);
	
	m_presentationObject->cleanup();
}


void Renderer::recordAllCommandBuffers()
{
	for (unsigned int i = 0; i < m_graphicsCommandBuffers.size(); ++i)
	{
		VulkanCommandUtil::beginCommandBuffer(m_graphicsCommandBuffers[i]);
		recordGraphicsCommandBuffer(m_graphicsCommandBuffers[i], m_frameBuffers[i]);
		VulkanCommandUtil::endCommandBuffer(m_graphicsCommandBuffers[i]);
	}

	for (unsigned int i = 0; i < m_computeCommandBuffers.size(); ++i)
	{
		VulkanCommandUtil::beginCommandBuffer(m_computeCommandBuffers[i]);
		recordComputeCommandBuffer(m_computeCommandBuffers[i]);
		VulkanCommandUtil::endCommandBuffer(m_computeCommandBuffers[i]);
	}
}
void Renderer::recordGraphicsCommandBuffer(VkCommandBuffer& graphicsCmdBuffer, VkFramebuffer& frameBuffer)
{
	const VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
	const VkRect2D renderArea = VulkanUtil::createRectangle({ 0,0 }, m_presentationObject->getVkExtent());
	VulkanCommandUtil::beginRenderPass(graphicsCmdBuffer, m_renderPass, frameBuffer, renderArea, &clearColor, 1);
	vkCmdBindPipeline(graphicsCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);

	// Draw Command Params, aside from the command buffer:
	// vertexCount   : Even though we don't have a vertex buffer, we technically still have 3 vertices to draw.
	// instanceCount : Used for instanced rendering, use 1 if you're not doing that.
	// firstVertex   : Used as an offset into the vertex buffer, defines the lowest value of gl_VertexIndex.
	// firstInstance : Used as an offset for instanced rendering, defines the lowest value of gl_InstanceIndex.
	vkCmdDraw(graphicsCmdBuffer, 3, 1, 0, 0);

	vkCmdEndRenderPass(graphicsCmdBuffer);
}
void Renderer::recordComputeCommandBuffer(VkCommandBuffer& ComputeCmdBuffer)
{

}
void Renderer::createCommandPoolsAndBuffers()
{
	// Commands in Vulkan, like drawing operations and memory transfers, are not executed directly using function calls. 
	// You have to record all of the operations you want to perform in command buffer objects. The advantage of this is 
	// that all of the hard work of setting up the drawing commands can be done in advance and in multiple threads.

	// Command buffers will be automatically freed when their command pool is destroyed, so we don't need an explicit cleanup.

	// Because one of the drawing commands involves binding the right VkFramebuffer, we'll actually have to 
	// record a command buffer for every image in the swap chain once again.
	VulkanCommandUtil::createCommandPool(m_logicalDevice, m_graphicsCommandPool, m_devices->getQueueIndex(QueueFlags::Graphics));
	VulkanCommandUtil::createCommandPool(m_logicalDevice, m_computeCommandPool, m_devices->getQueueIndex(QueueFlags::Compute));

	m_graphicsCommandBuffers.resize(m_presentationObject->getCount());
	m_computeCommandBuffers.resize(m_presentationObject->getCount());

	VulkanCommandUtil::allocateCommandBuffers(m_logicalDevice, m_graphicsCommandPool, m_graphicsCommandBuffers);
	VulkanCommandUtil::allocateCommandBuffers(m_logicalDevice, m_computeCommandPool, m_computeCommandBuffers);
}

void Renderer::createFrameBuffers()
{
	// The attachments specified during render pass creation are bound by wrapping them into a VkFramebuffer object. 
	// A framebuffer object references all of the VkImageView objects that represent the attachments. 
	// In our case that will be only a single one: the color attachment. 
	// The image that we have to use for the attachment depends on which image the swap chain returns when we retrieve one for presentation. 
	// That means that we have to create a framebuffer for all of the images in the swap chain and use the one that corresponds to 
	// the retrieved image at drawing time.

	m_frameBuffers.resize(m_presentationObject->getCount());

	for (size_t i = 0; i < m_presentationObject->getCount(); i++)
	{
		VkImageView attachments[] = { m_presentationObject->getVkImageView(i) };

		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = m_renderPass;
		framebufferInfo.attachmentCount = 1;
		framebufferInfo.pAttachments = attachments;
		framebufferInfo.width = m_presentationObject->getVkExtent().width;
		framebufferInfo.height = m_presentationObject->getVkExtent().height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(m_logicalDevice, &framebufferInfo, nullptr, &m_frameBuffers[i]) != VK_SUCCESS) 
		{
			throw std::runtime_error("failed to create framebuffer!");
		}
	}
}

void Renderer::createRenderPass()
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

	VkFormat swapChainImageFormat = m_presentationObject->getVkImageFormat();
	VkAttachmentDescription colorAttachment =
		VulkanImageStructures::attachmentDescription(
			swapChainImageFormat, VK_SAMPLE_COUNT_1_BIT,
			VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, //color and depth data
			VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE, //stencil data
			VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

	// Our attachments array consists of a single VkAttachmentDescription, so its index is 0. 
	VkAttachmentReference colorAttachmentRef = VulkanImageStructures::attachmentReference(0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	
	// The index of the color attachment in the color Attachment array is directly referenced from the fragment shader with the
	// layout(location = 0) out vec4 outColor directive!
	VkSubpassDescription subpass = 
		RenderPassUtility::subpassDescription(VK_PIPELINE_BIND_POINT_GRAPHICS, 0, nullptr, 1, &colorAttachmentRef, nullptr, nullptr, 0, nullptr);

	VkSubpassDependency dependency = 
		RenderPassUtility::subpassDependency(
			VK_SUBPASS_EXTERNAL, 0,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);

	RenderPassUtility::createRenderPass(m_logicalDevice, m_renderPass, 1, &colorAttachment, 1, &subpass, 1, &dependency);
}

void Renderer::createAllPipelines()
{
	std::vector<VkDescriptorSetLayout> descriptorSetLayouts = {};

	m_graphicsPipelineLayout = VulkanPipelineCreation::createPipelineLayout(m_logicalDevice, descriptorSetLayouts, 0, nullptr);
	createGraphicsPipeline(m_graphicsPipeline, m_renderPass, 0);
}
void Renderer::createGraphicsPipeline(VkPipeline& graphicsPipeline, VkRenderPass& renderPass, unsigned int subpassIndex)
{
	//--------------------------------------------------------
	//---------------- Set up shader stages ------------------
	//--------------------------------------------------------
	// Reference: https://vulkan-tutorial.com/Drawing_a_triangle/Graphics_pipeline_basics/Shader_modules
	// Create vert and frag shader modules
	VkShaderModule vertShaderModule = ShaderUtils::createShaderModule("GraphicsPlayground/shaders/testShader.vert.spv", m_logicalDevice);
	VkShaderModule fragShaderModule = ShaderUtils::createShaderModule("GraphicsPlayground/shaders/testShader.frag.spv", m_logicalDevice);

	// Assign each shader module to the appropriate stage in the pipeline
	VkPipelineShaderStageCreateInfo vertShaderStageInfo =
		ShaderUtils::shaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vertShaderModule, "main");
	VkPipelineShaderStageCreateInfo fragShaderStageInfo =
		ShaderUtils::shaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fragShaderModule, "main");

	//Add Shadermodules to the list of shader stages
	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

	//--------------------------------------------------------
	//------------- Set up fixed-function stages -------------
	//--------------------------------------------------------
	// Reference: https://vulkan-tutorial.com/Drawing_a_triangle/Graphics_pipeline_basics/Fixed_functions

	// -------- Vertex input binding --------
	// Vertex binding describes at which rate to load data from GPU memory 

	// All of our per-vertex data is packed together in 1 array so we only have one binding; 
	// The binding param specifies index of the binding in array of bindings
	VkVertexInputBindingDescription vertexInputBinding = VulkanPipelineStructures::vertexInputBindingDesc(0, sizeof(Vertex));

	// Input attribute bindings describe shader attribute locations and memory layouts
	std::array<VkVertexInputAttributeDescription, 2> vertexInputAttributes;
	vertexInputAttributes[0] = VulkanPipelineStructures::vertexInputAttributeDesc(0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position));
	vertexInputAttributes[1] = VulkanPipelineStructures::vertexInputAttributeDesc(1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color));

	// -------- Vertex input --------
	VkPipelineVertexInputStateCreateInfo vertexInput =
		VulkanPipelineStructures::vertexInputInfo(1, &vertexInputBinding, 2, vertexInputAttributes.data());

	// -------- Input assembly --------
	// The VkPipelineInputAssemblyStateCreateInfo struct describes two things: what kind of geometry will be drawn 
	// from the vertices and if primitive restart should be enabled.

	VkPipelineInputAssemblyStateCreateInfo inputAssembly =
		VulkanPipelineStructures::inputAssemblyStateCreationInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FALSE);

	// -------- Tesselation State ---------
	// Set up Tesselation state here


	// -------- Viewport State ---------
	// Viewports and Scissors (rectangles that define in which regions pixels are stored)
	VkViewport viewport = VulkanUtil::createViewport(
		0.0f, 0.0f,
		static_cast<float>(m_presentationObject->getVkExtent().width),
		static_cast<float>(m_presentationObject->getVkExtent().height),
		0.0f, 1.0f);

	// While viewports define the transformation from the image to the framebuffer, 
	// scissor rectangles define in which regions pixels will actually be stored.
	// we simply want to draw to the entire framebuffer, so we'll specify a scissor rectangle that covers the framebuffer entirely
	VkRect2D scissor = VulkanUtil::createRectangle({ 0,0 }, m_presentationObject->getVkExtent());

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
			VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_CLOCKWISE, VK_FALSE, 0.0f, 0.0f, 0.0f);

	// -------- Multisampling --------
	// (turned off here)
	VkPipelineMultisampleStateCreateInfo multisampling =
		VulkanPipelineStructures::multiSampleStateCreationInfo(VK_SAMPLE_COUNT_1_BIT, VK_FALSE, 1.0f, nullptr, VK_FALSE, VK_FALSE);

	// -------- Depth and Stencil Testing --------
	VkPipelineDepthStencilStateCreateInfo depthAndStencil =
		VulkanPipelineStructures::depthStencilStateCreationInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS, VK_FALSE, VK_FALSE, {}, {}, 0.0f, 1.0f);

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

	// -------- Create graphics pipeline ---------
	VkGraphicsPipelineCreateInfo graphicsPipelineInfo = 
		VulkanPipelineStructures::graphicsPipelineCreationInfo(
			2, shaderStages,
			&vertexInput,
			&inputAssembly,
			nullptr, //tessellation
			&viewportState,
			&rasterizer,
			&multisampling,
			&depthAndStencil,
			&colorBlending,
			nullptr, //dynamicState
			m_graphicsPipelineLayout,
			renderPass, subpassIndex,
			VK_NULL_HANDLE, -1);

	VulkanPipelineCreation::createGraphicsPipelines(m_logicalDevice, VK_NULL_HANDLE, 1, &graphicsPipelineInfo, &graphicsPipeline);

	// No need for the shader modules anymore, so we destory them!
	vkDestroyShaderModule(m_logicalDevice, vertShaderModule, nullptr);
	vkDestroyShaderModule(m_logicalDevice, fragShaderModule, nullptr);
}