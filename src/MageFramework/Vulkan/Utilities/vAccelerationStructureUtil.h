#pragma once
#include <global.h>
#include <Vulkan/Utilities/vBufferUtil.h>

// Disable warning C26495: Uninitialized variables (type.6); In various structs below
#pragma warning( disable : 26495 )

namespace AccelerationUtil
{
    inline void createAccelerationStructure(VkDevice& logicalDevice, vAccelerationStructure& accelerationStructure,
        PFN_vkCreateAccelerationStructureNV& vkCreateASNV)
    {
        VkAccelerationStructureCreateInfoNV l_accelerationStructureCI{};
        l_accelerationStructureCI.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_NV;
        l_accelerationStructureCI.pNext = nullptr;
        l_accelerationStructureCI.compactedSize = 0;
        l_accelerationStructureCI.info = accelerationStructure.m_buildInfo;
        
        //VK_CHECK_RESULT(vkCreateAccelerationStructureNV(logicalDevice, 
        //    &l_accelerationStructureCI, nullptr, &accelerationStructure.accelerationStructure));

        VK_CHECK_RESULT(vkCreateASNV(logicalDevice,
            &l_accelerationStructureCI, nullptr, &accelerationStructure.m_accelerationStructure));
    }

    inline void allocateMemory_AS(VkDevice& logicalDevice, VkPhysicalDevice pDevice, vAccelerationStructure& accelerationStructure,
        PFN_vkGetAccelerationStructureMemoryRequirementsNV& vkGetASMemoryRequirementsNV)
    {
        VkAccelerationStructureMemoryRequirementsInfoNV l_memoryRequirementsInfo{};
        l_memoryRequirementsInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV;
        l_memoryRequirementsInfo.pNext = nullptr;
        l_memoryRequirementsInfo.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_OBJECT_NV;
        l_memoryRequirementsInfo.accelerationStructure = accelerationStructure.m_accelerationStructure;

        VkMemoryRequirements2 l_memoryRequirements2{};
        //vkGetAccelerationStructureMemoryRequirementsNV(logicalDevice, &l_memoryRequirementsInfo, &l_memoryRequirements2);
        vkGetASMemoryRequirementsNV(logicalDevice, &l_memoryRequirementsInfo, &l_memoryRequirements2);


        VkMemoryAllocateInfo memoryAllocateInfo = {};
        memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        memoryAllocateInfo.pNext = nullptr;
        memoryAllocateInfo.allocationSize = l_memoryRequirements2.memoryRequirements.size;
        memoryAllocateInfo.memoryTypeIndex = BufferUtil::findMemoryType(
            pDevice, l_memoryRequirements2.memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        VK_CHECK_RESULT(vkAllocateMemory(logicalDevice, &memoryAllocateInfo, nullptr, &accelerationStructure.m_memory));
    }

    inline void bindAccelerationStructure(VkDevice& logicalDevice, vAccelerationStructure& accelerationStructure,
        PFN_vkBindAccelerationStructureMemoryNV& vkBindASMemoryNV)
    {
        VkBindAccelerationStructureMemoryInfoNV l_accelerationStructureMemoryInfo{};
        l_accelerationStructureMemoryInfo.sType = VK_STRUCTURE_TYPE_BIND_ACCELERATION_STRUCTURE_MEMORY_INFO_NV;
        l_accelerationStructureMemoryInfo.pNext = nullptr;
        l_accelerationStructureMemoryInfo.accelerationStructure = accelerationStructure.m_accelerationStructure;
        l_accelerationStructureMemoryInfo.memory = accelerationStructure.m_memory;
        l_accelerationStructureMemoryInfo.memoryOffset = 0;
        l_accelerationStructureMemoryInfo.deviceIndexCount = 0;
        l_accelerationStructureMemoryInfo.pDeviceIndices = nullptr;

        //VK_CHECK_RESULT(vkBindAccelerationStructureMemoryNV(logicalDevice, 1, &l_accelerationStructureMemoryInfo));
        VK_CHECK_RESULT(vkBindASMemoryNV(logicalDevice, 1, &l_accelerationStructureMemoryInfo));
    }

    inline void createAndGetHandle_AS(VkDevice& logicalDevice, VkPhysicalDevice pDevice, 
        vAccelerationStructure& accelerationStructure, 
        PFN_vkCreateAccelerationStructureNV& vkCreateASNV,
        PFN_vkGetAccelerationStructureMemoryRequirementsNV& vkGetASMemoryRequirementsNV,
        PFN_vkBindAccelerationStructureMemoryNV& vkBindASMemoryNV,
        PFN_vkGetAccelerationStructureHandleNV& vkGetASHandleNV)
    {
        AccelerationUtil::createAccelerationStructure(logicalDevice, accelerationStructure, vkCreateASNV);
        AccelerationUtil::allocateMemory_AS(logicalDevice, pDevice, accelerationStructure, vkGetASMemoryRequirementsNV);
        AccelerationUtil::bindAccelerationStructure(logicalDevice, accelerationStructure, vkBindASMemoryNV);

        ////VK_CHECK_RESULT(vkGetAccelerationStructureHandleNV(logicalDevice, 
        ////    accelerationStructure.accelerationStructure, sizeof(uint64_t), &accelerationStructure.handle));

        VK_CHECK_RESULT(vkGetASHandleNV(logicalDevice,
            accelerationStructure.m_accelerationStructure, sizeof(uint64_t), &accelerationStructure.m_handle));
    }
}