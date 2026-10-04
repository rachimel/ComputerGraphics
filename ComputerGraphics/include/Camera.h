#pragma once
#include <glm/glm.hpp>

class Camera
{
public:
	Camera() = default;
	Camera(const glm::vec3 pos, const glm::vec3& worldUp, float fovy);

	void RefWorldUp(const glm::vec3& worldUp);
	void Zoom(float fovy);
	void PlaceAt(const glm::vec3& pos);
	void OrientAt(float yaw = 0.0f, float pitch = 0.0f);
	void Rotate(float yawDelta = 0.0f, float pitchDelta = 0.0f);

	glm::mat4 ProjectionMatrix(float aspect, float zNear, float zFar) const;
	glm::mat4 ViewMatrix() const;
private:
	void UpdateViewMatrix();
private:
	float m_Yaw{};
	float m_Pitch{};
	float m_Fovy{}; // vertical field of view
	glm::vec3 m_Pos{}; // eye
	glm::vec3 m_WorldUp{}; // world up vector

	glm::mat4 m_ViewMatrix{};
};

