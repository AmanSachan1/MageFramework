#pragma once
#include <global.h>

namespace BufferUtil
{
	inline uint32_t findMemoryType(VkPhysicalDevice& pDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties)
	{
		// The VkMemoryPropertyFlags define special features of the memory, like .
		// -- VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT ---> being able to map the memory so we can write to it from the CPU
		// -- VK_MEMORY_PROPERTY_HOST_COHERENT_BIT property ---> ensures that the mapped memory always matches the contents of the allocated memory

		// The VkPhysicalDeviceMemoryProperties structure has two arrays memoryTypes and memoryHeaps.
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(pDevice, &memProperties);

		// Find a memory type that is suitable for the buffer itself:
		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) 
		{
			if (typeFilter & (1 << i) 
				&& (memProperties.memoryTypes[i].propertyFlags & properties) == properties )
			{
				return i;
			}
		}

		throw std::runtime_error("failed to find suitable memory type!");
	}

	inline void allocateMemory(VkPhysicalDevice& pDevice, VkDevice& logicalDevice, 
		VkBuffer& buffer, VkDeviceMemory& bufferMemory,	VkMemoryPropertyFlags properties)
	{
		// The VkMemoryRequirements struct has three fields:
		// - size: The size of the required amount of memory in bytes, may differ from bufferInfo.size.
		// - alignment : The offset in bytes where the buffer begins in the allocated region of memory, depends on bufferInfo.usage and bufferInfo.flags.
		// - memoryTypeBits : Bit field of the memory types that are suitable for the buffer.
		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(logicalDevice, buffer, &memRequirements);

		VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = findMemoryType(pDevice, memRequirements.memoryTypeBits, properties);

		if (vkAllocateMemory(logicalDevice, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to allocate vertex buffer memory!");
		}
	}

	inline VkBufferCreateInfo bufferCreateInfo(VkDeviceSize size, VkBufferUsageFlags usage, VkSharingMode sharingMode)
	{
		VkBufferCreateInfo l_bufferCreationInfo = {};
		l_bufferCreationInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		l_bufferCreationInfo.size = size;
		l_bufferCreationInfo.usage = usage;

		//Buffers can be owned by a specific queue family or be shared between multiple at the same time. 
		//The sharingMode flag determines if the buffer is being shared or not.
		l_bufferCreationInfo.sharingMode = sharingMode;

		return l_bufferCreationInfo;
	}

	inline void createBuffer(VkPhysicalDevice& pDevice, VkDevice& logicalDevice, 
		VkBuffer& buffer, VkDeviceMemory& bufferMemory, VkDeviceSize bufferSize,
		VkBufferUsageFlags allowedUsage, VkSharingMode sharingMode, VkMemoryPropertyFlags properties )
	{
		VkBufferCreateInfo bufferCreationInfo = bufferCreateInfo(bufferSize, allowedUsage, sharingMode);

		if (vkCreateBuffer(logicalDevice, &bufferCreationInfo, nullptr, &buffer) != VK_SUCCESS) 
		{
			throw std::runtime_error("failed to create buffer!");
		}

		BufferUtil::allocateMemory(pDevice, logicalDevice, buffer, bufferMemory, properties);

		// The fourth parameter in vkBindBufferMemory is the offset within the region of memory. 
		// Since this memory is allocated specifically for this the vertex buffer, the offset is simply 0.
		// If the offset is non-zero, then it is required to be divisible by memRequirements.alignment.
		vkBindBufferMemory(logicalDevice, buffer, bufferMemory, 0);
	}


}