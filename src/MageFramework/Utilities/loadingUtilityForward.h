#pragma once
#include <unordered_map>
#include <global.h>

enum class FILE_TYPE { OBJ, GLTF };

namespace JSONItem
{
	struct Model
	{
		std::string name;
		std::string meshPath;
		FILE_TYPE filetype;
		std::vector<std::string> texturePaths;
		std::string material;
		glm::mat4 transform;
	};

	struct Camera
	{
		glm::vec3 eyePos;
		glm::vec3 lookAtPoint;
		float foV_vertical;
		int width;
		int height;
		float aspectRatio;
		float nearClip;
		float farClip;
		float focalDistance;
		float lensRadius;
	};

	struct Scene
	{
		std::vector<JSONItem::Model> modelList;
	};
} // namespace JSONItem

struct JSONContents
{
	JSONItem::Camera mainCamera;
	JSONItem::Scene scene;
};

struct ImageLoaderOutput
{
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	int imgWidth, imgHeight;
};

struct ImageArrayLoaderOutput
{
	enum class IMAGE_USAGE { COLOR, NORMAL, EMISSIVE };

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	std::vector<IMAGE_USAGE> imgUsageTypes;
	std::vector<uint32_t> imgWidths;
	std::vector<uint32_t> imgHeights;
};