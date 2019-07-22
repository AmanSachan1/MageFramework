#pragma once
#include <Utilities/loadingUtility.h>

// These defines below can only be defined once
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define TINYOBJLOADER_IMPLEMENTATION
#include <stb_image.h>
#include <stb_image_write.h>
#include "tiny_obj_loader.h"

unsigned char* loadingUtil::loadImage(std::string imagePath, int* width, int* height, int* numChannels)
{
	std::string str = "../../src/Assets/Textures/";
	str.append(imagePath);
	const char* image_file_path = str.c_str();

	// The pointer that is returned is the first element in an array of pixel values. 
	// The pixels are laid out row by row with 4 bytes per pixel in the case of STBI_rgba_alpha for a total of texWidth * texHeight * 4 values.
	unsigned char* pixels = stbi_load(image_file_path, width, height, numChannels, STBI_rgb_alpha);
	if (!pixels)
	{
		throw std::runtime_error("failed to load image!");
	}

	return pixels;
}

void loadingUtil::freeImage(unsigned char* pixels)
{
	stbi_image_free(pixels);
}

bool loadingUtil::loadObj(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices, const std::string modelPath)
{
	std::string str = "../../src/Assets/Models/";
	str.append(modelPath);
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
	std::cout << "An obj file was loaded" << std::endl;
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
	const bool hasColor = attrib.colors.size() > 0;

	if (!hasPosition)
	{
		throw std::runtime_error("failed to load obj!");
	}

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
				vertex.texCoord = {
					static_cast<const float>(attrib.texcoords[2 * index.texcoord_index + 0]),
					static_cast<const float>(1.0f - attrib.texcoords[2 * index.texcoord_index + 1])
				};
			}
			else
			{
				vertex.texCoord = { 1.0, 1.0 };
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
				vertex.normal = { 0.0, 1.0, 0.0 };
			}

			if (hasColor)
			{
				vertex.color = {
					static_cast<const float>(attrib.colors[3 * index.vertex_index + 0]),
					static_cast<const float>(attrib.colors[3 * index.vertex_index + 1]),
					static_cast<const float>(attrib.colors[3 * index.vertex_index + 2]),
					1.0
				};
			}
			else
			{
				vertex.color = { 1.0, 0.0, 0.0, 0.5 };
			}

			if (uniqueVertices.count(vertex) == 0)
			{
				uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
				vertices.push_back(vertex);
			}

			indices.push_back(uniqueVertices[vertex]);
		}
	}

	return true;
}

bool loadingUtil::loadGLTF()
{
	return false;
}