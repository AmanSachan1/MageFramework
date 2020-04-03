#pragma once
#include <Vulkan/RendererBackend/vRendererBackend.h>

inline void VulkanRendererBackend::cleanupPipelines()
{
	// Compute Pipeline
	vkDestroyPipeline(m_logicalDevice, m_compute_P, nullptr);
	vkDestroyPipelineLayout(m_logicalDevice, m_compute_PL, nullptr);
	// Composite Compute Onto Raster Pipeline
	vkDestroyPipeline(m_logicalDevice, m_compositeComputeOntoRaster_P, nullptr);
	vkDestroyPipelineLayout(m_logicalDevice, m_compositeComputeOntoRaster_PL, nullptr);
	// Rasterization Pipeline
	vkDestroyPipeline(m_logicalDevice, m_rasterization_P, nullptr);
	vkDestroyPipelineLayout(m_logicalDevice, m_rasterization_PL, nullptr);
}
inline void VulkanRendererBackend::createComputePipeline(VkPipeline& computePipeline, VkPipelineLayout computePipelineLayout, const std::string &shaderName)
{
	// -------- Create Shader Stages -------------
	VkShaderModule compShaderModule;
	VkPipelineShaderStageCreateInfo compShaderStageInfo;
	ShaderUtil::createComputeShaderStageInfo(compShaderStageInfo, shaderName, compShaderModule, m_logicalDevice);

	VulkanPipelineCreation::createComputePipeline(m_logicalDevice, compShaderStageInfo, computePipeline, computePipelineLayout);

	// No need for the shader modules anymore, so we destory them!
	vkDestroyShaderModule(m_logicalDevice, compShaderModule, nullptr);
}
inline void VulkanRendererBackend::createCompositeComputeOntoRasterPipeline(std::vector<VkDescriptorSetLayout>& compositeDSL)
{
	// -------- Create Pipeline Layout -------------
	m_compositeComputeOntoRaster_PL = VulkanPipelineCreation::createPipelineLayout(m_logicalDevice, compositeDSL, 0, nullptr);

	// -------- Empty Vertex Input --------
	VkPipelineVertexInputStateCreateInfo vertexInput = VulkanPipelineStructures::vertexInputInfo(0, nullptr, 0, nullptr);

	// -------- Create Shader Stages -------------
	const uint32_t stageCount = 2;
	VkShaderModule vertShaderModule, fragShaderModule;
	std::vector<VkPipelineShaderStageCreateInfo> shaderStages; shaderStages.resize(2);

	ShaderUtil::createShaderStageInfos_RenderToQuad(shaderStages, "compositeComputeOntoRaster", vertShaderModule, fragShaderModule, m_logicalDevice);

	// -------- Create graphics pipeline ---------	
	VulkanPipelineCreation::createGraphicsPipeline(m_logicalDevice,
		m_compositeComputeOntoRaster_P, m_compositeComputeOntoRaster_PL,
		m_compositeComputeOntoRasterRPI.renderPass, 0,
		stageCount, shaderStages.data(), vertexInput, m_vulkanManager->getSwapChainVkExtent());

	// No need for the shader modules anymore, so we destory them!
	vkDestroyShaderModule(m_logicalDevice, vertShaderModule, nullptr);
	vkDestroyShaderModule(m_logicalDevice, fragShaderModule, nullptr);
}
inline void VulkanRendererBackend::createRasterizationRenderPipeline(std::vector<VkDescriptorSetLayout>& rasterizationDSL)
{
	// -------- Create Rasterization Layout -------------
	m_rasterization_PL = VulkanPipelineCreation::createPipelineLayout(m_logicalDevice, rasterizationDSL, 0, nullptr);

	// -------- Vertex Input --------
	// Vertex binding describes at which rate to load data from GPU memory 

	// All of our per-vertex data is packed together in 1 array so we only have one binding; 
	// The binding param specifies index of the binding in array of bindings
	const uint32_t numVertexAttributes = 3;
	VkVertexInputBindingDescription vertexInputBinding =
		VulkanPipelineStructures::vertexInputBindingDesc(0, static_cast<uint32_t>(sizeof(Vertex)));

	// Input attribute bindings describe shader attribute locations and memory layouts		
	std::array<VkVertexInputAttributeDescription, numVertexAttributes> vertexInputAttributes = Vertex::getAttributeDescriptions();

	VkPipelineVertexInputStateCreateInfo vertexInput =
		VulkanPipelineStructures::vertexInputInfo(1, &vertexInputBinding, numVertexAttributes, vertexInputAttributes.data());

	// -------- Create Shader Stages -------------
	const uint32_t stageCount = 2;
	VkShaderModule vertShaderModule, fragShaderModule;
	std::vector<VkPipelineShaderStageCreateInfo> shaderStages; shaderStages.resize(2);

	ShaderUtil::createShaderStageInfos(shaderStages, "geometryPlain", vertShaderModule, fragShaderModule, m_logicalDevice);

	// -------- Create graphics pipeline ---------	
	VulkanPipelineCreation::createGraphicsPipeline(m_logicalDevice,
		m_rasterization_P, m_rasterization_PL,
		m_rasterRPI.renderPass, 0,
		stageCount, shaderStages.data(), vertexInput, m_vulkanManager->getSwapChainVkExtent());

	// No need for the shader modules anymore, so we destory them!
	vkDestroyShaderModule(m_logicalDevice, vertShaderModule, nullptr);
	vkDestroyShaderModule(m_logicalDevice, fragShaderModule, nullptr);
}
