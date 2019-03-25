#pragma once
#include <global.h>

struct Vertex
{
	glm::vec4 position;
	glm::vec4 color;
};

struct ModelUBO
{
	glm::mat4 modelMatrix;
};