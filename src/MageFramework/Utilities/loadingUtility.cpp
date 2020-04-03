#pragma once
#include <Utilities/loadingUtility.h>

// Disable Warnings: 
#pragma warning( disable : 6386 )  // C6386: Buffer overrun possible;
#pragma warning( disable : 6282 )  // C6282: Incorrect operator: assignment of constant in Boolean context. Consider using '==' instead
#pragma warning( disable : 6385 )  // C6385: The readable extent of the buffer might be smaller than the index used to read from it
#pragma warning( disable : 6001 )  // C6385: using uninitialized memory

// These defines below can only be defined once
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image.h>
#include <stb_image_write.h>
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_INCLUDE_STB_IMAGE
#define TINYGLTF_NO_INCLUDE_STB_IMAGE_WRITE
#include <tiny_gltf.h>

// Helpers
void readTinygltfImages( tinygltf::Model& gltfModel, std::vector<std::shared_ptr<Texture2D>>& textures, 
	VkDevice& logicalDevice, VkPhysicalDevice& pDevice, VkQueue& graphicsQueue, VkCommandPool& commandPool );
void readTinygltfMaterials( tinygltf::Model& gltfModel, std::vector<std::shared_ptr<vkMaterial>>& materials, std::vector<std::shared_ptr<Texture2D>>& textures,
	VkDevice& logicalDevice, VkPhysicalDevice& pDevice );

void readTinygltfMesh( tinygltf::Model& gltfModel, tinygltf::Mesh& gltfMesh,
	std::shared_ptr<vkNode> newNode, std::vector<std::shared_ptr<vkMaterial>>& materials,
	std::vector<uint32_t>& indexBuffer, std::vector<Vertex>& vertexBuffer,
	unsigned int numFrames, VkDevice& logicalDevice, VkPhysicalDevice& pDevice );

void readTinygltfNode( tinygltf::Node& gltfNode, uint32_t nodeIndex, tinygltf::Model& gltfModel, glm::mat4& transform,
	std::shared_ptr<vkNode> parent, std::vector<std::shared_ptr<vkMaterial>>& materials,
	std::vector<std::shared_ptr<vkNode>>& nodes, std::vector<std::shared_ptr<vkNode>>& linearNodes,
	std::vector<uint32_t>& indexBuffer, std::vector<Vertex>& vertexBuffer,
	unsigned int numFrames, VkDevice& logicalDevice, VkPhysicalDevice& pDevice );

JSONContents loadingUtil::loadJSON(const std::string jsonFilePath)
{
	std::string str = "../../src/Assets/Scenes/";
	str.append(jsonFilePath);
	const char* obj_file_path = str.c_str();

	// read a JSON file
	std::ifstream i(str);
	json j;
	i >> j;

	// parse JSON
	JSONContents jsonContent;
	JSONItem::Camera& camera = jsonContent.mainCamera;
	{
		auto cameraObject = j["scene"]["camera"];
		camera.eyePos = glm::vec3( cameraObject["eye"]["x"].get<float>(),
								   cameraObject["eye"]["y"].get<float>(),
								   cameraObject["eye"]["z"].get<float>() );
		camera.lookAtPoint = glm::vec3( cameraObject["lookAtPoint"]["x"].get<float>(),
										cameraObject["lookAtPoint"]["y"].get<float>(),
										cameraObject["lookAtPoint"]["z"].get<float>() );
		camera.foV_vertical = cameraObject["fovY"].get<float>();
		camera.width = cameraObject["width"].get<int>();
		camera.height = cameraObject["height"].get<int>();
		camera.aspectRatio = static_cast<float>(camera.width) / static_cast<float>(camera.height);
		camera.nearClip = cameraObject["nearClip"].get<float>();
		camera.farClip = cameraObject["farClip"].get<float>();
		camera.focalDistance = cameraObject["focalDistance"].get<float>();
		camera.lensRadius = cameraObject["lensRadius"].get<float>();
	}

	std::vector<JSONItem::Model>& modelList = jsonContent.scene.modelList;
	for (auto& element : j["scene"]["models"].items())
	{
		JSONItem::Model model;
		model.name = element.value()["name"].get<std::string>();
		model.meshPath = element.value()["meshPath"].get<std::string>();

		if (model.meshPath.find(".obj") != std::string::npos)
		{
			model.filetype = FILE_TYPE::OBJ;
		}
		else if(model.meshPath.find(".gltf") != std::string::npos)
		{
			model.filetype = FILE_TYPE::GLTF;
		}
		else
		{
			throw std::runtime_error("Could not identify file type for mesh being loaded");
		}
		
		for (auto& texturePath : element.value()["texturePaths"].items())
		{
			model.texturePaths.push_back(texturePath.value().get<std::string>());
		}
		model.material = element.value()["material"].get<std::string>();
		glm::vec3 translation = element.value()["transform"]["translate"].get<glm::vec3>();
		glm::vec3 rotation = element.value()["transform"]["rotate"].get<glm::vec3>();
		glm::vec3 scale = element.value()["transform"]["scale"].get<glm::vec3>();
		model.transform = glm::translate(glm::mat4(1.0f), translation)
						* glm::rotate(glm::mat4(1.0f), glm::radians(rotation.x), glm::vec3(1, 0, 0))
						* glm::rotate(glm::mat4(1.0f), glm::radians(rotation.y), glm::vec3(0, 1, 0))
						* glm::rotate(glm::mat4(1.0f), glm::radians(rotation.z), glm::vec3(0, 0, 1))
						* glm::scale(glm::mat4(1.0f), scale);;
		modelList.push_back(model);
	}

	return jsonContent;
}

