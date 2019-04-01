#include "camera.h"

Camera::Camera(glm::vec3 eyePos, glm::vec3 lookAtPoint, int width, int height,
			   float foV_vertical, float aspectRatio, float nearClip, float farClip)
	: m_eyePos(eyePos), m_ref(lookAtPoint), m_width(width), m_height(height),
	m_fovy(foV_vertical), m_aspect(aspectRatio), m_near_clip(nearClip), m_far_clip(farClip)
{
	m_worldUp = glm::vec3(0,1,0);
	recomputeAttributes();
}

Camera::~Camera() 
{
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