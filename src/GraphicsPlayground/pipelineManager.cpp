#include "pipelineManager.h"

PipelineManager::PipelineManager(VulkanDevices* devices, VulkanPresentation* presentationObject,
	RenderPassManager* renderPassManager, int numSwapChainImages, VkExtent2D windowExtents)
	: m_devices(devices), m_logicalDevice(devices->getLogicalDevice()), m_pDevice(devices->getPhysicalDevice()),
	m_presentationObj(presentationObject), m_renderPassManager(renderPassManager),
	m_numSwapChainImages(numSwapChainImages), m_windowExtents(windowExtents)
{}

PipelineManager::~PipelineManager()
{}

void PipelineManager::cleanup()
{
	// Compute Pipeline
	vkDestroyPipeline(m_logicalDevice, m_compute_P, nullptr);
	vkDestroyPipelineLayout(m_logicalDevice, m_compute_PL, nullptr);

	// Final Composite Pipeline
	vkDestroyPipeline(m_logicalDevice, m_finalComposite_P, nullptr);
	vkDestroyPipelineLayout(m_logicalDevice, m_finalComposite_PL, nullptr);

	// Rasterization Pipeline
	vkDestroyPipeline(m_logicalDevice, m_rasterization_P, nullptr);
	vkDestroyPipelineLayout(m_logicalDevice, m_rasterization_PL, nullptr);


	vkDestroyDescriptorSetLayout(m_logicalDevice, m_DSL_finalComposite, nullptr);
	//vkDestroyDescriptorSetLayout(m_logicalDevice, m_DSL_compute, nullptr);

	//vkDestroyPipeline(m_logicalDevice, m_postProcess_ToneMap_Pipeline, nullptr);
	//vkDestroyPipelineLayout(m_logicalDevice, m_postProcess_ToneMap_PipelineLayout, nullptr);
	//vkDestroyPipeline(m_logicalDevice, m_postProcess_TXAA_Pipeline, nullptr);
	//vkDestroyPipelineLayout(m_logicalDevice, m_postProcess_TXAA_PipelineLayout, nullptr);
}
void PipelineManager::recreate(VkExtent2D windowExtents, DescriptorSetLayouts& pipelineDescriptorSetLayouts)
{
	std::vector<VkDescriptorSetLayout>& compute_DSL = pipelineDescriptorSetLayouts.computeDSL;
	std::vector<VkDescriptorSetLayout>& finalComposite_DSL = pipelineDescriptorSetLayouts.finalCompositeDSL;
	std::vector<VkDescriptorSetLayout>& rasterization_DSL = pipelineDescriptorSetLayouts.geomDSL;

	m_compute_PL = VulkanPipelineCreation::createPipelineLayout(m_logicalDevice, compute_DSL, 0, nullptr);
	createComputePipeline(m_compute_P, m_compute_PL, "testComputeShader");

	createFinalCompositePipeline(finalComposite_DSL);
	createRasterizationRenderPipeline(rasterization_DSL);
}