void loadingUtil::loadImageUsingSTB(const std::string filename, ImageLoaderOutput& out, VkDevice& logicalDevice, VkPhysicalDevice& pDevice)
{	
	std::string str = "../../src/Assets/Textures/";
	str.append(filename);
	const char* image_file_path = str.c_str();

	// The pointer that is returned is the first element in an array of pixel values. 
	// The pixels are laid out row by row with 4 bytes per pixel in the case of STBI_rgba_alpha for a total of texWidth * texHeight * 4 values.
	int numChannelsActuallyInImage;
	unsigned char* pixels = stbi_load(image_file_path, &out.imgWidth, &out.imgHeight, &numChannelsActuallyInImage, STBI_rgb_alpha);
	if (!pixels) { throw std::runtime_error("failed to load image!"); }

	VkDeviceSize imageSize = out.imgWidth * out.imgHeight * 4;
	BufferUtil::createStagingBuffer(logicalDevice, pDevice, pixels, out.stagingBuffer, out.stagingBufferMemory, imageSize);
	
	stbi_image_free(pixels);
}

void loadingUtil::loadArrayOfImageUsingSTB(std::vector<std::string>& texturePaths, ImageArrayLoaderOutput& out, VkDevice& logicalDevice, VkPhysicalDevice& pDevice)
{
	VkDeviceSize imageSize = 0;
	std::vector<unsigned char*> pixelsArray;
	for (uint32_t layer = 0; layer < texturePaths.size(); layer++)
	{
		std::string str = "../../src/Assets/Textures/";
		str.append(texturePaths[layer]);
		const char* image_file_path = str.c_str();

		// The pointer that is returned is the first element in an array of pixel values. 
		// The pixels are laid out row by row with 4 bytes per pixel in the case of STBI_rgba_alpha for a total of texWidth * texHeight * 4 values.
		int imgW, imgH, numChannelsActuallyInImage;;
		unsigned char* pixels = stbi_load(image_file_path, &imgW, &imgH, &numChannelsActuallyInImage, STBI_rgb_alpha);
		if (!pixels) { throw std::runtime_error("failed to load image!"); }
		pixelsArray.push_back(pixels);
		stbi_image_free(pixels);

		out.imgWidths.push_back(static_cast<uint32_t>(imgW));
		out.imgHeights.push_back(static_cast<uint32_t>(imgH));

		// Assumption: All images in the array, i.e. all images being loaded are of the same size
		imageSize = (layer == 0) ? (imgW * imgH * 4) : (imageSize * 2);
	}

	BufferUtil::createStagingBuffer(logicalDevice, pDevice, pixelsArray.data(), out.stagingBuffer, out.stagingBufferMemory, imageSize);
}

