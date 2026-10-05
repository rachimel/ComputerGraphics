#include "Camera.h"

#include <glm/gtc/matrix_transform.hpp>
Camera::Camera(const glm::vec3 pos, const glm::vec3& worldUp, float fovy)
	: m_Pos{pos}, m_WorldUp{worldUp}, m_Fovy{fovy}
{
}

void Camera::RefWorldUp(const glm::vec3& worldUp)
{
	m_WorldUp = worldUp;
}

void Camera::Zoom(float fovy)
{
	m_Fovy = fovy;
}

void Camera::PlaceAt(const glm::vec3& pos)
{
	m_Pos = pos;
}

void Camera::FocusAt(const glm::vec3& vec)
{
	glm::vec3 d = glm::normalize(vec - m_Pos);
	m_Yaw = glm::degrees(glm::atan(d.z,d.x));
	m_Pitch = glm::degrees(glm::atan(d.y ,glm::length(glm::vec2(d.x, d.z))));

	UpdateCameraBasis();
}

void Camera::OrientAt(float yaw, float pitch)
{
	m_Yaw = yaw;
	m_Pitch = pitch;
	UpdateCameraBasis();
}

void Camera::Rotate(float yawDelta /*= 0.0f*/, float pitchDelta /*= 0.0f*/)
{
	m_Yaw += yawDelta * m_Sensitivity;
	m_Pitch = glm::clamp(m_Pitch + (pitchDelta * m_Sensitivity), -89.0f, 89.0f);

	UpdateCameraBasis();
}

void Camera::Move(CameraDir dir, float dt)
{
	switch (dir)
	{
	case CameraDir::Up:
		m_Pos += m_Up * dt * m_Speed;
		break;
	case CameraDir::Down:
		m_Pos -= m_Up * dt * m_Speed;
		break;
	case CameraDir::Right:
		m_Pos += m_Right * dt * m_Speed;
		break;
	case CameraDir::Left:
		m_Pos -= m_Right * dt * m_Speed;
		break;
	}
}

glm::mat4 Camera::ProjectionMatrix(float aspect, float zNear, float zFar) const
{
	return glm::perspective(glm::radians(m_Fovy), aspect, zNear, zFar);
}

glm::mat4 Camera::ViewMatrix() const
{
	return glm::lookAt(m_Pos, m_Pos + m_Front, m_WorldUp);
}

void Camera::ChangeSpeed(float speed)
{
	m_Speed = speed;
}

void Camera::ChangeSensitivity(float sensitivity)
{
	m_Sensitivity = sensitivity;
}

void Camera::UpdateCameraBasis()
{
	glm::vec3 lookDir{};
	lookDir.x = glm::cos(glm::radians(m_Yaw)) * glm::cos(glm::radians(m_Pitch));
	lookDir.y = glm::sin(glm::radians(m_Pitch));
	lookDir.z = glm::sin(glm::radians(m_Yaw)) * glm::cos(glm::radians(m_Pitch));

	m_Front = glm::normalize(lookDir);
	m_Right = glm::normalize(glm::cross(m_Front, m_WorldUp));
	m_Up = glm::normalize(glm::cross(m_Right, m_Front));
	m_Up.y = 0.0f;
	m_Up = glm::normalize(m_Up);
}
