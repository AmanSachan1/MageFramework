#pragma once
#include <stdio.h>
#include <assert.h>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <array>
#include <set>
#include <bitset>

#include <vulkan/vulkan.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>

// Disable warning C26812: Prefer 'enum class' over 'enum' (Enum.3); Because of Vulkan
#pragma warning( disable : 26812 )

#include <Utilities/timerUtility.h>

#ifdef _DEBUG
#define DEBUG
#define DEBUG_MAGE_FRAMEWORK
#endif // _DEBUG

const static unsigned int MAX_FRAMES_IN_FLIGHT = 3;

#define TIME_POINT std::chrono::high_resolution_clock::time_point

enum QueueFlags
{
	Graphics,
	Compute,
	Transfer,
	Present,
};

enum QueueFlagBit
{
	GraphicsBit = 1 << 0,
	ComputeBit = 1 << 1,
	TransferBit = 1 << 2, 	// TransferBit tells Vulkan that we can transfer data between CPU and GPU
	PresentBit = 1 << 3,
};

using QueueFlagBits = std::bitset<sizeof(QueueFlags)>;
using QueueFamilyIndices = std::array<uint32_t, sizeof(QueueFlags)>;
using Queues = std::array<VkQueue, sizeof(QueueFlags)>;

enum class RenderAPI { VULKAN, DX12 };
enum class PIPELINE_TYPE { RASTER, COMPOSITE_COMPUTE_ONTO_RASTER, COMPUTE, POST_PROCESS };
enum class POST_PROCESS_TYPE { HIGH_RESOLUTION, TONEMAP, LOW_RESOLUTION };
enum class DSL_TYPE { COMPUTE, MODEL, CURRENT_FRAME_CAMERA, PREV_FRAME_CAMERA, TIME, COMPOSITE_COMPUTE_ONTO_RASTER, 
	POST_PROCESS, BEFOREPOST_FRAME, POST_HRFRAME1, POST_HRFRAME2, POST_LRFRAME1, POST_LRFRAME2};