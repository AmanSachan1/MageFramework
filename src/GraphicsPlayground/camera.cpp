#include "camera.h"

Camera::Camera(std::shared_ptr<VulkanManager> vulkanObject, glm::vec3 eyePos, glm::vec3 lookAtPoint, int width, int height,
	float foV_vertical, float aspectRatio, float nearClip, float farClip, int numSwapChainImages, CameraMode mode)
	: m_vulkanObj(vulkanObject), m_logicalDevice(m_vulkanObj->getLogicalDevice()), m_physicalDevice(m_vulkanObj->getPhysicalDevice()),
	m_numSwapChainImages(numSwapChainImages), m_mode(mode),
	m_eyePos(eyePos), m_ref(lookAtPoint), m_width(width), m_height(height),
	m_fovy(foV_vertical), m_aspect(aspectRatio), m_near_clip(nearClip), m_far_clip(farClip)
{
	m_worldUp = glm::vec3(0, 1, 0);
	recomputeAttributes();

	m_uniformBufferSize = sizeof(CameraUBO);
	m_uniformBuffers.resize(m_numSwapChainImages);
	m_uniformBufferMemories.resize(m_numSwapChainImages);
	m_cameraUBOs.resize(m_numSwapChainImages);
	m_mappedDataUniformBuffers.resize(m_numSwapChainImages);

	BufferUtil::createUniformBuffers(m_logicalDevice, m_physicalDevice, m_numSwapChainImages,
		m_uniformBuffers, m_uniformBufferMemories, m_uniformBufferSize);

	for (int i = 0; i < numSwapChainImages; i++)
	{
		updateUniformBuffer(i);
		vkMapMemory(m_logicalDevice, m_uniformBufferMemories[i], 0, m_uniformBufferSize, 0, &m_mappedDataUniformBuffers[i]);
		memcpy(m_mappedDataUniformBuffers[i], &m_cameraUBOs[i], (size_t)m_uniformBufferSize);
	}
}

Camera::~Camera()
{
	vkDeviceWaitIdle(m_logicalDevice);

	vkDestroyDescriptorSetLayout(m_logicalDevice, m_DSL_camera, nullptr);

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

void Camera::switchCameraMode()
{
	if (m_mode == CameraMode::FLY)
	{
		m_mode = CameraMode::ORBIT;
		std::cout << "Camera is in ORBIT mode \n";
	}
	else if (m_mode == CameraMode::ORBIT)
	{
		m_mode = CameraMode::FLY;
		std::cout << "Camera is in FLY mode \n";
	}
}

glm::mat4 Camera::getViewProj() const
{
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

	if (m_mode == CameraMode::FLY)
	{
		m_ref = m_ref - m_eyePos;
		m_ref = glm::vec3(rotation * glm::vec4(m_ref, 1));
		m_ref = m_ref + m_eyePos;
	}
	else if (m_mode == CameraMode::ORBIT)
	{
		glm::mat4 translation(1.0f);
		glm::mat4 m = glm::translate(translation, m_ref) * rotation * glm::translate(translation, -m_ref);
		m_eyePos = glm::vec3(m * glm::vec4(m_eyePos, 1));
	}

	recomputeAttributes();
}
void Camera::rotateAboutRight(float deg)
{
	deg = glm::radians(deg);
	glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), deg, m_right);

	if (m_mode == CameraMode::FLY)
	{
		m_ref = m_ref - m_eyePos;
		m_ref = glm::vec3(rotation * glm::vec4(m_ref, 1));
		m_ref = m_ref + m_eyePos;
	}
	else if (m_mode == CameraMode::ORBIT)
	{
		glm::mat4 translation(1.0f);
		glm::mat4 m = glm::translate(translation, m_ref) * rotation * glm::translate(translation, -m_ref);
		m_eyePos = glm::vec3(m * glm::vec4(m_eyePos, 1));
	}

	recomputeAttributes();
}

void Camera::translateAlongLook(float amt)
{
	glm::vec3 translation = m_forward * amt;
	m_eyePos += translation;

	if (m_mode == CameraMode::FLY)
	{
		m_ref += translation;
	}
	else if ((m_mode == CameraMode::ORBIT) &&
		(glm::dot(m_ref - m_eyePos, m_forward) < 0.0f))
	{
		m_eyePos -= translation;
	}

	recomputeAttributes();
}
void Camera::translateAlongRight(float amt)
{
	glm::vec3 translation = m_right * amt;
	m_eyePos += translation;

	if (m_mode == CameraMode::FLY)
	{
		m_ref += translation;
	}
	else if ((m_mode == CameraMode::ORBIT) &&
		(glm::dot(m_ref - m_eyePos, m_forward) < 0.0f))
	{
		m_eyePos -= translation;
	}

	recomputeAttributes();
}
void Camera::translateAlongUp(float amt)
{
	glm::vec3 translation = m_up * amt;
	m_eyePos += translation;

	if (m_mode == CameraMode::FLY)
	{
		m_ref += translation;
	}
	else if ((m_mode == CameraMode::ORBIT) &&
		(glm::dot(m_ref - m_eyePos, m_forward) < 0.0f))
	{
		m_eyePos -= translation;
	}

	recomputeAttributes();
}

void Camera::expandDescriptorPool(std::vector<VkDescriptorPoolSize>& poolSizes)
{
	poolSizes.push_back({ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, m_numSwapChainImages });	// Current Camera State
}
void Camera::createDescriptors(VkDescriptorPool descriptorPool)
{
	// Descriptor Set Layouts
	{
		// Descriptor set layouts are specified in the pipeline layout object., i.e. during pipeline creation to tell Vulkan 
		// which descriptors the shaders will be using.
		// The numbers are bindingCount, binding, and descriptorCount respectively
		VkDescriptorSetLayoutBinding cameraSetBinding = { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_ALL, nullptr };
		DescriptorUtil::createDescriptorSetLayout(m_logicalDevice, m_DSL_camera, 1, &cameraSetBinding);
	}

	// Descriptor Sets
	m_DS_camera.resize(m_numSwapChainImages);
	for (uint32_t i = 0; i < m_numSwapChainImages; i++)
	{
		DescriptorUtil::createDescriptorSets(m_logicalDevice, descriptorPool, 1, &m_DSL_camera, &m_DS_camera[i]);
	}
}

void Camera::writeToAndUpdateDescriptorSets()
{
	uint32_t cameraUniformBufferSize = static_cast<uint32_t>(sizeof(CameraUBO));
	for (uint32_t i = 0; i < m_numSwapChainImages; i++)
	{
		{
			VkDescriptorBufferInfo cameraBufferSetInfo =
				DescriptorUtil::createDescriptorBufferInfo(getUniformBuffer(i), 0, cameraUniformBufferSize);
			VkWriteDescriptorSet writecameraSetInfo =
				DescriptorUtil::writeDescriptorSet(m_DS_camera[i], 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &cameraBufferSetInfo);

			vkUpdateDescriptorSets(m_logicalDevice, 1, &writecameraSetInfo, 0, nullptr);
		}
	}
}

VkDescriptorSet Camera::getDescriptorSet(DSL_TYPE type, int index)
{
	return m_DS_camera[index];
}
VkDescriptorSetLayout Camera::getDescriptorSetLayout(DSL_TYPE key)
{
	return m_DSL_camera;
}