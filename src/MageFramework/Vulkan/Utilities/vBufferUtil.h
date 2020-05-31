#pragma once
#include <global.h>
#include <Vulkan/Utilities/vCommandUtil.h>

struct mageVKBuffer
{
	VkBuffer buffer = VK_NULL_HANDLE;
	VkDeviceMemory memory = VK_NULL_HANDLE;
	VkDeviceSize bufferSize = 0;

	void* mappedData = nullptr;
	VkDescriptorBufferInfo descriptorInfo;

	VkBufferUsageFlags usageFlags;
	VkSharingMode sharingMode;
	VkMemoryPropertyFlags memoryProperties;

	void setDescriptorInfo(VkDeviceSize offset = 0)
	{
		descriptorInfo.buffer = buffer;
		descriptorInfo.offset = offset;
		descriptorInfo.range = bufferSize;
	}

	void map(VkDevice& logicalDevice)
	{
		VkDeviceSize offset = 0;
		VkMemoryMapFlags flags = 0;
		vkMapMemory(logicalDevice, memory, offset, bufferSize, flags, &mappedData);
	}
	void copyDataToMappedBuffer(void* data)
	{
		memcpy(mappedData, data, bufferSize);
	}
	void unmap(VkDevice& logicalDevice)
	{
		vkUnmapMemory(logicalDevice, memory);
	}
	void destroy(VkDevice& logicalDevice)
	{
		vkDestroyBuffer(logicalDevice, buffer, nullptr);
		vkFreeMemory(logicalDevice, memory, nullptr);
	}
};

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
				&& (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
			{
				return i;
			}
		}

		throw std::runtime_error("failed to find suitable memory type!");
	}

	inline void allocateMemory(VkPhysicalDevice& pDevice, VkDevice& logicalDevice,
		VkBuffer& buffer, VkDeviceMemory& bufferMemory, VkMemoryPropertyFlags properties)
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
			throw std::runtime_error("failed to allocate buffer memory!");
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

	inline void createBuffer(VkDevice& logicalDevice, VkPhysicalDevice& pDevice,
		VkBuffer& buffer, VkDeviceMemory& bufferMemory, VkDeviceSize bufferSize,
		VkBufferUsageFlags allowedUsage, VkSharingMode sharingMode, VkMemoryPropertyFlags properties)
	{
		VkBufferCreateInfo bufferCreationInfo = bufferCreateInfo(bufferSize, allowedUsage, sharingMode);

		VK_CHECK_RESULT( vkCreateBuffer(logicalDevice, &bufferCreationInfo, nullptr, &buffer) );

		BufferUtil::allocateMemory(pDevice, logicalDevice, buffer, bufferMemory, properties);

		// The fourth parameter in vkBindBufferMemory is the offset within the region of memory. 
		// Since this memory is allocated specifically for this the vertex buffer, the offset is simply 0.
		// If the offset is non-zero, then it is required to be divisible by memRequirements.alignment.
		vkBindBufferMemory(logicalDevice, buffer, bufferMemory, 0);
	}

	inline void copyBuffer(VkDevice& logicalDevice, VkQueue& queue, VkCommandPool& cmdPool,
		VkBuffer& srcBuffer, VkBuffer& dstBuffer, VkDeviceSize srcOffset, VkDeviceSize dstOffset, VkDeviceSize size)
	{
		// Memory transfer operations are executed using command buffers, just like drawing commands.
		// Therefore we must first allocate a temporary command buffer. 
		// Creating a separate command pool for these kinds of short - lived buffers, could be better for performance because
		// it may be possible to apply memory allocation optimizations. You should use the VK_COMMAND_POOL_CREATE_TRANSIENT_BIT flag 
		// during command pool generation in that case.

		VkCommandBuffer commandBuffer;
		VulkanCommandUtil::beginSingleTimeCommand(logicalDevice, cmdPool, commandBuffer);
		VulkanCommandUtil::copyCommandBuffer(logicalDevice, commandBuffer, srcBuffer, dstBuffer, srcOffset, dstOffset, size);
		VulkanCommandUtil::endAndSubmitSingleTimeCommand(logicalDevice, queue, cmdPool, commandBuffer);
	}

	inline void createStagingBuffer(VkDevice& logicalDevice, VkPhysicalDevice& pDevice, const void* src,
		VkBuffer& stagingBuffer, VkDeviceMemory& stagingBufferMemory, VkDeviceSize stagingBufferSize)
	{
		BufferUtil::createBuffer(logicalDevice, pDevice, stagingBuffer, stagingBufferMemory, stagingBufferSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_SHARING_MODE_EXCLUSIVE,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		void* mappedData;
		vkMapMemory(logicalDevice, stagingBufferMemory, 0, stagingBufferSize, 0, &mappedData);
		memcpy(mappedData, src, static_cast<size_t>(stagingBufferSize));
		vkUnmapMemory(logicalDevice, stagingBufferMemory);
	}


	inline void createMageBuffer(VkDevice& logicalDevice, VkPhysicalDevice& pDevice, 
		mageVKBuffer& mageVKbuffer, VkDeviceSize bufferSize, 
		void* mappedData, VkBufferUsageFlags allowedUsage, 
		VkSharingMode sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
	{
		mageVKbuffer.bufferSize = bufferSize;
		mageVKbuffer.usageFlags = allowedUsage;
		mageVKbuffer.sharingMode = sharingMode;
		mageVKbuffer.memoryProperties = properties;
		createBuffer(logicalDevice, pDevice,
			mageVKbuffer.buffer, mageVKbuffer.memory, mageVKbuffer.bufferSize,
			mageVKbuffer.usageFlags, mageVKbuffer.sharingMode, mageVKbuffer.memoryProperties);

		if (mappedData)
		{
			mageVKbuffer.map(logicalDevice);
			mageVKbuffer.copyDataToMappedBuffer(mappedData);
			mageVKbuffer.unmap(logicalDevice);
		}

		mageVKbuffer.setDescriptorInfo();
	}

	inline void createMageVertexBuffer(VkDevice& logicalDevice, VkPhysicalDevice& pDevice, VkQueue& graphicsQueue, VkCommandPool& cmdPool,
		mageVKBuffer& vertexBuffer, VkDeviceSize bufferSize, Vertex* sourceVertexData,
		VkBufferUsageFlags allowedUsage, VkSharingMode sharingMode = VK_SHARING_MODE_EXCLUSIVE)
	{
		// ----- Create Staging Buffer -----
		// We use a staging buffer as an intermediate buffer used to copy the data associated with the buffer to a more optimal memory location
		// which is usually not accessible by the CPU (which is why we need to use the intermediate staging buffer).
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		createStagingBuffer(logicalDevice, pDevice, static_cast<void*>(sourceVertexData), stagingBuffer, stagingBufferMemory, bufferSize);

		// ----- Create Vertex Buffer -----
		// The vertexBuffer is now allocated from a memory type that is device local, which generally means that we're 
		// not able to use vkMapMemory. However, we can copy data from the stagingBuffer to the vertexBuffer. We have to 
		// indicate that we intend to do that by specifying the transfer source flag for the stagingBuffer and 
		// the transfer destination flag for the vertexBuffer, along with the vertex buffer usage flag.
		createMageBuffer(logicalDevice, pDevice, vertexBuffer, bufferSize, nullptr,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | allowedUsage,
			sharingMode, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		// ----- Copy Staging Buffer into the Vertex Buffer -----
		copyBuffer(logicalDevice, graphicsQueue, cmdPool, stagingBuffer, vertexBuffer.buffer, 0, 0, vertexBuffer.bufferSize);

		// ----- Free Staging Buffer and its memory -----
		vkDestroyBuffer(logicalDevice, stagingBuffer, nullptr);
		vkFreeMemory(logicalDevice, stagingBufferMemory, nullptr);
	}
	
	inline void createMageIndexBuffer(VkDevice& logicalDevice, VkPhysicalDevice& pDevice, VkQueue& graphicsQueue, VkCommandPool& cmdPool,
		mageVKBuffer& indexBuffer, VkDeviceSize bufferSize, uint32_t* sourceIndexData, 
		VkBufferUsageFlags allowedUsage, VkSharingMode sharingMode = VK_SHARING_MODE_EXCLUSIVE)
	{
		// Very similar to createVertexBuffer Function above
		// ----- Create Staging Buffer -----
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		createStagingBuffer(logicalDevice, pDevice, static_cast<void*>(sourceIndexData), stagingBuffer, stagingBufferMemory, bufferSize);
		
		// ----- Create Index Buffer -----
		createMageBuffer(logicalDevice, pDevice, indexBuffer, bufferSize, nullptr,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | allowedUsage,
			sharingMode, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		// ----- Copy Staging Buffer into the Index Buffer -----
		copyBuffer(logicalDevice, graphicsQueue, cmdPool, stagingBuffer, indexBuffer.buffer, 0, 0, indexBuffer.bufferSize);

		// ----- Free Staging Buffer and its memory -----
		vkDestroyBuffer(logicalDevice, stagingBuffer, nullptr);
		vkFreeMemory(logicalDevice, stagingBufferMemory, nullptr);
	}
}