void PipelineManager::createComputePipeline(VkPipeline& computePipeline, VkPipelineLayout computePipelineLayout, const std::string &shaderName)
{
	// -------- Create Shader Stages -------------
	VkShaderModule compShaderModule;
	VkPipelineShaderStageCreateInfo compShaderStageInfo;
	ShaderUtil::createComputeShaderStageInfo(compShaderStageInfo, shaderName, compShaderModule,m_logicalDevice);

	// -------- Create compute pipeline ---------
	VkComputePipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	pipelineInfo.stage = compShaderStageInfo;
	pipelineInfo.layout = computePipelineLayout;

	if (vkCreateComputePipelines(m_logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &computePipeline) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create pipeline");
	}

	// No need for the shader modules anymore, so we destory them!
	vkDestroyShaderModule(m_logicalDevice, compShaderModule, nullptr);
}
void PipelineManager::createFinalCompositePipeline(std::vector<VkDescriptorSetLayout>& compositeDSL)
{
	// -------- Create Pipeline Layout -------------
	m_finalComposite_PL = VulkanPipelineCreation::createPipelineLayout(m_logicalDevice, compositeDSL, 0, nullptr);

	// -------- Empty Vertex Input --------
	VkPipelineVertexInputStateCreateInfo vertexInput = VulkanPipelineStructures::vertexInputInfo(0, nullptr, 0, nullptr);
	
	// -------- Create Shader Stages -------------
	const uint32_t stageCount = 2;
	VkShaderModule vertShaderModule, fragShaderModule;	
	std::vector<VkPipelineShaderStageCreateInfo> shaderStages; shaderStages.resize(2);

	ShaderUtil::createShaderStageInfos_RenderToQuad(shaderStages, "finalComposite", vertShaderModule, fragShaderModule, m_logicalDevice);

	// -------- Create graphics pipeline ---------	
	createPipelineHelper(m_finalComposite_P, m_finalComposite_PL, m_renderPassManager->m_toDisplayRenderPass.renderPass, 0, 
		vertexInput, stageCount, shaderStages.data());
	
	// No need for the shader modules anymore, so we destory them!
	vkDestroyShaderModule(m_logicalDevice, vertShaderModule, nullptr);
	vkDestroyShaderModule(m_logicalDevice, fragShaderModule, nullptr);
}
void PipelineManager::createRasterizationRenderPipeline(std::vector<VkDescriptorSetLayout>& rasterizationDSL)
{
	// -------- Create Rasterization Layout -------------
	m_rasterization_PL = VulkanPipelineCreation::createPipelineLayout(m_logicalDevice, rasterizationDSL, 0, nullptr);

	// -------- Vertex Input --------
	// Vertex binding describes at which rate to load data from GPU memory 

	// All of our per-vertex data is packed together in 1 array so we only have one binding; 
	// The binding param specifies index of the binding in array of bindings
	const uint32_t numVertexAttributes = 4;
	VkVertexInputBindingDescription vertexInputBinding = 
		VulkanPipelineStructures::vertexInputBindingDesc(0, static_cast<uint32_t>(sizeof(Vertex)));

	// Input attribute bindings describe shader attribute locations and memory layouts		
	std::array<VkVertexInputAttributeDescription, numVertexAttributes> vertexInputAttributes = Vertex::getAttributeDescriptions();

	VkPipelineVertexInputStateCreateInfo vertexInput = 
		VulkanPipelineStructures::vertexInputInfo( 1, &vertexInputBinding, numVertexAttributes, vertexInputAttributes.data() );
	
	// -------- Create Shader Stages -------------
	const uint32_t stageCount = 2;
	VkShaderModule vertShaderModule, fragShaderModule;
	std::vector<VkPipelineShaderStageCreateInfo> shaderStages; shaderStages.resize(2);

	ShaderUtil::createShaderStageInfos(shaderStages, "geometryPlain", vertShaderModule, fragShaderModule, m_logicalDevice);

	// -------- Create graphics pipeline ---------	
	createPipelineHelper(m_rasterization_P, m_rasterization_PL, m_renderPassManager->m_geomRasterPass.renderPass, 0, 
		vertexInput, stageCount, shaderStages.data());

	// No need for the shader modules anymore, so we destory them!
	vkDestroyShaderModule(m_logicalDevice, vertShaderModule, nullptr);
	vkDestroyShaderModule(m_logicalDevice, fragShaderModule, nullptr);
}
void PipelineManager::createPostProcessPipelines(VkRenderPass& renderPass, unsigned int subpass)
{
	//VkShaderModule generic_vertShaderModule, toneMap_fragShaderModule;
	//std::vector<VkPipelineShaderStageCreateInfo> shaderStages; shaderStages.resize(2);

	//createVertShaderStageInfo(shaderStages[0], "postProcess_GenericVertShader", generic_vertShaderModule);
	//
	//VkGraphicsPipelineCreateInfo postProcessPipelineInfo = m_pipelineInfoPostProcess;
	//postProcessPipelineInfo.stageCount = shaderStages.size();

	//// --------- Tone Map Post Process Piepline ----------
	//createFragShaderStageInfo(shaderStages[1], "postProcess_ToneMap", toneMap_fragShaderModule);
	//postProcessPipelineInfo.pStages = shaderStages.data();
	//postProcessPipelineInfo.layout = m_postProcess_ToneMap_PipelineLayout;
	//postProcessPipelineInfo.renderPass;
	//postProcessPipelineInfo.subpass;

	//VulkanPipelineCreation::createGraphicsPipelines(m_logicalDevice, m_pipelineCachePostProcess, 1, &postProcessPipelineInfo, &m_postProcess_ToneMap_Pipeline);
	//vkDestroyShaderModule(m_logicalDevice, toneMap_fragShaderModule, nullptr);

	//// --------- TXAA Post Process Piepline ----------
	//createFragShaderStageInfo(shaderStages[1], "postProcess_TXAA", toneMap_fragShaderModule);
	//postProcessPipelineInfo.pStages = shaderStages.data();
	//postProcessPipelineInfo.layout = m_postProcess_TXAA_PipelineLayout;
	//postProcessPipelineInfo.renderPass;
	//postProcessPipelineInfo.subpass;

	//VulkanPipelineCreation::createGraphicsPipelines(m_logicalDevice, m_pipelineCachePostProcess, 1, &postProcessPipelineInfo, &m_postProcess_TXAA_Pipeline);
	//vkDestroyShaderModule(m_logicalDevice, TXAA_fragShaderModule, nullptr);
	//
	//// --------- cleanup ----------
	//vkDestroyShaderModule(m_logicalDevice, generic_vertShaderModule, nullptr);
}

