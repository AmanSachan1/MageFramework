#pragma once
#include <Vulkan/RendererBackend/vRendererBackend.h>

inline void VulkanRendererBackend::cleanupRayTracing()
{
	for (auto rayTracedImage : m_rayTracedImages)
	{
		rayTracedImage.reset();
	}
}
inline void VulkanRendererBackend::destroyRayTracing()
{	
	vkFreeMemory(m_logicalDevice, m_topLevelAS.m_memory, nullptr);
	vkDestroyAccelerationStructureNV(m_logicalDevice, m_topLevelAS.m_accelerationStructure, nullptr);

	m_shaderBindingTable.destroy(m_logicalDevice);
}


inline void VulkanRendererBackend::createDescriptors_rayTracing(VkDescriptorPool descriptorPool)
{
	// Descriptor Set Layouts
	{
		VkDescriptorSetLayoutBinding accelerationStructureLayoutBinding =
		{ 0, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV, 1,
			VK_SHADER_STAGE_RAYGEN_BIT_NV | VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV, nullptr };
		VkDescriptorSetLayoutBinding resultImageLayoutBinding =
		{ 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1,
				VK_SHADER_STAGE_RAYGEN_BIT_NV, nullptr };
		VkDescriptorSetLayoutBinding cameraUniform =
		{ 2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1,
				VK_SHADER_STAGE_RAYGEN_BIT_NV | VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV | VK_SHADER_STAGE_MISS_BIT_NV, nullptr };
		VkDescriptorSetLayoutBinding lightsUniform =
		{ 3, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1,
			VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV | VK_SHADER_STAGE_MISS_BIT_NV, nullptr };
		VkDescriptorSetLayoutBinding vertexBufferStorage =
		{ 4, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1,
			VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV, nullptr };
		VkDescriptorSetLayoutBinding indexBufferStorage =
		{ 5, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1,
			VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV, nullptr };
		std::vector<VkDescriptorSetLayoutBinding> rayGenBindings({
			accelerationStructureLayoutBinding,
			resultImageLayoutBinding,
			cameraUniform, lightsUniform,
			vertexBufferStorage, indexBufferStorage });

		DescriptorUtil::createDescriptorSetLayout(m_logicalDevice, m_DSL_rayTrace, static_cast<uint32_t>(rayGenBindings.size()), rayGenBindings.data());
	}

	// Descriptor Sets
	m_DS_rayTrace.resize(3);
	for (uint32_t i = 0; i < m_numSwapChainImages; i++)
	{
		DescriptorUtil::createDescriptorSets(m_logicalDevice, descriptorPool, 1, &m_DSL_rayTrace, &m_DS_rayTrace[i]);
	}
}
inline void VulkanRendererBackend::writeToAndUpdateDescriptorSets_rayTracing(std::shared_ptr<Camera> camera, std::shared_ptr<Scene> scene)
{
	for (uint32_t i = 0; i < m_numSwapChainImages; i++)
	{
		std::vector<VkWriteDescriptorSet> writeRayTracingDescriptorSets;

		{
			// The specialized acceleration structure descriptor has to be chained
			// Which is why we are using the pNext variable of the descriptor set
			VkWriteDescriptorSet accelerationStructureWrite = DescriptorUtil::writeDescriptorSet(
				m_DS_rayTrace[i], 0, 1, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV, &m_topLevelAS.m_descriptorSetInfo);
			writeRayTracingDescriptorSets.push_back(accelerationStructureWrite);
		}
		{
			VkWriteDescriptorSet rayTracedImageWrite = DescriptorUtil::writeDescriptorSet(
					m_DS_rayTrace[i], 1, 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, &m_rayTracedImages[i]->m_descriptorInfo);
			writeRayTracingDescriptorSets.push_back(rayTracedImageWrite);
		}
		{
			VkWriteDescriptorSet writecameraSetInfo = DescriptorUtil::writeDescriptorSet(
				m_DS_rayTrace[i], 2, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &camera->getCameraBufferInfo(i));
			writeRayTracingDescriptorSets.push_back(writecameraSetInfo);
		}
		{
			VkWriteDescriptorSet writeLightSetInfo = DescriptorUtil::writeDescriptorSet(
				m_DS_rayTrace[i], 3, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &scene->m_lightsUniform[i].lightBuffer.descriptorInfo);
			writeRayTracingDescriptorSets.push_back(writeLightSetInfo);
		}
		std::shared_ptr<Model> model = scene->m_modelMap.begin()->second;
		VkDescriptorBufferInfo vertexBufferDescriptorInfo{ model->m_vertices.vertexBuffer.buffer, 0, VK_WHOLE_SIZE };
		VkDescriptorBufferInfo indexBufferDescriptorInfo{ model->m_indices.indexBuffer.buffer, 0, VK_WHOLE_SIZE };
		{
			VkWriteDescriptorSet vertexBufferWrite = DescriptorUtil::writeDescriptorSet(
					m_DS_rayTrace[i], 4, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &vertexBufferDescriptorInfo);
			writeRayTracingDescriptorSets.push_back(vertexBufferWrite);
		}
		{
			VkWriteDescriptorSet indexBufferWrite =	DescriptorUtil::writeDescriptorSet(
					m_DS_rayTrace[i], 5, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &indexBufferDescriptorInfo);
			writeRayTracingDescriptorSets.push_back(indexBufferWrite);
		}

		const uint32_t writeCount = static_cast<uint32_t>(writeRayTracingDescriptorSets.size());
		vkUpdateDescriptorSets(m_logicalDevice, writeCount, writeRayTracingDescriptorSets.data(), 0, nullptr);
	}
}


