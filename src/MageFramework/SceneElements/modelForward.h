#pragma once
#include <bitset>
#include <global.h>
#include <Vulkan/Utilities/vBufferUtil.h>
#include <Vulkan/Utilities/vDescriptorUtil.h>
#include <SceneElements/texture.h>

enum AlphaMode { ALPHAMODE_OPAQUE, ALPHAMODE_MASK, ALPHAMODE_BLEND };

struct Vertices
{
	uint32_t numVertices;
	std::vector<Vertex> vertexArray;
	mageVKBuffer vertexBuffer;
};

struct Indices
{
	uint32_t numIndices;
	std::vector<uint32_t> indexArray;
	mageVKBuffer indexBuffer;
};

struct MeshUniformBlock
{
	glm::mat4 modelMat;
};

struct MeshUniform
{
	mageVKBuffer meshUB; // mesh Uniform Buffer
	MeshUniformBlock uniformBlock;
};

struct MaterialUniformBlock
{
	unsigned long activeTextureFlags; // shader storage for std::bitset<7> activeTextures;
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

//-------------------------------------------------------------
//----------------------- glTF classes ------------------------
//-------------------------------------------------------------
// ALl the structs and classes below were added specifically to load gltf files.
// This is influenced heavily by Sacha Willems implementation of gltf loading here: https://github.com/SaschaWillems/Vulkan/blob/master/base/VulkanglTFModel.hpp

// Added specifically to load gltf files
struct vkMesh;

//--------------------------------------------------------------------
//----------------------- glTF material class ------------------------
//--------------------------------------------------------------------
struct vkMaterial
{
	std::string name;
	VkDevice logicalDevice;

	std::bitset<7> activeTextures;
	std::shared_ptr<Texture2D> baseColorTexture;
	std::shared_ptr<Texture2D> normalTexture;

	std::shared_ptr<Texture2D> metallicRoughnessTexture;
	std::shared_ptr<Texture2D> emissiveTexture;
	std::shared_ptr<Texture2D> occlusionTexture;
	//std::shared_ptr<Texture2D> specularGlossinessTexture;
	//std::shared_ptr<Texture2D> diffuseTexture;

	mageVKBuffer materialUB; // material Uniform Buffer
	MaterialUniformBlock uniformBlock;
	
	vkMaterial(const std::string& name, VkDevice& logicalDevice, VkPhysicalDevice& physicalDevice)
	{
		this->name = name;
		this->logicalDevice = logicalDevice;
		uniformBlock = MaterialUniformBlock();
		BufferUtil::createMageBuffer(logicalDevice, physicalDevice, materialUB, sizeof(MaterialUniformBlock), nullptr, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
		materialUB.map(logicalDevice);
		materialUB.setDescriptorInfo();
	}
	~vkMaterial()
	{
		materialUB.unmap(this->logicalDevice);
		materialUB.destroy(this->logicalDevice);
	}

	void updateUniform()
	{
		materialUB.copyDataToMappedBuffer(&uniformBlock);
	}
};

//--------------------------------------------------------------------
//----------------------- glTF primitive class ------------------------
//--------------------------------------------------------------------
struct vkPrimitive
{
	uint32_t firstIndex;
	uint32_t indexCount;
	uint32_t firstVertex;
	uint32_t vertexCount;
	vkMaterial* material;
	std::vector<VkDescriptorSet> descriptorSets;
	
	vkPrimitive(uint32_t firstIndex, uint32_t indexCount, uint32_t firstVertex,	uint32_t vertexCount, vkMaterial* material)
		: firstIndex(firstIndex), indexCount(indexCount), firstVertex(firstVertex), vertexCount(vertexCount), material(material)
	{};

	void writeToAndUpdateNodeDescriptorSet(vkMesh* mesh, uint32_t index, VkDevice& logicalDevice);
};

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
	std::vector<vkPrimitive*> primitives;

	vkMesh(const std::string& name, glm::mat4 matrix, unsigned int numFrames, VkDevice& logicalDevice, VkPhysicalDevice& physicalDevice)
	{
		this->name = name;
		this->logicalDevice = logicalDevice;
		this->pDevice = physicalDevice;

		this->meshUniform.resize(numFrames);
		for (unsigned int i = 0; i < numFrames; i++)
		{
			this->meshUniform[i].uniformBlock.modelMat = matrix;
			BufferUtil::createMageBuffer(this->logicalDevice, this->pDevice, meshUniform[i].meshUB,
				sizeof(MeshUniformBlock), nullptr, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
			meshUniform[i].meshUB.map(this->logicalDevice);
			meshUniform[i].meshUB.setDescriptorInfo();
		}
	};

	~vkMesh()
	{
		const unsigned int numUniforms = static_cast<unsigned int>(meshUniform.size());
		for (unsigned int i = 0; i < numUniforms; i++)
		{
			meshUniform[i].meshUB.destroy(this->logicalDevice);
		}
		for (vkPrimitive* primitive : primitives)
		{
			delete primitive;
		}
	}

	void updateUniform(glm::mat4& m, unsigned int frameIndex)
	{
		meshUniform[frameIndex].meshUB.copyDataToMappedBuffer(&m);
	}
};

//--------------------------------------------------------------------
//----------------------- glTF Node class ------------------------
//--------------------------------------------------------------------
struct vkNode
{
	std::string name;

	uint32_t index;
	vkNode* parent;
	std::vector<vkNode*> children;
	vkMesh* mesh;

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
		vkNode* p = parent;
		while (p)
		{
			mat = p->localMatrix() * mat;
			p = p->parent;
		}
		return mat;
	}

	glm::mat3x4 getMatrix_3x4()
	{
		glm::mat4 mat = getMatrix();
		return glm::mat3x4(glm::row(mat, 0), glm::row(mat, 1), glm::row(mat, 2));
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

	vkNode(uint32_t nodeIndex, vkNode* parent, const std::string& name, 
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
		if (mesh) { delete mesh; }
		for (auto child : children)
		{
			delete child;
		}
	}
};