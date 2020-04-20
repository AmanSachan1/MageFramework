#pragma once
#include <Vulkan/RendererBackend/vRendererBackend.h>

inline void VulkanRendererBackend::setupRayTracing(std::shared_ptr<Camera> camera, std::shared_ptr<Scene> scene)
{
	getRayTracingFunctionPointers();
	createScene_RayTracing(scene);
	createStorageImages(); // Create StorageImage for RayTraced Image

	std::vector<VkDescriptorSetLayout> l_rayTraceDSL = { m_DSL_rayTrace };
	createRayTracePipeline(l_rayTraceDSL);
	createShaderBindingTable();
	writeToAndUpdateDescriptorSets_rayTracing(camera, scene);
}
inline void VulkanRendererBackend::destroyRayTracing()
{	
	vkFreeMemory(m_logicalDevice, m_bottomLevelAS.memory, nullptr);
	vkDestroyAccelerationStructureNV(m_logicalDevice, m_bottomLevelAS.accelerationStructure, nullptr);
	vkFreeMemory(m_logicalDevice, m_topLevelAS.memory, nullptr);
	vkDestroyAccelerationStructureNV(m_logicalDevice, m_topLevelAS.accelerationStructure, nullptr);

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
			VkWriteDescriptorSetAccelerationStructureNV descriptorAccelerationStructureInfo{};
			descriptorAccelerationStructureInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_NV;
			descriptorAccelerationStructureInfo.accelerationStructureCount = 1;
			descriptorAccelerationStructureInfo.pAccelerationStructures = &m_topLevelAS.accelerationStructure;

			// The specialized acceleration structure descriptor has to be chained
			// Which is why we are using the pNext variable of the descriptor set
			VkWriteDescriptorSet accelerationStructureWrite = DescriptorUtil::writeDescriptorSet(
				m_DS_rayTrace[i], 0, 1, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV, &descriptorAccelerationStructureInfo);
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
			VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT);
	}
}
inline void VulkanRendererBackend::createScene_RayTracing(std::shared_ptr<Scene> scene)
{
	std::vector<VkGeometryNV> geometries;
	//for (auto& model : scene->m_modelMap)
	{
		std::shared_ptr<Model> model = scene->m_modelMap.begin()->second;
		geometries.push_back(model->m_rayTracingGeom);
	}
	// Create the Bottom Level Acceleration Structure containing all of the various geometry in the scene
	const uint32_t geometryCount = static_cast<uint32_t>(geometries.size());
	createBottomLevelAccelerationStructure(geometryCount, geometries.data());

	// Create the Top Level Acceleration Structure containing the geometry
	{
		// Single instance with a 3x4 transform matrix for the ray traced triangle
		mageVKBuffer instanceBuffer, scratchBuffer;
		glm::mat3x4 transform = {
			1.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
		};

		std::array<GeometryInstance, 2> geometryInstances{};
		// First geometry instance is used for the scene hit and miss shaders
		geometryInstances[0].transform = transform;
		geometryInstances[0].instanceId = 0;
		geometryInstances[0].mask = 0xff;
		geometryInstances[0].instanceOffset = 0;
		geometryInstances[0].flags = VK_GEOMETRY_INSTANCE_TRIANGLE_CULL_DISABLE_BIT_NV;
		geometryInstances[0].accelerationStructureHandle = m_bottomLevelAS.handle;
		// Second geometry instance is used for the shadow hit and miss shaders
		geometryInstances[1].transform = transform;
		geometryInstances[1].instanceId = 1;
		geometryInstances[1].mask = 0xff;
		geometryInstances[1].instanceOffset = 2;
		geometryInstances[1].flags = VK_GEOMETRY_INSTANCE_TRIANGLE_CULL_DISABLE_BIT_NV;
		geometryInstances[1].accelerationStructureHandle = m_bottomLevelAS.handle;

		BufferUtil::createMageBuffer(m_logicalDevice, m_physicalDevice, instanceBuffer,
			geometryInstances.size() * sizeof(GeometryInstance), geometryInstances.data(), VK_BUFFER_USAGE_RAY_TRACING_BIT_NV);

		createTopLevelAccelerationStructure();
		buildAccelerationStructures(scratchBuffer, instanceBuffer, static_cast<uint32_t>(geometries.size()), geometries.data());
	}
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
inline void VulkanRendererBackend::createBottomLevelAccelerationStructure(uint32_t geometryCount, const VkGeometryNV* geometries)
{
	VkAccelerationStructureInfoNV accelerationStructureInfo{};
	accelerationStructureInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
	accelerationStructureInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_NV;
	accelerationStructureInfo.instanceCount = 0;
	accelerationStructureInfo.geometryCount = geometryCount;
	accelerationStructureInfo.pGeometries = geometries;

	VkAccelerationStructureCreateInfoNV accelerationStructureCI{};
	accelerationStructureCI.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_NV;
	accelerationStructureCI.info = accelerationStructureInfo;
	VK_CHECK_RESULT(vkCreateAccelerationStructureNV(m_logicalDevice, &accelerationStructureCI, nullptr, &m_bottomLevelAS.accelerationStructure));

	VkAccelerationStructureMemoryRequirementsInfoNV memoryRequirementsInfo{};
	memoryRequirementsInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV;
	memoryRequirementsInfo.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_OBJECT_NV;
	memoryRequirementsInfo.accelerationStructure = m_bottomLevelAS.accelerationStructure;

	VkMemoryRequirements2 memoryRequirements2{};
	vkGetAccelerationStructureMemoryRequirementsNV(m_logicalDevice, &memoryRequirementsInfo, &memoryRequirements2);

	VkMemoryAllocateInfo memoryAllocateInfo = {};
	memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocateInfo.allocationSize = memoryRequirements2.memoryRequirements.size;
	memoryAllocateInfo.memoryTypeIndex = BufferUtil::findMemoryType(m_physicalDevice,
		memoryRequirements2.memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	VK_CHECK_RESULT(vkAllocateMemory(m_logicalDevice, &memoryAllocateInfo, nullptr, &m_bottomLevelAS.memory));

	VkBindAccelerationStructureMemoryInfoNV accelerationStructureMemoryInfo{};
	accelerationStructureMemoryInfo.sType = VK_STRUCTURE_TYPE_BIND_ACCELERATION_STRUCTURE_MEMORY_INFO_NV;
	accelerationStructureMemoryInfo.accelerationStructure = m_bottomLevelAS.accelerationStructure;
	accelerationStructureMemoryInfo.memory = m_bottomLevelAS.memory;
	VK_CHECK_RESULT(vkBindAccelerationStructureMemoryNV(m_logicalDevice, 1, &accelerationStructureMemoryInfo));

	VK_CHECK_RESULT(vkGetAccelerationStructureHandleNV(m_logicalDevice, m_bottomLevelAS.accelerationStructure, sizeof(uint64_t), &m_bottomLevelAS.handle));
}
inline void VulkanRendererBackend::createTopLevelAccelerationStructure()
{
	VkAccelerationStructureInfoNV accelerationStructureInfo{};
	accelerationStructureInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
	accelerationStructureInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_NV;
	accelerationStructureInfo.instanceCount = 1;
	accelerationStructureInfo.geometryCount = 0;

	VkAccelerationStructureCreateInfoNV accelerationStructureCI{};
	accelerationStructureCI.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_NV;
	accelerationStructureCI.info = accelerationStructureInfo;
	VK_CHECK_RESULT(vkCreateAccelerationStructureNV(m_logicalDevice, &accelerationStructureCI, nullptr, &m_topLevelAS.accelerationStructure));

	VkAccelerationStructureMemoryRequirementsInfoNV memoryRequirementsInfo{};
	memoryRequirementsInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV;
	memoryRequirementsInfo.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_OBJECT_NV;
	memoryRequirementsInfo.accelerationStructure = m_topLevelAS.accelerationStructure;

	VkMemoryRequirements2 memoryRequirements2{};
	vkGetAccelerationStructureMemoryRequirementsNV(m_logicalDevice, &memoryRequirementsInfo, &memoryRequirements2);

	VkMemoryAllocateInfo memoryAllocateInfo = {};
	memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocateInfo.allocationSize = memoryRequirements2.memoryRequirements.size;
	memoryAllocateInfo.memoryTypeIndex = BufferUtil::findMemoryType(m_physicalDevice, 
		memoryRequirements2.memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	VK_CHECK_RESULT(vkAllocateMemory(m_logicalDevice, &memoryAllocateInfo, nullptr, &m_topLevelAS.memory));

	VkBindAccelerationStructureMemoryInfoNV accelerationStructureMemoryInfo{};
	accelerationStructureMemoryInfo.sType = VK_STRUCTURE_TYPE_BIND_ACCELERATION_STRUCTURE_MEMORY_INFO_NV;
	accelerationStructureMemoryInfo.accelerationStructure = m_topLevelAS.accelerationStructure;
	accelerationStructureMemoryInfo.memory = m_topLevelAS.memory;
	VK_CHECK_RESULT(vkBindAccelerationStructureMemoryNV(m_logicalDevice, 1, &accelerationStructureMemoryInfo));

	VK_CHECK_RESULT(vkGetAccelerationStructureHandleNV(m_logicalDevice, m_topLevelAS.accelerationStructure, sizeof(uint64_t), &m_topLevelAS.handle));
}
inline void VulkanRendererBackend::buildAccelerationStructures(
mageVKBuffer& scratchBuffer, mageVKBuffer& instanceBuffer, 
	uint32_t geometryCount, const VkGeometryNV* geometries)
{
	// Building an Acceleration Structur requires some scratch space to store temporary information
	VkAccelerationStructureMemoryRequirementsInfoNV memoryRequirementsInfo{};
	memoryRequirementsInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV;
	memoryRequirementsInfo.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_BUILD_SCRATCH_NV;

	VkMemoryRequirements2 memReqBottomLevelAS;
	memoryRequirementsInfo.accelerationStructure = m_bottomLevelAS.accelerationStructure;
	vkGetAccelerationStructureMemoryRequirementsNV(m_logicalDevice, &memoryRequirementsInfo, &memReqBottomLevelAS);

	VkMemoryRequirements2 memReqTopLevelAS;
	memoryRequirementsInfo.accelerationStructure = m_topLevelAS.accelerationStructure;
	vkGetAccelerationStructureMemoryRequirementsNV(m_logicalDevice, &memoryRequirementsInfo, &memReqTopLevelAS);

	const VkDeviceSize scratchBufferSize = std::max(memReqBottomLevelAS.memoryRequirements.size, memReqTopLevelAS.memoryRequirements.size);
	BufferUtil::createMageBuffer(m_logicalDevice, m_physicalDevice, scratchBuffer, scratchBufferSize, nullptr,
		VK_BUFFER_USAGE_RAY_TRACING_BIT_NV, VK_SHARING_MODE_EXCLUSIVE, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	VkCommandBuffer cmdBuffer;
	{
		VulkanCommandUtil::allocateCommandBuffers(m_logicalDevice, m_graphicsCmdPool, 1, &cmdBuffer, VK_COMMAND_BUFFER_LEVEL_PRIMARY);

		// Begin Single Command Buffer
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		VK_CHECK_RESULT(vkBeginCommandBuffer(cmdBuffer, &beginInfo));
	}


	VkMemoryBarrier memoryBarrier = {};
	memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
	memoryBarrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV;
	memoryBarrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV;

	// Build Bottom Level Acceleration Structure (BLAS)
	{
		VkAccelerationStructureInfoNV buildInfo{};
		buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
		buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_NV;
		buildInfo.geometryCount = geometryCount;
		buildInfo.pGeometries = geometries;
		
		VkBuffer l_instanceData = VK_NULL_HANDLE;
		VkDeviceSize l_instanceOffset = 0;
		VkBool32 l_update = VK_FALSE;
		VkAccelerationStructureNV l_dst = m_bottomLevelAS.accelerationStructure;
		VkAccelerationStructureNV l_src = VK_NULL_HANDLE;
		VkDeviceSize l_scratchOffset = 0;

		vkCmdBuildAccelerationStructureNV(
			cmdBuffer,
			&buildInfo,
			l_instanceData,
			l_instanceOffset,
			l_update,
			l_dst,
			l_src,
			scratchBuffer.buffer,
			l_scratchOffset);

		vkCmdPipelineBarrier(cmdBuffer,
			VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV,
			VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV,
			0, 1, &memoryBarrier, 0, 0, 0, 0);
	}

	// Build Top Level Acceleration Structure (TLAS)
	{
		VkAccelerationStructureInfoNV buildInfo{};
		buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
		buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_NV;
		buildInfo.geometryCount = 0;
		buildInfo.pGeometries = 0;
		buildInfo.instanceCount = 1;
		
		VkDeviceSize l_instanceOffset = 0;
		VkBool32 l_update = VK_FALSE;
		VkAccelerationStructureNV l_dst = m_topLevelAS.accelerationStructure;
		VkAccelerationStructureNV l_src = VK_NULL_HANDLE;
		VkDeviceSize l_scratchOffset = 0;

		vkCmdBuildAccelerationStructureNV(
			cmdBuffer,
			&buildInfo,
			instanceBuffer.buffer,
			l_instanceOffset,
			l_update,
			l_dst,
			l_src,
			scratchBuffer.buffer,
			l_scratchOffset);

		vkCmdPipelineBarrier(cmdBuffer,
			VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV,
			VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV,
			0, 1, &memoryBarrier, 0, 0, 0, 0);
	}

	VulkanCommandUtil::endAndSubmitSingleTimeCommand(m_logicalDevice, m_graphicsQueue, m_graphicsCmdPool, cmdBuffer);

	scratchBuffer.destroy(m_logicalDevice);
	instanceBuffer.destroy(m_logicalDevice);
}


inline void VulkanRendererBackend::createShaderBindingTable()
{
	const uint32_t sbtSize = m_rayTracingProperties.shaderGroupHandleSize * NUM_SHADER_GROUPS;

	// Create buffer for the shader binding table, without mapping data
	BufferUtil::createMageBuffer(m_logicalDevice, m_physicalDevice, m_shaderBindingTable,
		sbtSize, nullptr, VK_BUFFER_USAGE_RAY_TRACING_BIT_NV,
		VK_SHARING_MODE_EXCLUSIVE, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

	// Create Mapped Data for Shader Binding Table Buffer
	{
		m_shaderBindingTable.map(m_logicalDevice);

		uint8_t* data = static_cast<uint8_t*>(m_shaderBindingTable.mappedData);
		// Get shader identifiers
		auto shaderHandleStorage = new uint8_t[sbtSize];
		vkGetRayTracingShaderGroupHandlesNV(m_logicalDevice, m_rayTrace_P, 0, NUM_SHADER_GROUPS, sbtSize, shaderHandleStorage);
		// Copy the shader identifiers to the shader binding table
		data += copyShaderIdentifier(data, shaderHandleStorage, INDEX_RAYGEN);
		data += copyShaderIdentifier(data, shaderHandleStorage, INDEX_MISS);
		data += copyShaderIdentifier(data, shaderHandleStorage, INDEX_SHADOW_MISS);
		data += copyShaderIdentifier(data, shaderHandleStorage, INDEX_CLOSEST_HIT);
		data += copyShaderIdentifier(data, shaderHandleStorage, INDEX_SHADOW_HIT);

		m_shaderBindingTable.unmap(m_logicalDevice);
	}
}
inline VkDeviceSize VulkanRendererBackend::copyShaderIdentifier(uint8_t* data, const uint8_t* shaderHandleStorage, uint32_t groupIndex)
{
	const uint32_t shaderGroupHandleSize = m_rayTracingProperties.shaderGroupHandleSize;
	memcpy(data, shaderHandleStorage + groupIndex * shaderGroupHandleSize, shaderGroupHandleSize);
	data += shaderGroupHandleSize;
	return shaderGroupHandleSize;
}