#pragma once
#include <bitset>
#include <global.h>
#include <Vulkan/Utilities/vBufferUtil.h>
#include <Vulkan/Utilities/vDescriptorUtil.h>
#include <SceneElements/texture.h>

enum AlphaMode { ALPHAMODE_OPAQUE, ALPHAMODE_MASK, ALPHAMODE_BLEND };

struct Vertices
{
	std::vector<Vertex> vertexArray;
	VkBuffer buffer;
	VkDeviceMemory bufferMemory;
	VkDeviceSize bufferSize;
};

struct Indices
{
	uint32_t numIndices;
	std::vector<uint32_t> indexArray;
	VkBuffer buffer;
	VkDeviceMemory bufferMemory;
	VkDeviceSize bufferSize;
};

struct MeshUniformBlock
{
	glm::mat4 modelMat;
};

struct MeshUniform
{
	UniformBufferObject ubo;
	MeshUniformBlock uniformBlock;
	VkDescriptorBufferInfo descriptorInfo;
};

struct MaterialUniformBlock
{
	int alphaMode;
	float alphaCutoff;
	float metallicFactor;
	float roughnessFactor;
	glm::vec4 baseColorFactor;

	MaterialUniformBlock() 
		: alphaMode(ALPHAMODE_OPAQUE), alphaCutoff (1.0f),
		metallicFactor (1.0f), roughnessFactor (1.0f), baseColorFactor (glm::vec4(1.0f, 1.0f, 1.0f,1.0f))
	{};
};

// Every primitive in a mesh shares the same MeshUniformBlock but has its own material and hence it's own textures 
struct MaterialDescriptorInfo
{
	VkDescriptorBufferInfo uniformdescriptorInfo;

	VkDescriptorImageInfo baseColorDescriptorInfo;
	VkDescriptorImageInfo normalDescriptorInfo;
	
	VkDescriptorImageInfo occlusionDescriptorInfo;
	VkDescriptorImageInfo emissiveDescriptorInfo;

	VkDescriptorImageInfo metallicRoughnessDescriptorInfo;
	VkDescriptorImageInfo specularGlossinessDescriptorInfo;
	VkDescriptorImageInfo diffuseTextureDescriptorInfo;
};

//-------------------------------------------------------------
//----------------------- glTF classes ------------------------
//-------------------------------------------------------------
// ALl the structs and classes below were added specifically to load gltf files.
// This is influenced heavily by Sacha Willems implementation of gltf loading here: https://github.com/SaschaWillems/Vulkan/blob/master/base/VulkanglTFModel.hpp

// Added specifically to load gltf files
struct vkNode;
struct vkMesh;
struct vkMaterial;
struct vkPrimitive;

//--------------------------------------------------------------------
//----------------------- glTF mesh class ------------------------
//--------------------------------------------------------------------
struct vkMesh
{
	std::string name;
	VkDevice logicalDevice;
	VkPhysicalDevice pDevice;
	// Multiple buffers for UBO because multiple frames may be in flight at the same time and this is data that could potentially be updated every frame
	std::vector<MeshUniform> meshUniform;
	std::vector<std::shared_ptr<vkPrimitive>> primitives;

	vkMesh(const std::string& name, glm::mat4 matrix, unsigned int numFrames, VkDevice& logicalDevice, VkPhysicalDevice& physicalDevice)
	{
		this->name = name;
		this->logicalDevice = logicalDevice;
		this->pDevice = physicalDevice;

		this->meshUniform.resize(numFrames);
		for (unsigned int i = 0; i < numFrames; i++)
		{
			this->meshUniform[i].uniformBlock.modelMat = matrix;
			BufferUtil::createUniformBuffer(this->logicalDevice, this->pDevice, meshUniform[i].ubo.buffer, meshUniform[i].ubo.memory, sizeof(MeshUniformBlock));
			vkMapMemory(logicalDevice, meshUniform[i].ubo.memory, 0, sizeof(meshUniform[i].uniformBlock), 0, &meshUniform[i].ubo.mappedData);
			meshUniform[i].descriptorInfo = { meshUniform[i].ubo.buffer, 0, sizeof(meshUniform[i].uniformBlock) };
		}
	};

	~vkMesh()
	{
		const unsigned int numUniforms = static_cast<unsigned int>(meshUniform.size());
		for (unsigned int i = 0; i < numUniforms; i++)
		{
			vkDestroyBuffer(logicalDevice, meshUniform[i].ubo.buffer, nullptr);
			vkFreeMemory(logicalDevice, meshUniform[i].ubo.memory, nullptr);
		}
	}

	void updateUniform(glm::mat4& m, unsigned int frameIndex)
	{
		memcpy(this->meshUniform[frameIndex].ubo.mappedData, &m, sizeof(glm::mat4));
	}
};

//--------------------------------------------------------------------
//----------------------- glTF material class ------------------------
//--------------------------------------------------------------------
struct vkMaterial
{
	std::bitset<7> activeTextures;
	std::shared_ptr<Texture2D> baseColorTexture;
	std::shared_ptr<Texture2D> normalTexture;

	std::shared_ptr<Texture2D> metallicRoughnessTexture;
	std::shared_ptr<Texture2D> emissiveTexture;
	std::shared_ptr<Texture2D> occlusionTexture;
	//std::shared_ptr<Texture2D> specularGlossinessTexture;
	//std::shared_ptr<Texture2D> diffuseTexture;