void PipelineManager::createPipelineHelper(VkPipeline& pipeline, VkPipelineLayout& pipelineLayout, VkRenderPass& renderPass, uint32_t subpass,
		VkPipelineVertexInputStateCreateInfo& vertexInput, uint32_t stageCount, const VkPipelineShaderStageCreateInfo* stages)
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
	const VkExtent2D swapChainExtents = m_presentationObj->getVkExtent();
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
			VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_CLOCKWISE, VK_FALSE, 0.0f, 0.0f, 0.0f);

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
	VulkanPipelineCreation::createGraphicsPipelines(m_logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, &pipeline);
}


// Descriptor Sets
void PipelineManager::expandDescriptorPool(std::vector<VkDescriptorPoolSize>& poolSizes)
{
	// Final Composite pass writes to the swapchain image(s) so doesnt need a separate desciptor for that
	// It does need descriptors for any images it reads in

	// FINAL COMPOSITE
	poolSizes.push_back({ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, m_numSwapChainImages }); // compute image
	poolSizes.push_back({ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, m_numSwapChainImages }); // geomRenderPass image
}
void PipelineManager::createDescriptorSetLayouts()
{
	// FINAL COMPOSITE
	const uint32_t numImagesComposited = 2;
	VkDescriptorSetLayoutBinding computePassImageLB = { 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr };
	VkDescriptorSetLayoutBinding geomRenderedImageLB = { 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr };

	std::array<VkDescriptorSetLayoutBinding, numImagesComposited> finalCompositeLBs = { computePassImageLB, geomRenderedImageLB };
	DescriptorUtil::createDescriptorSetLayout(m_logicalDevice, m_DSL_finalComposite, numImagesComposited, finalCompositeLBs.data());
}
void PipelineManager::createAndWriteDescriptorSets(VkDescriptorPool descriptorPool, DescriptorSetDependencies& descSetDependencies)
{
	m_DS_finalComposite.resize(m_numSwapChainImages);
	const uint32_t numImagesComposited = 2;

	for (uint32_t i = 0; i < m_numSwapChainImages; i++)
	{
		// Final Composite
		{
			DescriptorUtil::createDescriptorSets(m_logicalDevice, descriptorPool, 1, &m_DSL_finalComposite, &m_DS_finalComposite[i]);
			
			VkDescriptorImageInfo computePassImageSetInfo = DescriptorUtil::createDescriptorImageInfo(
				descSetDependencies.computeImages[i]->getSampler(),
				descSetDependencies.computeImages[i]->getImageView(),
				descSetDependencies.computeImages[i]->getImageLayout());
			VkDescriptorImageInfo geomRenderedImageSetInfo = DescriptorUtil::createDescriptorImageInfo(
				descSetDependencies.geomRenderPassImageSet[i].sampler,
				descSetDependencies.geomRenderPassImageSet[i].imageView,
				descSetDependencies.geomRenderPassImageSet[i].imageLayout);

			std::array<VkWriteDescriptorSet, numImagesComposited> writeFinalCompositeSetInfo = {};
			writeFinalCompositeSetInfo[0] = DescriptorUtil::writeDescriptorSet(
				m_DS_finalComposite[i], 0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &computePassImageSetInfo);
			writeFinalCompositeSetInfo[1] = DescriptorUtil::writeDescriptorSet(
				m_DS_finalComposite[i], 1, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &geomRenderedImageSetInfo);

			vkUpdateDescriptorSets(m_logicalDevice, numImagesComposited, writeFinalCompositeSetInfo.data(), 0, nullptr);
		}
	}
}

VkDescriptorSet PipelineManager::getDescriptorSet(DSL_TYPE type, int index)
{
	switch (type)
	{
	case DSL_TYPE::FINAL_COMPOSITE:
		return m_DS_finalComposite[index];
		break;
	default:
		throw std::runtime_error("no such Descriptor Set Layout Type (DSL_TYPE) exists");
	}

	return nullptr;
}
VkDescriptorSetLayout PipelineManager::getDescriptorSetLayout(DSL_TYPE key)
{
	switch (key)
	{
	case DSL_TYPE::FINAL_COMPOSITE:
		return m_DSL_finalComposite;
		break;
	default:
		throw std::runtime_error("no such Descriptor Set Layout Type (DSL_TYPE) exists");
	}

	assert(("Did not find a Descriptor Set Layout to return", true));
	return nullptr;
}