#pragma once
#include <global.h>
#include <Vulkan/Utilities/vBufferUtil.h>
#include <SceneElements/modelForward.h>

// Ray tracing acceleration structure
class vAccelerationStructure
{
public:
	void initInfo(VkAccelerationStructureTypeNV l_type, VkBuildAccelerationStructureFlagsNV l_flags,
		uint32_t l_instanceCount, uint32_t l_geometryCount, const VkGeometryNV* l_geometries)
	{
		m_buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
		m_buildInfo.pNext = nullptr;
		m_buildInfo.type = l_type;
		m_buildInfo.flags = l_flags;
		m_buildInfo.instanceCount = l_instanceCount;
		m_buildInfo.geometryCount = l_geometryCount;
		m_buildInfo.pGeometries = l_geometries;

		setDescriptorSetInfo();
	}

	void setDescriptorSetInfo()
	{
		m_descriptorSetInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_NV;
		m_descriptorSetInfo.pNext = nullptr;
		m_descriptorSetInfo.accelerationStructureCount = 1;
		m_descriptorSetInfo.pAccelerationStructures = &m_accelerationStructure;
	}

	static void addMemoryBarrier(VkCommandBuffer& commandBuffer)
	{
		VkMemoryBarrier memoryBarrier = {};
		memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
		memoryBarrier.pNext = nullptr;
		memoryBarrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV;
		memoryBarrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV;

		vkCmdPipelineBarrier(commandBuffer,
			VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV,
			VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV,
			0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
	}

public:
	VkAccelerationStructureNV m_accelerationStructure;
	VkDeviceMemory m_memory;
	uint64_t m_handle;

	VkAccelerationStructureInfoNV m_buildInfo;
	VkWriteDescriptorSetAccelerationStructureNV m_descriptorSetInfo;
};

class vBLAS : public vAccelerationStructure
{
public:
	void generateBLAS(VkCommandBuffer commandBuffer,
		mageVKBuffer& scratchBuffer, VkDeviceSize scratchOffset,
		bool updateOnly) const;
	
	static VkGeometryNV createRayTraceGeometry(Vertices ModelVertices, Indices ModelIndices,
		uint32_t vertexOffset, uint32_t vertexCount,
		uint32_t indexOffset, uint32_t indexCount,
		bool isOpaque)
	{
		VkGeometryNV geometry = {};
		geometry.sType = VK_STRUCTURE_TYPE_GEOMETRY_NV;
		geometry.pNext = nullptr;
		geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_NV;

		geometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_GEOMETRY_TRIANGLES_NV;
		geometry.geometry.triangles.pNext = nullptr;
		geometry.geometry.triangles.vertexData = ModelVertices.vertexBuffer.buffer;
		geometry.geometry.triangles.vertexOffset = vertexOffset;
		geometry.geometry.triangles.vertexCount = vertexCount;
		geometry.geometry.triangles.vertexStride = sizeof(Vertex);
		geometry.geometry.triangles.vertexFormat = Vertex::getVertexPositionFormat_RayTracing();
		geometry.geometry.triangles.indexData = ModelIndices.indexBuffer.buffer;
		geometry.geometry.triangles.indexOffset = indexOffset;
		geometry.geometry.triangles.indexCount = indexCount;
		geometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
		geometry.geometry.triangles.transformData = nullptr;
		geometry.geometry.triangles.transformOffset = 0;

		geometry.geometry.aabbs = {};
		geometry.geometry.aabbs.sType = { VK_STRUCTURE_TYPE_GEOMETRY_AABB_NV };

		geometry.flags = isOpaque ? VK_GEOMETRY_OPAQUE_BIT_NV : 0;

		return geometry;
	}

private:
	std::vector<VkGeometryNV> m_geometries;
};

class vTLAS : public vAccelerationStructure
{
public:
	void generateTLAS(VkCommandBuffer commandBuffer,
		mageVKBuffer& scratchBuffer, VkDeviceSize scratchOffset,
		bool updateOnly) const;

public:
	std::vector<GeometryInstance> m_geometryInstances;
};