bool loadingUtil::loadObj(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices, std::vector<std::shared_ptr<Texture2D>>& textures,
	const std::string meshFilePath, const std::vector<std::string>& textureFilePaths, bool areTexturesMipMapped,
	VkDevice& logicalDevice, VkPhysicalDevice& pDevice, VkQueue& graphicsQueue, VkCommandPool& commandPool)
{
	std::string str = "../../src/Assets/Models/obj/";
	str.append(meshFilePath);
	const char* obj_file_path = str.c_str();

	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, obj_file_path, nullptr, true))
	{
		throw std::runtime_error(err);
		return false;
	}

#ifndef NDEBUG
	std::cout << "\nAn obj file was loaded" << std::endl;
	std::cout << "# of vertices  : " << (attrib.vertices.size() / 3) << std::endl;
	std::cout << "# of normals   : " << (attrib.normals.size() / 3) << std::endl;
	std::cout << "# of texcoords : " << (attrib.texcoords.size() / 2) << std::endl;
	std::cout << "# of shapes    : " << shapes.size() << std::endl;
	std::cout << "# of materials : " << materials.size() << std::endl;
#endif

	std::unordered_map<Vertex, uint32_t> uniqueVertices = {};
	//Assumption: obj files are consistent for all attributes, i.e. attributes exist for all elements or for none of them
	const bool hasPosition = attrib.vertices.size() > 0;
	const bool hasNormal = attrib.normals.size() > 0;
	const bool hasUV = attrib.texcoords.size() > 0;

	if (!hasPosition)
	{
		throw std::runtime_error("failed to load obj!");
	}

	// Meshes
	for (const auto& shape : shapes)
	{
		for (const auto& index : shape.mesh.indices)
		{
			Vertex vertex = {};

			vertex.position = {
				static_cast<const float>(attrib.vertices[3 * index.vertex_index + 0]),
				static_cast<const float>(attrib.vertices[3 * index.vertex_index + 1]),
				static_cast<const float>(attrib.vertices[3 * index.vertex_index + 2])
			};

			if (hasUV)
			{
				vertex.uv = {
					static_cast<const float>(attrib.texcoords[2 * index.texcoord_index + 0]),
					static_cast<const float>(1.0f - attrib.texcoords[2 * index.texcoord_index + 1])
				};
			}
			else
			{
				vertex.uv = glm::vec2(0.0f);
			}

			if (hasNormal)
			{
				vertex.normal = {
					static_cast<const float>(attrib.normals[3 * index.normal_index + 0]),
					static_cast<const float>(attrib.normals[3 * index.normal_index + 1]),
					static_cast<const float>(attrib.normals[3 * index.normal_index + 2])
				};
			}
			else
			{
				vertex.normal = glm::vec3(0.0f, 0.0f, 0.0f);
			}
			
			if (uniqueVertices.count(vertex) == 0)
			{
				uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
				vertices.push_back(vertex);
			}

			indices.push_back(uniqueVertices[vertex]);
		}
	}

	// Textures
	for (unsigned int i = 0; i < textureFilePaths.size(); i++)
	{
		std::shared_ptr<Texture2D> texture =
			std::make_shared<Texture2D>(logicalDevice, pDevice, graphicsQueue, commandPool, VK_FORMAT_R8G8B8A8_UNORM);
		texture->create2DTexture(textureFilePaths[i], graphicsQueue, commandPool, areTexturesMipMapped);
		textures.push_back(texture);
	}

	return true;
}

