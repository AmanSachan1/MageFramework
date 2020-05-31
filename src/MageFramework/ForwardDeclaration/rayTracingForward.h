#pragma once
#include <global.h>

// Indices for the different ray tracing shader types used in this project
#define INDEX_RAYGEN 0
#define INDEX_MISS 1
#define INDEX_SHADOW_MISS 2
#define INDEX_CLOSEST_HIT 3
#define INDEX_SHADOW_HIT 4
#define NUM_SHADER_GROUPS 5

static const uint32_t MAX_RECURSION_DEPTH = 2;

// Ray tracing geometry instance
struct GeometryInstance
{
	/// Transform matrix, containing only the top 3 rows
	glm::mat3x4	transform;
	/// Instance index
	uint32_t instanceId : 24;
	/// Visibility mask
	uint32_t mask : 8;
	/// Index of the hit group which will be invoked when a ray hits the instance
	uint32_t instanceOffset : 24;
	/// Instance flags, such as culling
	uint32_t flags : 8;
	/// Opaque handle of the bottom-level acceleration structure
	uint64_t accelerationStructureHandle;
};