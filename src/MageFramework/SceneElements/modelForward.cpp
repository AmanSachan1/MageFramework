#pragma once
#include <SceneElements/modelForward.h>

void vkPrimitive::writeToAndUpdateNodeDescriptorSet(vkMesh* mesh, uint32_t index, VkDevice& logicalDevice)
{
	std::vector<VkWriteDescriptorSet> writePrimitiveDescriptorSet = {
		DescriptorUtil::writeDescriptorSet(descriptorSets[index], 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &mesh->meshUniform[index].meshUB.descriptorInfo),
		DescriptorUtil::writeDescriptorSet(descriptorSets[index], 1, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &material->materialUB.descriptorInfo),
		DescriptorUtil::writeDescriptorSet(descriptorSets[index], 2, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &material->baseColorTexture->m_descriptorInfo)
	};

	if (material->activeTextures[1]) {
		writePrimitiveDescriptorSet.push_back(
			DescriptorUtil::writeDescriptorSet(descriptorSets[index], 3, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &material->normalTexture->m_descriptorInfo)
		);
	}
	if (material->activeTextures[2]) {
		writePrimitiveDescriptorSet.push_back(
			DescriptorUtil::writeDescriptorSet(descriptorSets[index], 4, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &material->metallicRoughnessTexture->m_descriptorInfo)
		);
	}
	if (material->activeTextures[3]) {
		writePrimitiveDescriptorSet.push_back(
			DescriptorUtil::writeDescriptorSet(descriptorSets[index], 5, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &material->emissiveTexture->m_descriptorInfo)
		);
	}
	if (material->activeTextures[4]) {
		writePrimitiveDescriptorSet.push_back(
			DescriptorUtil::writeDescriptorSet(descriptorSets[index], 6, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &material->occlusionTexture->m_descriptorInfo)
		);
	}

	vkUpdateDescriptorSets(logicalDevice, static_cast<uint32_t>(writePrimitiveDescriptorSet.size()), writePrimitiveDescriptorSet.data(), 0, nullptr);
}