bool loadingUtil::loadGLTF(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices,
	std::vector<std::shared_ptr<Texture2D>>& textures, std::vector<std::shared_ptr<vkMaterial>>& materials,
	std::vector<std::shared_ptr<vkNode>>& nodes, std::vector<std::shared_ptr<vkNode>>& linearNodes, 
	const std::string filename, glm::mat4& transform, uint32_t& uboCount, unsigned int numFrames,
	VkDevice& logicalDevice, VkPhysicalDevice& pDevice, VkQueue& graphicsQueue, VkCommandPool& commandPool)
{
	tinygltf::Model gltfModel;
	tinygltf::TinyGLTF gltfLoader;
	std::string errors, warnings;

	std::string str = "../../src/Assets/Models/gltf/";
	str.append(filename);
	const char* gltf_file_path = str.c_str();

	bool res = gltfLoader.LoadASCIIFromFile(&gltfModel, &errors, &warnings, gltf_file_path);
	if (!res) { std::cout << "Failed to load glTF: " << filename << std::endl; }
	if (!warnings.empty()) { std::cout << "WARNING: " << warnings << std::endl; }
	if (!errors.empty()) { std::cout << "ERROR: " << errors << std::endl; }

	// Read the data from the loaded in gltf file
	{
		readTinygltfImages(gltfModel, textures, logicalDevice, pDevice, graphicsQueue, commandPool);
		readTinygltfMaterials(gltfModel, materials, textures, logicalDevice, pDevice);

		// Load in the index and vertex buffers
		std::vector<ImageArrayLoaderOutput::IMAGE_USAGE> imgUsageTypes;
		const tinygltf::Scene& gltfScene = gltfModel.scenes[gltfModel.defaultScene > -1 ? gltfModel.defaultScene : 0];
		for (size_t i = 0; i < gltfScene.nodes.size(); i++)
		{
			uint32_t nodeIndex = gltfScene.nodes[i];
			tinygltf::Node gltfNode = gltfModel.nodes[nodeIndex];
			readTinygltfNode(gltfNode, nodeIndex, gltfModel, transform, nullptr,
				materials, nodes, linearNodes, indices, vertices, numFrames, logicalDevice, pDevice);
		}

		for (auto node : linearNodes)
		{
			for (unsigned int i = 0; i < numFrames; i++)
			{
				if (node->mesh) { node->update(i); }
			}
		}
	}

#ifndef NDEBUG
	std::cout << "\nA gltf file was loaded" << std::endl;
	std::cout << "# of vertices  : " << (vertices.size() * 3) << std::endl;
	std::cout << "# of triangles  : " << (vertices.size()) << std::endl;
#endif

	// Total number of descriptors
	for (auto node : linearNodes) 
	{
		if (node->mesh) { uboCount++; }
	}

	return res;
}

void loadingUtil::convertObjToNodeStructure(Indices& indices,
	std::vector<std::shared_ptr<Texture2D>>& textures, std::vector<std::shared_ptr<vkMaterial>>& materials,
	std::vector<std::shared_ptr<vkNode>>& nodes, std::vector<std::shared_ptr<vkNode>>& linearNodes,
	const std::string filename, glm::mat4& transform, uint32_t& uboCount, unsigned int numFrames,
	VkDevice& logicalDevice, VkPhysicalDevice& pDevice, VkQueue& graphicsQueue, VkCommandPool& commandPool)
{
	std::shared_ptr<vkMaterial> material = std::make_shared<vkMaterial>(logicalDevice, pDevice);
	material->activeTextures.reset();
	material->baseColorTexture = textures[0];
	material->activeTextures[0] = true;
	materials.push_back(material);

	// We are assuming any model loaded as an Obj file has one baseColor texture
	// If a second texture exists we assume it's a normal texture 
	if (textures.size() > 1)
	{
		material->normalTexture = textures[1];
		material->activeTextures[1] = true;
	}
	materials.push_back(material);

	std::shared_ptr<vkNode> newNode = std::make_shared<vkNode>(0, nullptr, filename,
		glm::vec3(1.0f), glm::quat(glm::vec4(0.0)), glm::vec3(1.0f), glm::mat4(1.0f), transform);

	uint32_t indexCount = static_cast<uint32_t>(indices.indexArray.size());
	std::shared_ptr<vkMesh> newMesh = std::make_shared<vkMesh>(filename, glm::mat4(1.0f), numFrames, logicalDevice, pDevice);
	std::shared_ptr<vkPrimitive> newPrimitive = std::make_shared<vkPrimitive>(0, indexCount, materials[0]);
	newMesh->primitives.push_back(newPrimitive);
	newNode->mesh = newMesh;

	uboCount = 1;
	nodes.push_back(newNode);
	linearNodes.push_back(newNode);
}

//---------------------------------------------------------------
//--------------------------- Helpers ---------------------------
//---------------------------------------------------------------

