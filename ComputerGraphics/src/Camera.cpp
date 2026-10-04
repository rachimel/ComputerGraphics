#include "Camera.h"

#include <glm/gtc/matrix_transform.hpp>
Camera::Camera(const glm::vec3 pos, const glm::vec3& worldUp, float fovy)
	: m_Pos{pos}, m_WorldUp{worldUp}, m_Fovy{fovy}
{
	UpdateViewMatrix();
}

void Camera::RefWorldUp(const glm::vec3& worldUp)
{
	m_WorldUp = worldUp;
	UpdateViewMatrix();
}

void Camera::Zoom(float fovy)
{
	m_Fovy = fovy;
}

void Camera::PlaceAt(const glm::vec3& pos)
{
	m_Pos = pos;
	UpdateViewMatrix();
}

void Camera::OrientAt(float yaw, float pitch)
{
	m_Yaw = yaw;
	m_Pitch = pitch;
	UpdateViewMatrix();
}

void Camera::Rotate(float yawDelta /*= 0.0f*/, float pitchDelta /*= 0.0f*/)
{
	m_Yaw += yawDelta;
	m_Pitch = glm::clamp(m_Pitch + pitchDelta, -89.0f, 89.0f);
	UpdateViewMatrix();
}

glm::mat4 Camera::ProjectionMatrix(float aspect, float zNear, float zFar) const
{
	return glm::perspective(m_Fovy, aspect, zNear, zFar);
}

glm::mat4 Camera::ViewMatrix() const
{
	return m_ViewMatrix;
}

void Camera::UpdateViewMatrix()
{
	glm::vec3 lookDir{};
	lookDir.x = glm::cos(glm::radians(m_Yaw)) * glm::cos(glm::radians(m_Pitch));
	lookDir.y = glm::sin(glm::radians(m_Pitch));
	lookDir.z = glm::sin(glm::radians(m_Yaw)) * glm::cos(glm::radians(m_Pitch));
	m_ViewMatrix = glm::lookAt(m_Pos, m_Pos + lookDir, m_WorldUp);
}
