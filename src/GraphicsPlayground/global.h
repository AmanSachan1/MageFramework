#pragma once
#include <stdio.h>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <array>
#include <set>
#include <bitset>

#include <chrono>

#include <vulkan/vulkan.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

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

// Timer
static std::chrono::time_point<std::chrono::steady_clock> startTime;
inline void initTimer()
{
	std::cout << "Starting Timer \n";
	startTime = std::chrono::high_resolution_clock::now();
}