void readTinygltfImages( tinygltf::Model& gltfModel, std::vector<std::shared_ptr<Texture2D>>& textures, 
	VkDevice& logicalDevice, VkPhysicalDevice& pDevice, VkQueue& graphicsQueue, VkCommandPool& commandPool )
{
	for (tinygltf::Image& gltfImage : gltfModel.images)
	{
		VkDeviceSize imageSize = 0;
		unsigned char* pixels = nullptr;
		if (gltfImage.component == 3) 
		{
			// Most devices don't support RGB only on Vulkan so convert if necessary
			imageSize = gltfImage.width * gltfImage.height * 4;
			pixels = new unsigned char[imageSize];
			unsigned char* rgba = pixels;
			unsigned char* rgb = &gltfImage.image[0];
			for (size_t i = 0; i < gltfImage.width * gltfImage.height; i++) 
			{
				rgba[i*4 + 0] = rgb[i*3 + 0];
				rgba[i*4 + 1] = rgb[i*3 + 1];
				rgba[i*4 + 2] = rgb[i*3 + 2];
				rgba[i*4 + 3] = static_cast<unsigned char>(1.0f);
			}
		}
		else
		{
			pixels = &gltfImage.image[0];
			imageSize = gltfImage.image.size();
		}
		
		VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
		uint32_t mipLevels = static_cast<uint32_t>(floor(log2(std::max(gltfImage.width, gltfImage.height))) + 1.0);
		std::shared_ptr<Texture2D> texture =
			std::make_shared<Texture2D>(logicalDevice, pDevice, graphicsQueue, commandPool, format,	mipLevels);

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		BufferUtil::createStagingBuffer(logicalDevice, pDevice, pixels, stagingBuffer, stagingBufferMemory, imageSize);

		ImageLoaderOutput imgOut = { stagingBuffer, stagingBufferMemory, gltfImage.width, gltfImage.height };
		texture->create2DTexture(imgOut, graphicsQueue, commandPool, true);
		textures.push_back(texture);

#ifndef NDEBUG
		//if (gltfImage.name != "")
		//{
		//	std::cout << "\nA gltf Image, " << gltfImage.name << ", was loaded" << std::endl;
		//}
		//else
		//{
		//	std::cout << "\nA gltf Image, " << gltfImage.uri << ", was loaded" << std::endl;
		//}
#endif		
	}
}

void readTinygltfMaterials(tinygltf::Model& gltfModel, std::vector<std::shared_ptr<vkMaterial>>& materials, std::vector<std::shared_ptr<Texture2D>>& textures,
	VkDevice& logicalDevice, VkPhysicalDevice& pDevice)
{
	for (tinygltf::Material& mat : gltfModel.materials) 
	{
		std::shared_ptr<vkMaterial> material = std::make_shared<vkMaterial>(logicalDevice, pDevice);
		material->activeTextures.reset();
		
		if (mat.values.find("baseColorTexture") != mat.values.end())
		{
			material->activeTextures[0] = true;
			material->baseColorTexture = textures[gltfModel.textures[mat.values["baseColorTexture"].TextureIndex()].source];
		}
		else
		{
			throw std::runtime_error("All meshes and hence materials are expected to have a base Color texture");
		}
		
		if(mat.additionalValues.find("normalTexture") != mat.additionalValues.end())
		{
			material->activeTextures[1] = true;
			material->normalTexture = textures[gltfModel.textures[mat.additionalValues["normalTexture"].TextureIndex()].source];
		}

		if (mat.values.find("metallicRoughnessTexture") != mat.values.end())
		{
			material->activeTextures[2] = true;
			material->metallicRoughnessTexture = textures[gltfModel.textures[mat.values["metallicRoughnessTexture"].TextureIndex()].source];
		}

		if (mat.additionalValues.find("emissiveTexture") != mat.additionalValues.end())
		{
			material->activeTextures[3] = true;
			material->emissiveTexture = textures[gltfModel.textures[mat.additionalValues["emissiveTexture"].TextureIndex()].source];
		}
		if (mat.additionalValues.find("occlusionTexture") != mat.additionalValues.end())
		{
			material->activeTextures[4] = true;
			material->occlusionTexture = textures[gltfModel.textures[mat.additionalValues["occlusionTexture"].TextureIndex()].source];
		}

		// Metallic roughness workflow
		if (mat.values.find("roughnessFactor") != mat.values.end()) 
		{
			material->uniformBlock.roughnessFactor = static_cast<float>(mat.values["roughnessFactor"].Factor());
		}
		if (mat.values.find("metallicFactor") != mat.values.end()) 
		{
			material->uniformBlock.metallicFactor = static_cast<float>(mat.values["metallicFactor"].Factor());
		}
		if (mat.values.find("baseColorFactor") != mat.values.end()) 
		{
			material->uniformBlock.baseColorFactor = glm::make_vec4(mat.values["baseColorFactor"].ColorFactor().data());
		}

		if (mat.additionalValues.find("alphaMode") != mat.additionalValues.end()) 
		{
			tinygltf::Parameter param = mat.additionalValues["alphaMode"];
			if (param.string_value == "BLEND") {
				material->uniformBlock.alphaMode = AlphaMode::ALPHAMODE_BLEND;
			}
			if (param.string_value == "MASK") {
				material->uniformBlock.alphaMode = AlphaMode::ALPHAMODE_MASK;
			}
		}
		if (mat.additionalValues.find("alphaCutoff") != mat.additionalValues.end()) 
		{
			material->uniformBlock.alphaCutoff = static_cast<float>(mat.additionalValues["alphaCutoff"].Factor());
		}

		materials.push_back(material);
	}
}