	UniformBufferObject ubo;
	MaterialUniformBlock uniformBlock;
	MaterialDescriptorInfo materialDescriptor;

	vkMaterial(VkDevice& logicalDevice, VkPhysicalDevice& physicalDevice)
	{
		uniformBlock = MaterialUniformBlock();
		BufferUtil::createUniformBuffer(logicalDevice, physicalDevice, ubo.buffer, ubo.memory, sizeof(MaterialUniformBlock));
		vkMapMemory(logicalDevice, ubo.memory, 0, sizeof(uniformBlock), 0, &ubo.mappedData);
		materialDescriptor.uniformdescriptorInfo = { ubo.buffer, 0, sizeof(uniformBlock) };
	}

	void updateUniform()
	{
		memcpy(this->ubo.mappedData, &uniformBlock, sizeof(uniformBlock));
	}
};

//--------------------------------------------------------------------
//----------------------- glTF primitive class ------------------------
//--------------------------------------------------------------------
struct vkPrimitive
{
	uint32_t firstIndex;
	uint32_t indexCount;
	std::shared_ptr<vkMaterial> material;
	std::vector<VkDescriptorSet> descriptorSets;

	vkPrimitive(uint32_t firstIndex, uint32_t indexCount, std::shared_ptr<vkMaterial> material)
		: firstIndex(firstIndex), indexCount(indexCount), material(material)
	{};

	void writeToAndUpdateNodeDescriptorSet(std::shared_ptr<vkMesh> mesh, uint32_t index, VkDevice& logicalDevice)
	{
		VkDescriptorBufferInfo meshDescInfo = mesh->meshUniform[index].descriptorInfo;
		std::vector<VkWriteDescriptorSet> writePrimitiveDescriptorSet = {
			DescriptorUtil::writeDescriptorSet(descriptorSets[index], 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &meshDescInfo),
			DescriptorUtil::writeDescriptorSet(descriptorSets[index], 1, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &material->materialDescriptor.uniformdescriptorInfo),
			DescriptorUtil::writeDescriptorSet(descriptorSets[index], 2, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &material->materialDescriptor.baseColorDescriptorInfo)
		};

		if (material->activeTextures[1]) {
			writePrimitiveDescriptorSet.push_back(
				DescriptorUtil::writeDescriptorSet(descriptorSets[index], 3, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &material->materialDescriptor.normalDescriptorInfo)
			);
		}
		if (material->activeTextures[2]) {
			writePrimitiveDescriptorSet.push_back(
				DescriptorUtil::writeDescriptorSet(descriptorSets[index], 4, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &material->materialDescriptor.metallicRoughnessDescriptorInfo)
			);
		}
		if (material->activeTextures[3]) {
			writePrimitiveDescriptorSet.push_back(
				DescriptorUtil::writeDescriptorSet(descriptorSets[index], 5, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &material->materialDescriptor.emissiveDescriptorInfo)
			);
		}
		if (material->activeTextures[4]) {
			writePrimitiveDescriptorSet.push_back(
				DescriptorUtil::writeDescriptorSet(descriptorSets[index], 6, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &material->materialDescriptor.occlusionDescriptorInfo)
			);
		}

		vkUpdateDescriptorSets(logicalDevice, static_cast<uint32_t>(writePrimitiveDescriptorSet.size()), writePrimitiveDescriptorSet.data(), 0, nullptr);
	}
};

//--------------------------------------------------------------------
//----------------------- glTF Node class ------------------------
//--------------------------------------------------------------------
struct vkNode
{
	std::string name;

	uint32_t index;
	std::shared_ptr<vkNode> parent;
	std::vector<std::shared_ptr<vkNode>> children;
	std::shared_ptr<vkMesh> mesh;

	glm::mat4 matrix;
	glm::vec3 translation{};
	glm::vec3 scale{ 1.0f };
	glm::quat rotation{};

	glm::mat4 localMatrix()
	{
		return glm::translate(glm::mat4(1.0f), translation) * glm::mat4(rotation) * glm::scale(glm::mat4(1.0f), scale) * matrix;
	}

	glm::mat4 getMatrix()
	{
		glm::mat4 mat = localMatrix();
		std::shared_ptr<vkNode> p = parent;
		while (p)
		{
			mat = p->localMatrix() * mat;
			p = p->parent;
		}
		return mat;
	}

	void update(unsigned int frameIndex)
	{
		if (mesh)
		{
			glm::mat4 m = getMatrix();
			mesh->updateUniform(m, frameIndex);
		}

		for (auto& child : children)
		{
			child->update(frameIndex);
		}
	}

	vkNode(uint32_t nodeIndex, std::shared_ptr<vkNode> parent, const std::string& name, 
		glm::vec3 translation, glm::quat rotation, glm::vec3 scale, glm::mat4 matrix, glm::mat4& globalTransform)
	{
		this->name = name;
		this->index = nodeIndex;
		this->parent = parent;

		// Generate local node matrix
		this->translation = translation;
		this->rotation = rotation;
		this->scale = scale;
		this->matrix = globalTransform * matrix;
	}

	~vkNode()
	{
		children.clear();
	}
};