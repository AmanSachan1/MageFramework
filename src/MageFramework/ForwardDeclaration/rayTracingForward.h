#pragma once
#include <global.h>

// Ray tracing acceleration structure
struct AccelerationStructure
{
	VkAccelerationStructureNV accelerationStructure;
	VkDeviceMemory memory;
	uint64_t handle;
};

// Ray tracing geometry instance
struct GeometryInstance
{
	glm::mat3x4	transform;
	uint32_t instanceId : 24;
	uint32_t mask : 8;
	uint32_t instanceOffset : 24;
	uint32_t flags : 8;
	uint64_t accelerationStructureHandle;
};

// Indices for the different ray tracing shader types used in this project
#define INDEX_RAYGEN 0
#define INDEX_MISS 1
#define INDEX_SHADOW_MISS 2
#define INDEX_CLOSEST_HIT 3
#define INDEX_SHADOW_HIT 4
#define NUM_SHADER_GROUPS 5

static const uint32_t MAX_RECURSION_DEPTH = 2;