void readTinygltfMesh(tinygltf::Model& gltfModel, tinygltf::Mesh& gltfMesh,
	std::shared_ptr<vkNode> newNode, std::vector<std::shared_ptr<vkMaterial>>& materials,
	std::vector<uint32_t>& indices, std::vector<Vertex>& vertices,
	unsigned int numFrames, VkDevice& logicalDevice, VkPhysicalDevice& pDevice)
{
	std::shared_ptr<vkMesh> newMesh = std::make_shared<vkMesh>(gltfMesh.name, glm::mat4(1.0f), numFrames, logicalDevice, pDevice);
	for (size_t i = 0; i < gltfMesh.primitives.size(); i++)
	{
		const tinygltf::Primitive primitive = gltfMesh.primitives[i];
		if (primitive.indices < 0) 
		{
			continue;
		}
		uint32_t indexStart = static_cast<uint32_t>(indices.size());
		uint32_t vertexStart = static_cast<uint32_t>(vertices.size());
		uint32_t indexCount = 0;

		// Vertices
		{
			const float* bufferPos = nullptr;
			const float* bufferNormals = nullptr;
			const float* bufferTexCoords = nullptr;

			// Position attribute is required
			assert(primitive.attributes.find("POSITION") != primitive.attributes.end());

			const tinygltf::Accessor& posAccessor = gltfModel.accessors[primitive.attributes.find("POSITION")->second];
			const tinygltf::BufferView& posView = gltfModel.bufferViews[posAccessor.bufferView];
			bufferPos = reinterpret_cast<const float*>(&(gltfModel.buffers[posView.buffer].data[posAccessor.byteOffset + posView.byteOffset]));
			
			if (primitive.attributes.find("NORMAL") != primitive.attributes.end()) 
			{
				const tinygltf::Accessor& normAccessor = gltfModel.accessors[primitive.attributes.find("NORMAL")->second];
				const tinygltf::BufferView& normView = gltfModel.bufferViews[normAccessor.bufferView];
				bufferNormals = reinterpret_cast<const float*>(&(gltfModel.buffers[normView.buffer].data[normAccessor.byteOffset + normView.byteOffset]));
			}

			if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end()) 
			{
				const tinygltf::Accessor& uvAccessor = gltfModel.accessors[primitive.attributes.find("TEXCOORD_0")->second];
				const tinygltf::BufferView& uvView = gltfModel.bufferViews[uvAccessor.bufferView];
				bufferTexCoords = reinterpret_cast<const float*>(&(gltfModel.buffers[uvView.buffer].data[uvAccessor.byteOffset + uvView.byteOffset]));
			}

			for (size_t v = 0; v < posAccessor.count; v++) 
			{
				Vertex vert{};
				vert.position = glm::make_vec3(&bufferPos[v * 3]);

				if (bufferNormals) { vert.normal = glm::normalize(glm::make_vec3(&bufferNormals[v*3])); }
				else { vert.normal = glm::vec3(0.0f, 0.0f, 0.0f); }

				vert.uv = bufferTexCoords ? glm::make_vec2(&bufferTexCoords[v*2]) : glm::vec2(0.0f);

				vertices.push_back(vert);
			}
		}
		// Indices
		{
			const tinygltf::Accessor& indexAccessor = gltfModel.accessors[primitive.indices];
			const tinygltf::BufferView& bufferView = gltfModel.bufferViews[indexAccessor.bufferView];
			const tinygltf::Buffer& buffer = gltfModel.buffers[bufferView.buffer];

			indexCount = static_cast<uint32_t>(indexAccessor.count);
			switch (indexAccessor.componentType)
			{
				case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: 
				{
					uint32_t* buf = new uint32_t[indexAccessor.count];
					memcpy(buf, &buffer.data[indexAccessor.byteOffset + bufferView.byteOffset], indexAccessor.count * sizeof(uint32_t));
					for (size_t index = 0; index < indexAccessor.count; index++)
					{
						indices.push_back(buf[index] + vertexStart);
					}
					break;
				}
				case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: 
				{
					uint16_t* buf = new uint16_t[indexAccessor.count];
					memcpy(buf, &buffer.data[indexAccessor.byteOffset + bufferView.byteOffset], indexAccessor.count * sizeof(uint16_t));
					for (size_t index = 0; index < indexAccessor.count; index++)
					{
						indices.push_back(buf[index] + vertexStart);
					}
					break;
				}
				case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: 
				{
					uint8_t* buf = new uint8_t[indexAccessor.count];
					memcpy(buf, &buffer.data[indexAccessor.byteOffset + bufferView.byteOffset], indexAccessor.count * sizeof(uint8_t));
					for (size_t index = 0; index < indexAccessor.count; index++)
					{
						indices.push_back(buf[index] + vertexStart);
					}
					break;
				}
				default:
					std::cerr << "Index component type " << indexAccessor.componentType << " not supported!" << std::endl;
					return;
			}
		}

		std::shared_ptr<vkPrimitive> newPrimitive =
			std::make_shared<vkPrimitive>(indexStart, indexCount, materials[primitive.material]);
		newMesh->primitives.push_back(newPrimitive);
	}
	newNode->mesh = newMesh;
}

