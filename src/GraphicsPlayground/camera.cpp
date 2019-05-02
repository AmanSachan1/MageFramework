#include "camera.h"

Camera::Camera(VulkanDevices* devices, unsigned int numSwapChainImages,
	glm::vec3 eyePos, glm::vec3 lookAtPoint, int width, int height,
	float foV_vertical, float aspectRatio, float nearClip, float farClip)
	: m_devices(devices), m_logicalDevice(devices->getLogicalDevice()), m_physicalDevice(devices->getPhysicalDevice()), 
	m_numSwapChainImages(numSwapChainImages), 
	m_eyePos(eyePos), m_ref(lookAtPoint), m_width(width), m_height(height),
	m_fovy(foV_vertical), m_aspect(aspectRatio), m_near_clip(nearClip), m_far_clip(farClip)
{
	m_worldUp = glm::vec3(0,1,0);
	recomputeAttributes();

	m_uniformBufferSize = sizeof(CameraUBO);
	m_uniformBuffers.resize(m_numSwapChainImages);
	m_uniformBufferMemories.resize(m_numSwapChainImages);
	m_cameraUBOs.resize(m_numSwapChainImages);
	m_mappedDataUniformBuffers.resize(m_numSwapChainImages);

	BufferUtil::createUniformBuffers(m_devices, m_physicalDevice, m_logicalDevice, m_numSwapChainImages,
		m_uniformBuffers, m_uniformBufferMemories, m_uniformBufferSize);

	for (unsigned int i = 0; i < numSwapChainImages; i++)
	{
		updateUniformBuffer(i);
		vkMapMemory(m_logicalDevice, m_uniformBufferMemories[i], 0, m_uniformBufferSize, 0, &m_mappedDataUniformBuffers[i]);
		memcpy(m_mappedDataUniformBuffers[i], &m_cameraUBOs[i], (size_t)m_uniformBufferSize);
	}
}

Camera::~Camera() 
{
	for (unsigned int i = 0; i < m_numSwapChainImages; i++)
	{
		vkUnmapMemory(m_logicalDevice, m_uniformBufferMemories[i]);
		vkDestroyBuffer(m_logicalDevice, m_uniformBuffers[i], nullptr);
		vkFreeMemory(m_logicalDevice, m_uniformBufferMemories[i], nullptr);
	}
}


VkBuffer Camera::getUniformBuffer(unsigned int bufferIndex) const
{
	return m_uniformBuffers[bufferIndex];
}
uint32_t Camera::getUniformBufferSize() const
{
	return static_cast<uint32_t>(sizeof(CameraUBO));
}

void Camera::updateUniformBuffer(unsigned int bufferIndex)
{
	m_cameraUBOs[bufferIndex].view = getView();
	m_cameraUBOs[bufferIndex].proj = getProj();
	//Reason for flipping the y axis: https://matthewwellings.com/blog/the-new-vulkan-coordinate-system/
	m_cameraUBOs[bufferIndex].proj[1][1] *= -1; // y-coordinate is flipped

	m_cameraUBOs[bufferIndex].eyePos = glm::vec4(m_eyePos, 1.0);

	m_cameraUBOs[bufferIndex].tanFovBy2.y = static_cast<float>(std::abs(std::tan(m_fovy* 0.5 * (PI / 180.0))));
	m_cameraUBOs[bufferIndex].tanFovBy2.x = m_aspect * m_cameraUBOs[bufferIndex].tanFovBy2.y;
}
void Camera::updateUniformBuffer(Camera* cam, unsigned int dstCamBufferIndex, unsigned int srcCamBufferIndex)
{
	m_cameraUBOs[dstCamBufferIndex].view = cam->m_cameraUBOs[srcCamBufferIndex].view;
	m_cameraUBOs[dstCamBufferIndex].proj = cam->m_cameraUBOs[srcCamBufferIndex].proj;
	m_cameraUBOs[dstCamBufferIndex].eyePos = cam->m_cameraUBOs[srcCamBufferIndex].eyePos;
	m_cameraUBOs[dstCamBufferIndex].tanFovBy2 = cam->m_cameraUBOs[srcCamBufferIndex].tanFovBy2;
}
void Camera::copyToGPUMemory(unsigned int bufferIndex)
{
	memcpy(m_mappedDataUniformBuffers[bufferIndex], &m_cameraUBOs[bufferIndex], sizeof(CameraUBO));
}

glm::mat4 Camera::getViewProj() const
{
	//static casts
	return glm::perspective(glm::radians(m_fovy), m_width / (float)m_height, m_near_clip, m_far_clip) * glm::lookAt(m_eyePos, m_ref, m_up);
}
glm::mat4 Camera::getView() const
{
	return glm::lookAt(m_eyePos, m_ref, m_up);
}
glm::mat4 Camera::getProj() const
{
	return glm::perspective(glm::radians(m_fovy), m_width / (float)m_height, m_near_clip, m_far_clip);
}
void Camera::recomputeAttributes()
{
	m_forward = glm::normalize(m_ref - m_eyePos);
	m_right = glm::normalize(glm::cross(m_forward, m_worldUp));
	m_up = glm::cross(m_right, m_forward);

	float tan_fovy = tan(glm::radians(m_fovy / 2));
	float len = glm::length(m_ref - m_eyePos);
	m_aspect = m_width / (float)m_height;
}

void Camera::rotateAboutUp(float deg)
{
	deg = glm::radians(deg);
	glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), deg, m_up);
	m_ref = m_ref - m_eyePos;
	m_ref = glm::vec3(rotation * glm::vec4(m_ref, 1));
	m_ref = m_ref + m_eyePos;
	recomputeAttributes();
}
void Camera::rotateAboutRight(float deg)
{
	deg = glm::radians(deg);
	glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), deg, m_right);
	m_ref = m_ref - m_eyePos;
	m_ref = glm::vec3(rotation * glm::vec4(m_ref, 1));
	m_ref = m_ref + m_eyePos;
	recomputeAttributes();
}

void Camera::translateAlongLook(float amt)
{
	glm::vec3 translation = m_forward * amt;
	m_eyePos += translation;
	m_ref += translation;
	recomputeAttributes();
}
void Camera::translateAlongRight(float amt)
{
	glm::vec3 translation = m_right * amt;
	m_eyePos += translation;
	m_ref += translation;
	recomputeAttributes();
}
void Camera::translateAlongUp(float amt)
{
	glm::vec3 translation = m_up * amt;
	m_eyePos += translation;
	m_ref += translation;
	recomputeAttributes();
}