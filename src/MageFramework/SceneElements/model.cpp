#include "model.h"

Model::Model(std::shared_ptr<VulkanManager> vulkanManager, VkQueue& graphicsQueue, VkCommandPool& commandPool, unsigned int numSwapChainImages,
	const JSONItem::Model& jsonModel, bool isMipMapped)
	: m_logicalDevice(vulkanManager->getLogicalDevice()), m_physicalDevice(vulkanManager->getPhysicalDevice()), m_numSwapChainImages(numSwapChainImages), 
	m_areTexturesMipMapped(isMipMapped), m_uboCount(0)
{
	m_updateUniforms.resize(m_numSwapChainImages, true);
	LoadModel(jsonModel, graphicsQueue, commandPool);
}; 
Model::~Model()
{
	vkDeviceWaitIdle(m_logicalDevice);

	vkDestroyBuffer(m_logicalDevice, m_indices.buffer, nullptr);
	vkFreeMemory(m_logicalDevice, m_indices.bufferMemory, nullptr);

	vkDestroyBuffer(m_logicalDevice, m_vertices.buffer, nullptr);
	vkFreeMemory(m_logicalDevice, m_vertices.bufferMemory, nullptr);
}

bool Model::LoadModel(const JSONItem::Model& jsonModel, VkQueue& graphicsQueue, VkCommandPool& commandPool)
{
	glm::mat4 modelTransform = jsonModel.transform;
	if (jsonModel.filetype == FILE_TYPE::OBJ)
	{		
		loadingUtil::loadObj(m_vertices.vertexArray, m_indices.indexArray, m_textures, jsonModel.meshPath, jsonModel.texturePaths,
			m_areTexturesMipMapped, m_logicalDevice, m_physicalDevice, graphicsQueue, commandPool);
		loadingUtil::convertObjToNodeStructure( m_indices, m_textures, m_materials, m_nodes, m_linearNodes,
			jsonModel.name, modelTransform, m_uboCount, m_numSwapChainImages, 
			m_logicalDevice, m_physicalDevice, graphicsQueue, commandPool);
	}
	else if (jsonModel.filetype == FILE_TYPE::GLTF)
	{
		loadingUtil::loadGLTF(m_vertices.vertexArray, m_indices.indexArray, m_textures, m_materials,
			m_nodes, m_linearNodes, jsonModel.meshPath, modelTransform,
			m_uboCount, m_numSwapChainImages, m_logicalDevice, m_physicalDevice, graphicsQueue, commandPool);
	}
	else
	{
		throw std::runtime_error("Model not created because the filetype could not be identified");
	}

	m_vertices.bufferSize = m_vertices.vertexArray.size() * sizeof(Vertex);
	m_indices.bufferSize = m_indices.indexArray.size() * sizeof(uint32_t);
	m_indices.numIndices = static_cast<uint32_t>(m_indices.indexArray.size());

	BufferUtil::createVertexBuffer(m_logicalDevice, m_physicalDevice, graphicsQueue, commandPool,
		m_vertices.buffer, m_vertices.bufferMemory, m_vertices.bufferSize, m_vertices.vertexArray.data());

	BufferUtil::createIndexBuffer(m_logicalDevice, m_physicalDevice, graphicsQueue, commandPool,
		m_indices.buffer, m_indices.bufferMemory, m_indices.bufferSize, m_indices.indexArray.data());

	return true;
}


void Model::updateUniformBuffer(uint32_t currentImageIndex)
{
	if (m_updateUniforms[currentImageIndex])
	{
		for (std::shared_ptr<vkNode> node : m_linearNodes)
		{
			if (node->mesh) 
			{ 
				node->update(currentImageIndex);
			}
		}

		m_updateUniforms[currentImageIndex] = false;
	}
}

void Model::recordDrawCmds(	unsigned int frameIndex, const VkDescriptorSet& DS_camera, 
	const VkPipeline& rasterP, const VkPipelineLayout& rasterPL, VkCommandBuffer& graphicsCmdBuffer )
{
	VkBuffer vertexBuffers[] = { m_vertices.buffer };
	VkBuffer indexBuffer = m_indices.buffer;
	VkDeviceSize offsets[] = { 0 };

	vkCmdBindPipeline(graphicsCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, rasterP);
	vkCmdBindVertexBuffers(graphicsCmdBuffer, 0, 1, vertexBuffers, offsets);
	vkCmdBindIndexBuffer(graphicsCmdBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);

	vkCmdBindDescriptorSets(graphicsCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, rasterPL, 0, 1, &DS_camera, 0, nullptr);

	for (std::shared_ptr<vkNode> node : m_linearNodes)
	{
		if (node->mesh)
		{
			for (std::shared_ptr<vkPrimitive> primitive : node->mesh->primitives)
			{
				VkDescriptorSet DS_primitive = primitive->descriptorSets[frameIndex];
				vkCmdBindDescriptorSets(graphicsCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, rasterPL, 1, 1, &DS_primitive, 0, nullptr);
				vkCmdDrawIndexed(graphicsCmdBuffer, primitive->indexCount, 1, primitive->firstIndex, 0, 0);
			}
		}
	}
}

