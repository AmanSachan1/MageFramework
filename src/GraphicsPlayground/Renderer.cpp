#include "Renderer.h"

Renderer::Renderer(VkDevice& logicalDevice, VkPhysicalDevice& physicalDevice, 
	VulkanPresentation* presentation, 
	Camera* camera, uint32_t width, uint32_t height)
	: m_logicalDevice(logicalDevice), m_physicalDevice(physicalDevice),
	m_presentationObject(presentation),
	m_camera(camera),
	m_windowWidth(width),
	m_windowHeight(height)
{
	initialize();
}
Renderer::~Renderer()
{
	vkDestroyPipelineLayout(m_logicalDevice, m_graphicsPipelineLayout, nullptr);
	vkDestroyRenderPass(m_logicalDevice, m_renderPass, nullptr);
}

void Renderer::initialize()
{
	createRenderPass();
	createAllPipelines();
}
void Renderer::renderLoop()
{
}

void recreate(uint32_t width, uint32_t height);
void DestroyOnWindowResize();

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

	VkFormat swapChainImageFormat = m_presentationObject->GetVkImageFormat();
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

	RenderPassUtility::createRenderPass(m_logicalDevice, m_renderPass, 1, &colorAttachment, 1, &subpass, 0, nullptr);
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
	VkShaderModule vertShaderModule = ShaderUtils::createShaderModule("GraphicsPlayground/shaders/shader.vert.spv", m_logicalDevice);
	VkShaderModule fragShaderModule = ShaderUtils::createShaderModule("GraphicsPlayground/shaders/shader.frag.spv", m_logicalDevice);

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
	VkViewport viewport = VulkanStructures::createViewport(
		0.0f, 0.0f,
		static_cast<float>(m_presentationObject->GetVkExtent().width),
		static_cast<float>(m_presentationObject->GetVkExtent().height),
		0.0f, 1.0f);

	// While viewports define the transformation from the image to the framebuffer, 
	// scissor rectangles define in which regions pixels will actually be stored.
	// we simply want to draw to the entire framebuffer, so we'll specify a scissor rectangle that covers the framebuffer entirely
	VkRect2D scissor = VulkanStructures::createRectangle({ 0,0 }, m_presentationObject->GetVkExtent());

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
	VkGraphicsPipelineCreateInfo graphicsPipelineInfo = {};
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