#include "camera.h"

Camera::Camera(std::shared_ptr<VulkanManager> vulkanManager, glm::vec3 eyePos, glm::vec3 lookAtPoint, int width, int height,
	float foV_vertical, float aspectRatio, float nearClip, float farClip, int numSwapChainImages, CameraMode mode, RENDER_TYPE renderType)
	: m_vulkanManager(vulkanManager), m_logicalDevice(m_vulkanManager->getLogicalDevice()), m_physicalDevice(m_vulkanManager->getPhysicalDevice()),
	m_numSwapChainImages(numSwapChainImages), m_mode(mode), m_renderType(renderType),
	m_eyePos(eyePos), m_ref(lookAtPoint), m_width(width), m_height(height),
	m_fovy(foV_vertical), m_aspect(aspectRatio), m_near_clip(nearClip), m_far_clip(farClip)
{
	m_worldUp = glm::vec3(0, 1, 0);
	recomputeAttributes();

	m_cameraUniforms.resize(m_numSwapChainImages);
	for (int i = 0; i < numSwapChainImages; i++)
	{
		BufferUtil::createMageBuffer(m_logicalDevice, m_physicalDevice,	m_cameraUniforms[i].cameraUB, 
			sizeof(CameraUniformBlock),	nullptr, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
		updateUniformBuffer(i);
		m_cameraUniforms[i].cameraUB.map(m_logicalDevice);
		m_cameraUniforms[i].cameraUB.copyDataToMappedBuffer(&m_cameraUniforms[i].uniformBlock);
	}
}

Camera::Camera(std::shared_ptr<VulkanManager> vulkanManager, JSONItem::Camera& jsonCam, int numSwapChainImages, CameraMode mode, RENDER_TYPE renderType)
	: Camera( vulkanManager, jsonCam.eyePos, jsonCam.lookAtPoint, jsonCam.width, jsonCam.height,
		jsonCam.foV_vertical, jsonCam.aspectRatio, jsonCam.nearClip, jsonCam.farClip, numSwapChainImages, mode, renderType)
{};

Camera::~Camera()
{
	vkDeviceWaitIdle(m_logicalDevice);

	vkDestroyDescriptorSetLayout(m_logicalDevice, m_DSL_camera, nullptr);
	for (unsigned int i = 0; i < m_numSwapChainImages; i++)
	{
		m_cameraUniforms[i].cameraUB.unmap(m_logicalDevice);
		m_cameraUniforms[i].cameraUB.destroy(m_logicalDevice);
	}
}

void Camera::updateUniformBuffer(unsigned int bufferIndex)
{
	CameraUniformBlock& l_cameraUBO = m_cameraUniforms[bufferIndex].uniformBlock;
	l_cameraUBO.view = getView();
	l_cameraUBO.proj = getProj();
	
	//Reason for flipping the y axis: https://matthewwellings.com/blog/the-new-vulkan-coordinate-system/
	l_cameraUBO.proj[1][1] *= -1; // y-coordinate is flipped
	l_cameraUBO.proj[2][2] *= 0.5; // correct depth range
	l_cameraUBO.proj[2][3] *= 0.5; // correct depth range

	l_cameraUBO.viewInverse = glm::inverse(l_cameraUBO.view);
	l_cameraUBO.projInverse = glm::inverse(l_cameraUBO.proj);
	l_cameraUBO.eyePos = glm::vec4(m_eyePos, 1.0);

	l_cameraUBO.tanFovBy2.y = static_cast<float>(std::abs(std::tan(m_fovy* 0.5 * (PI / 180.0))));
	l_cameraUBO.tanFovBy2.x = m_aspect * l_cameraUBO.tanFovBy2.y;
}
void Camera::updateUniformBuffer(Camera* cam, unsigned int dstCamBufferIndex, unsigned int srcCamBufferIndex)
{
	CameraUniformBlock& l_cameraUBO = m_cameraUniforms[dstCamBufferIndex].uniformBlock;
	l_cameraUBO.view = cam->m_cameraUniforms[srcCamBufferIndex].uniformBlock.view;
	l_cameraUBO.proj = cam->m_cameraUniforms[srcCamBufferIndex].uniformBlock.proj;
	l_cameraUBO.viewInverse = cam->m_cameraUniforms[srcCamBufferIndex].uniformBlock.viewInverse;
	l_cameraUBO.projInverse = cam->m_cameraUniforms[srcCamBufferIndex].uniformBlock.projInverse;
	l_cameraUBO.eyePos = cam->m_cameraUniforms[srcCamBufferIndex].uniformBlock.eyePos;
	l_cameraUBO.tanFovBy2 = cam->m_cameraUniforms[srcCamBufferIndex].uniformBlock.tanFovBy2;
}
void Camera::copyToGPUMemory(unsigned int bufferIndex)
{
	m_cameraUniforms[bufferIndex].cameraUB.copyDataToMappedBuffer(&m_cameraUniforms[bufferIndex].uniformBlock);
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
	for (uint32_t i = 0; i < m_numSwapChainImages; i++)
	{
		DescriptorUtil::createDescriptorSets(m_logicalDevice, descriptorPool, 1, &m_DSL_camera, &m_cameraUniforms[i].DS_camera);
	}
}
void Camera::writeToAndUpdateDescriptorSets()
{
	for (uint32_t i = 0; i < m_numSwapChainImages; i++)
	{
		VkWriteDescriptorSet writecameraSetInfo = DescriptorUtil::writeDescriptorSet(
				m_cameraUniforms[i].DS_camera, 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &m_cameraUniforms[i].cameraUB.descriptorInfo);
		vkUpdateDescriptorSets(m_logicalDevice, 1, &writecameraSetInfo, 0, nullptr);
	}
}