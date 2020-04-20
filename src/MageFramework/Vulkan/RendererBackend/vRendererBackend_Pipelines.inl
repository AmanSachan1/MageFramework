#pragma once
#include <Vulkan/RendererBackend/vRendererBackend.h>

inline void VulkanRendererBackend::cleanupPipelines()
{
	if (m_rendererOptions.renderType == RENDER_TYPE::RASTERIZATION)
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
	if (m_rendererOptions.renderType == RENDER_TYPE::RAYTRACE)
	{
		// Ray Tracing Pipeline
		vkDestroyPipeline(m_logicalDevice, m_rayTrace_P, nullptr);
		vkDestroyPipelineLayout(m_logicalDevice, m_rayTrace_PL, nullptr);
	}
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
inline void VulkanRendererBackend::createRayTracePipeline(std::vector<VkDescriptorSetLayout>& rayTraceDSL)
{
	m_rayTrace_PL = VulkanPipelineCreation::createPipelineLayout(m_logicalDevice, rayTraceDSL, 0, nullptr);

	const uint32_t shaderIndexRaygen = 0;
	const uint32_t shaderIndexMiss = 1;
	const uint32_t shaderIndexShadowMiss = 2;
	const uint32_t shaderIndexClosestHit = 3;
	const uint32_t shaderIndexShadowClosestHit = 4;

	std::array<VkPipelineShaderStageCreateInfo, 5> rayTraceShaderStageInfos;
	ShaderUtil::createRayTracingShaderStageInfo(
		rayTraceShaderStageInfos[shaderIndexRaygen], "ray_generation.rgen", VK_SHADER_STAGE_RAYGEN_BIT_NV, m_logicalDevice);
	ShaderUtil::createRayTracingShaderStageInfo(
		rayTraceShaderStageInfos[shaderIndexMiss], "ray_miss.rmiss", VK_SHADER_STAGE_MISS_BIT_NV, m_logicalDevice);
	ShaderUtil::createRayTracingShaderStageInfo(
		rayTraceShaderStageInfos[shaderIndexShadowMiss], "shadows.rmiss", VK_SHADER_STAGE_MISS_BIT_NV, m_logicalDevice);
	ShaderUtil::createRayTracingShaderStageInfo(
		rayTraceShaderStageInfos[shaderIndexClosestHit], "ray_closestHit.rchit", VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV, m_logicalDevice);
	ShaderUtil::createRayTracingShaderStageInfo(
		rayTraceShaderStageInfos[shaderIndexShadowClosestHit], "shadows_closestHit.rchit", VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV, m_logicalDevice);

	// Pass recursion depth for reflections to ray generation shader via specialization constant
	VkSpecializationMapEntry specializationMapEntry = { 0, 0, sizeof(uint32_t) };
	VkSpecializationInfo specializationInfo = { 1, &specializationMapEntry, sizeof(MAX_RECURSION_DEPTH), &MAX_RECURSION_DEPTH };
	rayTraceShaderStageInfos[shaderIndexRaygen].pSpecializationInfo = &specializationInfo;

	// Setup ray tracing shader groups
	std::array<VkRayTracingShaderGroupCreateInfoNV, NUM_SHADER_GROUPS> groups{};
	for (auto& group : groups)
	{
		// Init all groups with some default values
		group.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_NV;
		group.generalShader = VK_SHADER_UNUSED_NV;
		group.closestHitShader = VK_SHADER_UNUSED_NV;
		group.anyHitShader = VK_SHADER_UNUSED_NV;
		group.intersectionShader = VK_SHADER_UNUSED_NV;
	}

	// Links shaders and types to ray tracing shader groups
	// Ray generation shader group
	groups[INDEX_RAYGEN].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_NV;
	groups[INDEX_RAYGEN].generalShader = shaderIndexRaygen;
	// Scene miss shader group
	groups[INDEX_MISS].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_NV;
	groups[INDEX_MISS].generalShader = shaderIndexMiss;
	// Shadow miss shader group 
	groups[INDEX_SHADOW_MISS].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_NV;
	groups[INDEX_SHADOW_MISS].generalShader = shaderIndexShadowMiss;
	// Scene closest hit shader group
	groups[INDEX_CLOSEST_HIT].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_NV;
	groups[INDEX_CLOSEST_HIT].generalShader = VK_SHADER_UNUSED_NV;
	groups[INDEX_CLOSEST_HIT].closestHitShader = shaderIndexClosestHit;
	// Shadow closest hit shader group
	groups[INDEX_SHADOW_HIT].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_NV;
	groups[INDEX_SHADOW_HIT].generalShader = VK_SHADER_UNUSED_NV;
	// Reuse shadow miss shader
	groups[INDEX_SHADOW_HIT].closestHitShader = shaderIndexShadowClosestHit;

	VkRayTracingPipelineCreateInfoNV rayPipelineInfo{};
	rayPipelineInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_NV;
	rayPipelineInfo.stageCount = static_cast<uint32_t>(rayTraceShaderStageInfos.size());
	rayPipelineInfo.pStages = rayTraceShaderStageInfos.data();
	rayPipelineInfo.groupCount = static_cast<uint32_t>(groups.size());
	rayPipelineInfo.pGroups = groups.data();
	rayPipelineInfo.maxRecursionDepth = MAX_RECURSION_DEPTH;
	rayPipelineInfo.layout = m_rayTrace_PL;
	VK_CHECK_RESULT(vkCreateRayTracingPipelinesNV(m_logicalDevice, VK_NULL_HANDLE, 1, &rayPipelineInfo, nullptr, &m_rayTrace_P));
}