inline void VulkanRendererBackend::createStorageImages()
{
	m_rayTracedImages.resize(m_numSwapChainImages);
	for (uint32_t i = 0; i < m_numSwapChainImages; i++)
	{
		m_rayTracedImages[i] = std::make_shared<Texture2D>(
			m_logicalDevice, m_physicalDevice, m_graphicsQueue, m_graphicsCmdPool, m_lowResolutionRenderFormat);
		m_rayTracedImages[i]->createEmptyTexture(
			m_windowExtents.width, m_windowExtents.height, 1, 1, 
			m_graphicsQueue, m_graphicsCmdPool, false,
			VK_SAMPLER_ADDRESS_MODE_REPEAT,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
	}
}

inline void VulkanRendererBackend::createAndBuildAccelerationStructures(std::shared_ptr<Scene> scene)
{
	// Create the Bottom Level Acceleration Structure containing all of the various geometry in the scene
	createAllBottomLevelAccelerationStructures(scene);

	createGeometryInstancesForTLAS(scene);
	createTopLevelAccelerationStructure(false);

	buildAccelerationStructures(scene, m_topLevelAS.m_geometryInstances);
}


inline void VulkanRendererBackend::getRayTracingFunctionPointers()
{
	VkDevice logicalDevice = m_vulkanManager->getLogicalDevice();
	VkPhysicalDevice pDevice = m_vulkanManager->getPhysicalDevice();

	// Query the ray tracing properties of the current implementation, we will need them later on
	m_rayTracingProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PROPERTIES_NV;
	VkPhysicalDeviceProperties2 deviceProps2{};
	deviceProps2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
	deviceProps2.pNext = &m_rayTracingProperties;
	vkGetPhysicalDeviceProperties2(pDevice, &deviceProps2);

	// Get VK_NV_ray_tracing related function pointers
	vkCreateAccelerationStructureNV = reinterpret_cast<PFN_vkCreateAccelerationStructureNV>(vkGetDeviceProcAddr(logicalDevice, "vkCreateAccelerationStructureNV"));
	vkDestroyAccelerationStructureNV = reinterpret_cast<PFN_vkDestroyAccelerationStructureNV>(vkGetDeviceProcAddr(logicalDevice, "vkDestroyAccelerationStructureNV"));
	vkBindAccelerationStructureMemoryNV = reinterpret_cast<PFN_vkBindAccelerationStructureMemoryNV>(vkGetDeviceProcAddr(logicalDevice, "vkBindAccelerationStructureMemoryNV"));
	vkGetAccelerationStructureHandleNV = reinterpret_cast<PFN_vkGetAccelerationStructureHandleNV>(vkGetDeviceProcAddr(logicalDevice, "vkGetAccelerationStructureHandleNV"));
	vkGetAccelerationStructureMemoryRequirementsNV = reinterpret_cast<PFN_vkGetAccelerationStructureMemoryRequirementsNV>(vkGetDeviceProcAddr(logicalDevice, "vkGetAccelerationStructureMemoryRequirementsNV"));
	vkCmdBuildAccelerationStructureNV = reinterpret_cast<PFN_vkCmdBuildAccelerationStructureNV>(vkGetDeviceProcAddr(logicalDevice, "vkCmdBuildAccelerationStructureNV"));
	vkCreateRayTracingPipelinesNV = reinterpret_cast<PFN_vkCreateRayTracingPipelinesNV>(vkGetDeviceProcAddr(logicalDevice, "vkCreateRayTracingPipelinesNV"));
	vkGetRayTracingShaderGroupHandlesNV = reinterpret_cast<PFN_vkGetRayTracingShaderGroupHandlesNV>(vkGetDeviceProcAddr(logicalDevice, "vkGetRayTracingShaderGroupHandlesNV"));
	vkCmdTraceRaysNV = reinterpret_cast<PFN_vkCmdTraceRaysNV>(vkGetDeviceProcAddr(logicalDevice, "vkCmdTraceRaysNV"));
}
inline void VulkanRendererBackend::createAllBottomLevelAccelerationStructures(std::shared_ptr<Scene> scene)
{
	for (auto& modelElement : scene->m_modelMap)
	{
		auto model = modelElement.second;

		// Create the Bottom Level Acceleration Structure containing all of the various geometry in the scene
		model->m_bottomLevelAS.initInfo(VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_NV, 0, 0, 1, &(model->m_rayTracingGeom));

		AccelerationUtil::createAndGetHandle_AS(m_logicalDevice, m_physicalDevice, model->m_bottomLevelAS,
			vkCreateAccelerationStructureNV, vkGetAccelerationStructureMemoryRequirementsNV,
			vkBindAccelerationStructureMemoryNV, vkGetAccelerationStructureHandleNV);
	}
}
inline void VulkanRendererBackend::createGeometryInstancesForTLAS(std::shared_ptr<Scene> scene)
{
	// Hit group 0: triangles
	// Hit group 2: shadows
	uint32_t l_instanceId = 0;

	for (auto& modelElement : scene->m_modelMap)
	{
		std::shared_ptr<Model> model = modelElement.second;

		// First geometry instance is used for the scene hit and miss shaders
		GeometryInstance tempGeomInstance1;
		tempGeomInstance1.transform = model->getMatrix_3x4();
		tempGeomInstance1.instanceId = l_instanceId;
		tempGeomInstance1.mask = 0xff;
		tempGeomInstance1.instanceOffset = 0;
		tempGeomInstance1.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_CULL_DISABLE_BIT_NV;
		tempGeomInstance1.accelerationStructureHandle = model->m_bottomLevelAS.m_handle;
		m_topLevelAS.m_geometryInstances.push_back(tempGeomInstance1);
		// Second geometry instance is used for the shadow hit and miss shaders
		GeometryInstance tempGeomInstance2;
		tempGeomInstance2.transform = model->getMatrix_3x4();
		tempGeomInstance2.instanceId = l_instanceId + 1;
		tempGeomInstance2.mask = 0xff;
		tempGeomInstance2.instanceOffset = 2;
		tempGeomInstance2.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_CULL_DISABLE_BIT_NV;
		tempGeomInstance2.accelerationStructureHandle = model->m_bottomLevelAS.m_handle;
		m_topLevelAS.m_geometryInstances.push_back(tempGeomInstance2);

		l_instanceId += 2;
	}
}
inline void VulkanRendererBackend::createTopLevelAccelerationStructure(bool allowUpdate)
{
	const VkBuildAccelerationStructureFlagsNV flags = allowUpdate? 
		VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_NV
		: VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_NV;

	m_topLevelAS.initInfo(VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_NV, flags, 
		static_cast<uint32_t>(m_topLevelAS.m_geometryInstances.size()), 0, nullptr);

	AccelerationUtil::createAndGetHandle_AS(m_logicalDevice, m_physicalDevice, m_topLevelAS, 
		vkCreateAccelerationStructureNV, vkGetAccelerationStructureMemoryRequirementsNV,
		vkBindAccelerationStructureMemoryNV, vkGetAccelerationStructureHandleNV);
}
inline void VulkanRendererBackend::buildAccelerationStructures(
	std::shared_ptr<Scene> scene, std::vector<GeometryInstance>& geometryInstances)
{
	mageVKBuffer scratchBuffer, instanceBuffer;
	VkDeviceSize scratchBufferSize = getScratchBufferSize(scene);

	BufferUtil::createMageBuffer(m_logicalDevice, m_physicalDevice, scratchBuffer, scratchBufferSize, nullptr,
		VK_BUFFER_USAGE_RAY_TRACING_BIT_NV, VK_SHARING_MODE_EXCLUSIVE, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	BufferUtil::createMageBuffer(m_logicalDevice, m_physicalDevice, instanceBuffer,
		geometryInstances.size() * sizeof(GeometryInstance), geometryInstances.data(), VK_BUFFER_USAGE_RAY_TRACING_BIT_NV);

	VkCommandBuffer cmdBuffer;
	VulkanCommandUtil::beginSingleTimeCommand(m_logicalDevice, m_graphicsCmdPool, cmdBuffer);

	// Build All Bottom Level Acceleration Structure (BLAS)
	for (auto& modelElement : scene->m_modelMap)
	{
		auto model = modelElement.second;
		
		VkBuffer l_instanceData = VK_NULL_HANDLE;
		VkDeviceSize l_instanceOffset = 0;
		VkBool32 l_update = VK_FALSE;
		VkAccelerationStructureNV l_dst = model->m_bottomLevelAS.m_accelerationStructure;
		VkAccelerationStructureNV l_src = VK_NULL_HANDLE;
		VkDeviceSize l_scratchOffset = 0;

		vkCmdBuildAccelerationStructureNV(
			cmdBuffer,
			&(model->m_bottomLevelAS.m_buildInfo),
			l_instanceData,
			l_instanceOffset,
			l_update,
			l_dst,
			l_src,
			scratchBuffer.buffer,
			l_scratchOffset);

		vAccelerationStructure::addMemoryBarrier(cmdBuffer);
	}

	// Build Top Level Acceleration Structure (TLAS)
	{
		VkDeviceSize l_instanceOffset = 0;
		VkBool32 l_update = VK_FALSE;
		VkAccelerationStructureNV l_dst = m_topLevelAS.m_accelerationStructure;
		VkAccelerationStructureNV l_src = VK_NULL_HANDLE;
		VkDeviceSize l_scratchOffset = 0;

		vkCmdBuildAccelerationStructureNV(
			cmdBuffer,
			&(m_topLevelAS.m_buildInfo),
			instanceBuffer.buffer,
			l_instanceOffset,
			l_update,
			l_dst,
			l_src,
			scratchBuffer.buffer,
			l_scratchOffset);

		vAccelerationStructure::addMemoryBarrier(cmdBuffer);
	}

	VulkanCommandUtil::endAndSubmitSingleTimeCommand(m_logicalDevice, m_graphicsQueue, m_graphicsCmdPool, cmdBuffer);

	scratchBuffer.destroy(m_logicalDevice);
	instanceBuffer.destroy(m_logicalDevice);
}


inline void VulkanRendererBackend::createShaderBindingTable()
{
	m_sbtSize = m_rayTracingProperties.shaderGroupHandleSize * NUM_SHADER_GROUPS;

	// Create buffer for the shader binding table, without mapping data
	BufferUtil::createMageBuffer(m_logicalDevice, m_physicalDevice, m_shaderBindingTable,
		m_sbtSize, nullptr, VK_BUFFER_USAGE_RAY_TRACING_BIT_NV,
		VK_SHARING_MODE_EXCLUSIVE, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

	mapShaderBindingTable();
}
inline void VulkanRendererBackend::mapShaderBindingTable()
{
	// Create Mapped Data for Shader Binding Table Buffer
	m_shaderBindingTable.map(m_logicalDevice);

	uint8_t* data = static_cast<uint8_t*>(m_shaderBindingTable.mappedData);
	// Get shader identifiers
	auto shaderHandleStorage = new uint8_t[m_sbtSize];
	vkGetRayTracingShaderGroupHandlesNV(m_logicalDevice, m_rayTrace_P, 0, NUM_SHADER_GROUPS, m_sbtSize, shaderHandleStorage);
	// Copy the shader identifiers to the shader binding table
	data += copyShaderIdentifier(data, shaderHandleStorage, INDEX_RAYGEN);
	data += copyShaderIdentifier(data, shaderHandleStorage, INDEX_MISS);
	data += copyShaderIdentifier(data, shaderHandleStorage, INDEX_SHADOW_MISS);
	data += copyShaderIdentifier(data, shaderHandleStorage, INDEX_CLOSEST_HIT);
	data += copyShaderIdentifier(data, shaderHandleStorage, INDEX_SHADOW_HIT);

	m_shaderBindingTable.unmap(m_logicalDevice);
}
inline VkDeviceSize VulkanRendererBackend::copyShaderIdentifier(uint8_t* data, const uint8_t* shaderHandleStorage, uint32_t groupIndex)
{
	const uint32_t shaderGroupHandleSize = m_rayTracingProperties.shaderGroupHandleSize;
	memcpy(data, shaderHandleStorage + groupIndex * shaderGroupHandleSize, shaderGroupHandleSize);
	data += shaderGroupHandleSize;
	return shaderGroupHandleSize;
}
inline VkDeviceSize VulkanRendererBackend::getScratchBufferSize(std::shared_ptr<Scene> scene)
{
	// Building an Acceleration Structur requires some scratch space to store temporary information
	VkAccelerationStructureMemoryRequirementsInfoNV memoryRequirementsInfo{};
	memoryRequirementsInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV;
	memoryRequirementsInfo.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_BUILD_SCRATCH_NV;

	VkMemoryRequirements2 memReqTopLevelAS;
	memoryRequirementsInfo.accelerationStructure = m_topLevelAS.m_accelerationStructure;
	vkGetAccelerationStructureMemoryRequirementsNV(m_logicalDevice, &memoryRequirementsInfo, &memReqTopLevelAS);

	VkDeviceSize scratchBufferSize = memReqTopLevelAS.memoryRequirements.size;

	for (auto& modelElement : scene->m_modelMap)
	{
		auto model = modelElement.second;

		VkMemoryRequirements2 memReqBottomLevelAS;
		memoryRequirementsInfo.accelerationStructure = model->m_bottomLevelAS.m_accelerationStructure;
		vkGetAccelerationStructureMemoryRequirementsNV(m_logicalDevice, &memoryRequirementsInfo, &memReqBottomLevelAS);

		scratchBufferSize = std::max(memReqBottomLevelAS.memoryRequirements.size, scratchBufferSize);
	}
	
	return scratchBufferSize;
}