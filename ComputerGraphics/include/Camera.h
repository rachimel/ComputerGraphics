#pragma once
#include <glm/glm.hpp>
#include <Alias.h>
enum class CameraDir
{
	Up,
	Down,
	Right,
	Left,
	Ascend,
	Descend,
};
class Camera
{
public:
	Camera() = default;
	Camera(const glm::vec3& pos, const glm::vec3& worldUp, float fovy,float near, float far);

	Camera(const Camera&) = default;
	Camera& operator= (const Camera&) = default;

	void RefWorldUp(const glm::vec3& worldUp);
	void Zoom(float fovy);
	void PlaceAt(const glm::vec3& pos);

	void FocusAt(const glm::vec3& vec);
	void OrientAt(float yaw = 0.0f, float pitch = 0.0f);
	void Rotate(float yawDelta = 0.0f, float pitchDelta = 0.0f);

	void Move(CameraDir dir, float dt);
	void Move(const glm::vec3& delta);

	glm::mat4 ProjectionMatrix(float aspect) const;
	glm::mat4 ViewMatrix() const;

	void ChangeSpeed(float speed);
	void ChangeSensitivity(float sensitivity);

	float QueryPitch() const;
	float QueryYaw() const;

	glm::vec3 QueryPos() const;
private:
	void UpdateCameraBasis();
private:
	float m_Near{ 0.0f };
	float m_Far{ 0.0f };

	float m_Speed{5.0f};
	float m_Sensitivity{0.05f};
	float m_Fovy{}; // vertical field of view

	float m_Pitch{};
	float m_Yaw{};

	glm::vec3 m_Pos{}; // eye
	glm::vec3 m_WorldUp{}; // world up vector

	glm::vec3 m_Front{};
	glm::vec3 m_Up{};
	glm::vec3 m_Right{};

};

