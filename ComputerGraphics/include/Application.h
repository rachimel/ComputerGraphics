#pragma once

struct GLFWwindow;

#include <string_view>
#include <glm/glm.hpp>
#include <memory>

class Application
{
public:
	Application(int width, int height);
	~Application();
	// You can override the following callbacks by defining macro before including the Application Header:
	// APPLICATION_CUSTOM_CALLBACK_(CALLBACK_NAME)
#if defined(CG_APPLICATION_CUSTOM_CALLBACK_MOUSE_BUTTON)
	void OnMouseButtonEvent(GLFWwindow* window, int key, int action, int mods);
#endif
#if defined (CG_APPLICATION_CUSTOM_CALLBACK_CURSOR_POS)
	void OnCursorMoveEvent(GLFWwindow* window, float xPos, float yPos);
#endif
#if defined (CG_APPLICATION_CUSTOM_CALLBACK_KEY)
#define CG_APPLICATION_CUSTOM_CALLBACK_KEY_IMPL
	void OnKeyEvent(GLFWwindow* window, int key, int action, int mods);
#endif
#if defined (CG_APPLICATION_CUSTOM_CALLBACK_CHAR)
	void OnCharEvent(GLFWwindow* window, unsigned int codepoint);
#endif
	// FrameBuffer callback override is currently not supported.

// Basic Methods
	int Init(std::string_view title);
	void Run();

	void PollInputs();
	void CaptureMouse();
	void EnableOpenGLFeatures(unsigned int features);
	void DisableOpenGLFeatures(unsigned int features);

	void CreateCamera(const glm::vec3& pos, const glm::vec3& worldUp = glm::vec3(0.0f, 1.0f, 0.0f), float fovy = 45.0f, float yaw = 0.0f, float pitch = 0.0f);
	void SetNearPlane(float zNear);
	void SetFarPlane(float zFar);
// Override Methods
	void OnInit();
	void Update();
	void Render();
#if defined(CG_APPLICATION_CUSTOM_POLL_INPUT)
	void OnInputPoll();
#endif
// Callback entry & default callback handlers
private:
	static void MouseButtonCallbackEntry(GLFWwindow* window, int button, int action, int mods);
	void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);

	static void CursorPosCallbackEntry(GLFWwindow* window, double xPos, double yPos);
	void CursorPosCallback(GLFWwindow* window, float xPos, float yPos);

	static void KeyCallbackEntry(GLFWwindow* window, int key, int scancode, int action, int mods);
	// void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

	static void CharCallbackEntry(GLFWwindow* window, unsigned int codepoint);
	// void CharCallback(GLFWwindow* window, unsigned int codepoint);

	static void FrameBufferSizeCallbackEntry(GLFWwindow* window, int width, int height);
	void FrameBufferSizeCallback(GLFWwindow* window, int width, int height);
private:
	GLFWwindow* m_Window;

	bool isMouseOutOfFocus{true};

	std::unique_ptr<class Camera> m_Camera;
	glm::mat4 projection;
	float m_Near{ 0.1f };
	float m_Far{ 1000.0f };

	glm::vec2 m_ScreenSize;
	glm::vec2 m_MousePos{};

	float lastTime{ 0.0f };
	float deltaTime{ 0.0f };
};

