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

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_access.hpp>

#include <Utilities/timerUtility.h>
#include <ForwardDeclaration/vulkanForward.h>
#include <ForwardDeclaration/renderforward.h>
#include <ForwardDeclaration/rayTracingForward.h>

// Disable Warnings: 
#pragma warning( disable : 26495 ) // C26495: Uninitialized variable;
#pragma warning( disable : 26451 ) // C26451: Arithmetic Overflow;
#pragma warning( disable : 26812 ) // C26812: Prefer 'enum class' over 'enum' (Enum.3); Because of Vulkan 

#ifdef _DEBUG
#define DEBUG
#define DEBUG_MAGE_FRAMEWORK
#endif // _DEBUG

// Default fence timeout in nanoseconds
#define DEFAULT_FENCE_TIMEOUT 100000000000

const static float PI = 3.14159f;