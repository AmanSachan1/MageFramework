#pragma once
#include <global.h>
#include <Vulkan/Utilities/vRenderUtil.h>
#include <SceneElements/texture.h>

namespace DescriptorUtil
{
	inline VkDescriptorSetLayoutBinding createDescriptorSetLayoutBinding(uint32_t binding,
		VkDescriptorType descriptorType, uint32_t descriptorCount,
		VkShaderStageFlags stageFlags, const VkSampler* pImmutableSampler)
	{
		VkDescriptorSetLayoutBinding l_UBOLayoutBinding = {};
		// Binding --> used in shader
		// Descriptor Type --> type of descriptor
		// Descriptor Count --> Shader variable can represent an array of UBO's, descriptorCount specifies number of values in the array
		// Stage Flags --> which shader you're referencing this descriptor from 
		// pImmutableSamplers --> for image sampling related descriptors
		l_UBOLayoutBinding.binding = binding;
		l_UBOLayoutBinding.descriptorType = descriptorType;
		l_UBOLayoutBinding.descriptorCount = descriptorCount;
		l_UBOLayoutBinding.stageFlags = stageFlags;
		l_UBOLayoutBinding.pImmutableSamplers = pImmutableSampler;

		return l_UBOLayoutBinding;
	}

	inline void createDescriptorSetLayout(VkDevice& logicalDevice, VkDescriptorSetLayout& descriptorSetLayout,
		uint32_t bindingCount, VkDescriptorSetLayoutBinding* data)
	{
		VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {};
		descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		descriptorSetLayoutCreateInfo.pNext = nullptr;
		descriptorSetLayoutCreateInfo.bindingCount = bindingCount;
		descriptorSetLayoutCreateInfo.pBindings = data;

		if (vkCreateDescriptorSetLayout(logicalDevice, &descriptorSetLayoutCreateInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create descriptor set layout!");
		}
	}

	inline VkDescriptorPoolSize descriptorPoolSize(VkDescriptorType type, uint32_t descriptorCount)
	{
		VkDescriptorPoolSize  l_poolSize = {};
		l_poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		l_poolSize.descriptorCount = descriptorCount;

		return l_poolSize;
	}

	inline void createDescriptorPool(VkDevice& logicalDevice, uint32_t maxSets, uint32_t poolSizeCount, VkDescriptorPoolSize* data, VkDescriptorPool& descriptorPool)
	{
		VkDescriptorPoolCreateInfo descriptorPoolInfo = {};
		descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		descriptorPoolInfo.pNext = nullptr;
		descriptorPoolInfo.flags = 0; // Change if you're going to modify the descriptor set after its creation
		descriptorPoolInfo.poolSizeCount = poolSizeCount;
		descriptorPoolInfo.pPoolSizes = data;
		descriptorPoolInfo.maxSets = maxSets; // max number of descriptor sets allowed

		if (vkCreateDescriptorPool(logicalDevice, &descriptorPoolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create descriptor pool");
		}
	}

	inline void createDescriptorSets(VkDevice& logicalDevice, VkDescriptorPool descriptorPool, uint32_t descriptorSetCount,
		VkDescriptorSetLayout* descriptorSetLayouts, VkDescriptorSet* descriptorSetData)
	{
		VkDescriptorSetAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = descriptorPool;
		allocInfo.descriptorSetCount = descriptorSetCount;
		allocInfo.pSetLayouts = descriptorSetLayouts;

		if (vkAllocateDescriptorSets(logicalDevice, &allocInfo, descriptorSetData) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to allocate descriptor sets!");
		}
	}

	inline VkDescriptorBufferInfo createDescriptorBufferInfo(VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range)
	{
		VkDescriptorBufferInfo l_descriptorBufferInfo = {};
		l_descriptorBufferInfo.buffer = buffer;
		l_descriptorBufferInfo.offset = offset;
		l_descriptorBufferInfo.range = range;
		return l_descriptorBufferInfo;
	}

	inline VkDescriptorImageInfo createDescriptorImageInfo(VkSampler& sampler, VkImageView& imageView, VkImageLayout imageLayout)
	{
		VkDescriptorImageInfo l_descriptorImageInfo = {};
		l_descriptorImageInfo.sampler = sampler;
		l_descriptorImageInfo.imageView = imageView;
		l_descriptorImageInfo.imageLayout = imageLayout;
		return l_descriptorImageInfo;
	}

	inline VkDescriptorImageInfo createDescriptorImageInfo(std::shared_ptr<Texture2D>& texture)
	{
		return createDescriptorImageInfo(texture->m_sampler, texture->m_imageView, texture->m_imageLayout);
	}

	inline VkWriteDescriptorSet writeDescriptorSet(
		VkDescriptorSet dstSet, uint32_t dstBinding, uint32_t descriptorCount, VkDescriptorType descriptorType,
		const VkDescriptorBufferInfo* pBufferInfo, uint32_t dstArrayElement = 0)
	{
		VkWriteDescriptorSet l_descriptorWrite = {};
		l_descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		l_descriptorWrite.dstSet = dstSet;
		l_descriptorWrite.dstBinding = dstBinding;
		l_descriptorWrite.dstArrayElement = dstArrayElement;
		l_descriptorWrite.descriptorCount = descriptorCount;
		l_descriptorWrite.descriptorType = descriptorType;
		l_descriptorWrite.pImageInfo = nullptr;
		l_descriptorWrite.pBufferInfo = pBufferInfo;
		l_descriptorWrite.pTexelBufferView = nullptr;
		return l_descriptorWrite;
	}

	inline VkWriteDescriptorSet writeDescriptorSet(
		VkDescriptorSet dstSet, uint32_t dstBinding, uint32_t descriptorCount, VkDescriptorType descriptorType,
		const VkDescriptorImageInfo* pImageInfo = nullptr, uint32_t dstArrayElement = 0)
	{
		VkWriteDescriptorSet l_descriptorWrite = {};
		l_descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		l_descriptorWrite.dstSet = dstSet;
		l_descriptorWrite.dstBinding = dstBinding;
		l_descriptorWrite.dstArrayElement = dstArrayElement;
		l_descriptorWrite.descriptorCount = descriptorCount;
		l_descriptorWrite.descriptorType = descriptorType;
		l_descriptorWrite.pImageInfo = pImageInfo;
		return l_descriptorWrite;
	}

	inline PostProcessDescriptors createImgSamplerDescriptor(int serialIndex, std::string descriptorName, uint32_t numFrames,
		VkDescriptorPool descriptorPool, VkDevice& logicalDevice)
	{
		PostProcessDescriptors ImgSamplerDescriptor;
		ImgSamplerDescriptor.descriptorName = descriptorName;
		ImgSamplerDescriptor.serialIndex = serialIndex;
		ImgSamplerDescriptor.postProcess_DSs.resize(numFrames);

		VkDescriptorSetLayoutBinding ImgSamplerBinding = { 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr };
		DescriptorUtil::createDescriptorSetLayout(logicalDevice, ImgSamplerDescriptor.postProcess_DSL, 1, &ImgSamplerBinding);

		for (uint32_t i = 0; i < numFrames; i++)
		{
			DescriptorUtil::createDescriptorSets(logicalDevice, descriptorPool, 1,
				&ImgSamplerDescriptor.postProcess_DSL, &ImgSamplerDescriptor.postProcess_DSs[i]);
		}
		return ImgSamplerDescriptor;
	}

	inline void writeToImageSamplerDescriptor(std::vector<VkDescriptorSet>& descriptorsSets,
		std::vector<VkDescriptorImageInfo>& descriptorInfo, VkSampler& sampler,
		uint32_t numFrames, VkDevice& logicalDevice)
	{
		for (uint32_t i = 0; i < numFrames; i++)
		{
			VkDescriptorImageInfo ImgSamplerDescriptorInfo = DescriptorUtil::createDescriptorImageInfo(
					sampler, descriptorInfo[i].imageView, descriptorInfo[i].imageLayout);

			VkWriteDescriptorSet writeToneMapSetInfo = DescriptorUtil::writeDescriptorSet(
				descriptorsSets[i], 0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &ImgSamplerDescriptorInfo);

			vkUpdateDescriptorSets(logicalDevice, 1, &writeToneMapSetInfo, 0, nullptr);
		}
	}

	inline void writeToImageSamplerDescriptor(std::vector<VkDescriptorSet>& descriptorsSets,
		std::vector<FrameBufferAttachment>& fba, VkImageLayout imageLayout, VkSampler& sampler,
		uint32_t numFrames, VkDevice& logicalDevice)
	{
		for (uint32_t i = 0; i < numFrames; i++)
		{
			VkDescriptorImageInfo ImgSamplerDescriptorInfo = 
				DescriptorUtil::createDescriptorImageInfo(sampler, fba[i].view, imageLayout);

			VkWriteDescriptorSet writeToneMapSetInfo = DescriptorUtil::writeDescriptorSet(
				descriptorsSets[i], 0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &ImgSamplerDescriptorInfo);

			vkUpdateDescriptorSets(logicalDevice, 1, &writeToneMapSetInfo, 0, nullptr);
		}
	}
}