// Descriptor Setup
void Model::addToDescriptorPoolSize(std::vector<VkDescriptorPoolSize>& poolSizes)
{
	// Model Uniforms + Material Uniforms
	poolSizes.push_back({ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2* m_uboCount * m_numSwapChainImages });
	// (baseColor + metallicRoughness + normal + occlusion + emissive + specularGlossiness + diffuse) Texture Sampler
	poolSizes.push_back({ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 7 * m_uboCount * m_numSwapChainImages });
}
void Model::createDescriptorSetLayout(VkDescriptorSetLayout& DSL_model)
{
	//We pass in a descriptor Set layout because it will remain common to all models that we create. So no point keep multiple copies of it.
	VkDescriptorSetLayoutBinding modelUniformLB					= { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,		  1, VK_SHADER_STAGE_VERTEX_BIT,   nullptr };
	VkDescriptorSetLayoutBinding materialUniformLB				= { 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,		  1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr };
	VkDescriptorSetLayoutBinding baseColorTexSamplerLB			= { 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr };
	VkDescriptorSetLayoutBinding normalTexSamplerLB				= { 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr };
	VkDescriptorSetLayoutBinding metallicRoughnessTexSamplerLB	= { 4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr };
	VkDescriptorSetLayoutBinding emissiveTexSamplerLB			= { 5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr };
	VkDescriptorSetLayoutBinding occlusionTexSamplerLB			= { 6, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr };
	
	//VkDescriptorSetLayoutBinding specularGlossinessTexSamplerLB = { 7, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr };
	//VkDescriptorSetLayoutBinding diffuseTexSamplerLB			= { 8, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr };

	std::vector<VkDescriptorSetLayoutBinding> modelPrimitiveBindings = { 
		modelUniformLB, materialUniformLB, baseColorTexSamplerLB, normalTexSamplerLB, 
		metallicRoughnessTexSamplerLB, emissiveTexSamplerLB, occlusionTexSamplerLB
	//  specularGlossinessTexSamplerLB, diffuseTexSamplerLB };
	};

	DescriptorUtil::createDescriptorSetLayout(m_logicalDevice, DSL_model, static_cast<uint32_t>(modelPrimitiveBindings.size()), modelPrimitiveBindings.data());
}
void Model::createDescriptorSets(VkDescriptorPool descriptorPool, VkDescriptorSetLayout& DSL_model, uint32_t index)
{
	for (std::shared_ptr<vkNode> node : m_linearNodes)
	{
		if (node->mesh)
		{
			for (auto primitive : node->mesh->primitives)
			{
				primitive->descriptorSets.resize(m_numSwapChainImages);
				DescriptorUtil::createDescriptorSets(m_logicalDevice, descriptorPool, 1, &DSL_model, &primitive->descriptorSets[index]);
			}
		}
	}
}
void Model::writeToAndUpdateDescriptorSets(uint32_t index)
{
	// Loop over all materials and create the respective material descriptors
	for (auto material : m_materials)
	{	
		material->updateUniform();
		material->materialDescriptor.uniformdescriptorInfo = DescriptorUtil::createDescriptorBufferInfo(material->ubo.buffer, 0, material->ubo.bufferSize);
		material->materialDescriptor.baseColorDescriptorInfo = DescriptorUtil::createDescriptorImageInfo(material->baseColorTexture);

		if (material->activeTextures[1]) { material->materialDescriptor.normalDescriptorInfo = DescriptorUtil::createDescriptorImageInfo(material->normalTexture);	}
		if (material->activeTextures[2]) { material->materialDescriptor.metallicRoughnessDescriptorInfo = DescriptorUtil::createDescriptorImageInfo(material->metallicRoughnessTexture); }
		if (material->activeTextures[3]) { material->materialDescriptor.emissiveDescriptorInfo = DescriptorUtil::createDescriptorImageInfo(material->emissiveTexture); }
		if (material->activeTextures[4]) { material->materialDescriptor.occlusionDescriptorInfo = DescriptorUtil::createDescriptorImageInfo(material->occlusionTexture); }
		
	//	if (material->activeTextures[5]) { material->materialDescriptor.specularGlossinessDescriptorInfo = DescriptorUtil::createDescriptorImageInfo(material->specularGlossinessTexture);	}
	//	if (material->activeTextures[6]) { material->materialDescriptor.diffuseTextureDescriptorInfo = DescriptorUtil::createDescriptorImageInfo(material->diffuseTexture);	}
	}

	for (std::shared_ptr<vkNode> node : m_linearNodes)
	{
		if (node->mesh)
		{
			// Create the mesh descriptors
			MeshUniform meshUBO = node->mesh->meshUniform[index];
			meshUBO.descriptorInfo = DescriptorUtil::createDescriptorBufferInfo(meshUBO.ubo.buffer, 0, meshUBO.ubo.bufferSize);

			for (auto primitive : node->mesh->primitives)
			{
				primitive->writeToAndUpdateNodeDescriptorSet(node->mesh, index, m_logicalDevice);
			}
		}
	}
}