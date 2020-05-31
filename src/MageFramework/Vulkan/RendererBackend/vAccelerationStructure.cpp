#include "Vulkan/RendererBackend/vAccelerationStructure.h"
#include <Vulkan/Utilities/vAccelerationStructureUtil.h>

void vBLAS::generateBLAS(VkCommandBuffer commandBuffer,
	mageVKBuffer& scratchBuffer, VkDeviceSize scratchOffset,
	bool updateOnly) const
{
	const VkAccelerationStructureNV previousStructure = updateOnly ? m_accelerationStructure : nullptr;
}

void vTLAS::generateTLAS(VkCommandBuffer commandBuffer,
	mageVKBuffer& scratchBuffer, VkDeviceSize scratchOffset,
	bool updateOnly) const
{

}