void readTinygltfNode(tinygltf::Node& gltfNode, uint32_t nodeIndex, tinygltf::Model& gltfModel, glm::mat4& transform,
		std::shared_ptr<vkNode> parent, std::vector<std::shared_ptr<vkMaterial>>& materials,
		std::vector<std::shared_ptr<vkNode>>& nodes, std::vector<std::shared_ptr<vkNode>>& linearNodes,
		std::vector<uint32_t>& indices, std::vector<Vertex>& vertices,
		unsigned int numFrames, VkDevice& logicalDevice, VkPhysicalDevice& pDevice)
{
	std::shared_ptr<vkNode> newNode;
	// Generate local node matrix
	if ((gltfNode.translation.size() == 3) && (gltfNode.rotation.size() == 4) && (gltfNode.scale.size() == 3) && (gltfNode.matrix.size() == 16))
	{
		const double* translationData = gltfNode.translation.data();
		const double* rotationData = gltfNode.rotation.data();
		const double* scaleData = gltfNode.scale.data();
		const double* matrixData = gltfNode.matrix.data();
		newNode = std::make_shared<vkNode>(nodeIndex, parent, gltfNode.name,
			glm::make_vec3(translationData), glm::make_quat(rotationData), glm::make_vec3(scaleData), glm::make_mat4x4(matrixData), transform);
	}
	else
	{
		newNode = std::make_shared<vkNode>(nodeIndex, parent, gltfNode.name,
			glm::vec3(1.0f), glm::quat(glm::vec4(0.0)), glm::vec3(1.0f), glm::mat4(1.0f), transform);
	}
	
	//Recurse over all children
	for (uint32_t i = 0; i < gltfNode.children.size(); i++)
	{
		const uint32_t childNodeIndex = gltfNode.children[i];
		readTinygltfNode(gltfModel.nodes[childNodeIndex], childNodeIndex, gltfModel, transform,
			newNode, materials, nodes, linearNodes, indices, vertices, numFrames, logicalDevice, pDevice);
	}

	// Read the mesh if the gltfNode contains mesh data
	if (gltfNode.mesh > -1)
	{
		readTinygltfMesh(gltfModel, gltfModel.meshes[gltfNode.mesh], newNode, 
			materials, indices, vertices, numFrames, logicalDevice, pDevice);
	}

	if (parent) { parent->children.push_back(newNode); }
	else { nodes.push_back(newNode); }
	linearNodes.push_back(newNode);
}