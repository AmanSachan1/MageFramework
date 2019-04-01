#pragma once

#include <global.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/glm.hpp>

//#include "VulkanDevice.h"
//#include "BufferUtils.h"

#define PI 3.14159

struct CameraUBO {
	glm::mat4 view;
	glm::mat4 proj;
	glm::vec4 eyePos; //pad vec3s with extra float to make them vec4s so vulkan can do offsets correctly
	glm::vec2 tanFovBy2; //vec2 and vec4 are acceptable for offseting; 
	//stored as .x = horizontalFovBy2 and .y = verticalFovBy2
};

class Camera 
{
public:
	Camera() = delete;	// https://stackoverflow.com/questions/5513881/meaning-of-delete-after-function-declaration
	Camera(glm::vec3 eyePos, glm::vec3 ref, int width, int height,
		   float foV_vertical, float aspectRatio, float nearClip, float farClip);
	~Camera();

	VkBuffer getBuffer() const;
	void updateBuffer();
	void updateBuffer(Camera* cam);
	void copyToGPUMemory();

	glm::mat4 getView() const;
	glm::mat4 getProj() const;
	glm::mat4 getViewProj() const;
	void recomputeAttributes();

	void rotateAboutUp(float deg);
	void rotateAboutRight(float deg);

	void translateAlongLook(float amt);
	void translateAlongRight(float amt);
	void translateAlongUp(float amt);

private:
	//VulkanDevice* device; //member variable because it is needed for the destructor

	//CameraUBO cameraUBO;
	//VkBuffer buffer;
	//VkDeviceMemory bufferMemory;

	//void* mappedData;

	glm::vec3 m_eyePos;
	glm::vec3 m_ref;      //The point in world space towards which the camera is pointing
			  
	glm::vec3 m_forward;
	glm::vec3 m_right;
	glm::vec3 m_up;
	glm::vec3 m_worldUp;

	int m_width, m_height;

	float m_fovy;
	float m_aspect;
	float m_near_clip;  // Near clip plane distance
	float m_far_clip;  // Far clip plane distance
};