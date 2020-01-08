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

#include <Utilities/timerUtility.h>

#ifdef _DEBUG
#define DEBUG
#define DEBUG_GRAPHICS_PLAYGROUND
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
enum class PIPELINE_TYPE { RASTER, FINAL_COMPOSITE, COMPUTE, POST_PROCESS };
enum class DSL_TYPE { COMPUTE, MODEL, CURRENT_FRAME_CAMERA, PREV_FRAME_CAMERA, TIME, FINAL32BITIMAGE, POST_PROCESS, FINAL_COMPOSITE };