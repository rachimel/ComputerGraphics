#include <iostream>
#include <print>

#include <gl/glew.h>
#include <GLFW/glfw3.h>
#include <Application.h>

// The Application class was designed with the following requirements:
//
// 1. Unused callbacks should be completely disabled at compile time.
// 2. Custom callback implementations should be defined in the application's entry file.
// 3. Application should still handle the common behavior of each callback.
// 4. No inheritance should be required.
// 5. Enabling or disabling callbacks should require as little configuration as possible.
//
// A macro-based compile-time configuration satisfies these requirements well.
// However, macros defined in an entry file are local to that translation unit;
// they are not propagated to a separately compiled Application.cpp.
//
// A shared configuration header could solve this, but it would require a
// separate configuration for each executable, which defeats the goal of
// keeping each practice/application self-contained.
//
// Therefore, the configuration-dependent implementation of Application is
// placed in Application.inl and included by the entry translation unit.
// This allows the entry file to define the required callback macros before
// including the implementation.

Application::Application(int width, int height)
	: m_Window{}, projection{glm::mat4(1.0f)},
	m_ScreenSize {static_cast<float>(width), static_cast<float>(height)}
{

}

Application::~Application()
{
	if (m_Window)
		glfwDestroyWindow(m_Window);
	glfwTerminate();
}

void Application::MouseButtonCallbackEntry(GLFWwindow* window, int button, int action, int mods)
{
#if defined(CG_APPLICATION_CUSTOM_CALLBACK_MOUSE_BUTTON)
	Application* app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
	if (app)
	{
		app->OnMouseButtonEvent(window, button, action, mods);
	}
#endif
}

void Application::CursorPosCallbackEntry(GLFWwindow* window, double xPos, double yPos) 
{
	Application* app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
	if (app)
	{
#if defined(CG_APPLICATION_CUSTOM_CALLBACK_CURSOR_POS)
		app->OnCursorMoveEvent(window, static_cast<float>(xPos), static_cast<float>(yPos));
#endif
		app->CursorPosCallback(window, static_cast<float>(xPos), static_cast<float>(yPos));
	}
}

void Application::CursorPosCallback(GLFWwindow* window, float xPos, float yPos)
{
	m_MousePos = glm::vec2(xPos, yPos);
}

void Application::KeyCallbackEntry(GLFWwindow* window, int key, int scancode, int action, int mods)
{
#if defined(CG_APPLICATION_CUSTOM_CALLBACK_KEY)	
	Application* app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
	if (app)
		app->OnKeyEvent(window, key, action, mods);
#endif
}


void Application::CharCallbackEntry(GLFWwindow* window, unsigned int codepoint) 
{
#if defined(CG_APPLICATION_CUSTOM_CALLBACK_CHAR)	
	Application* app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
	if (app)
		app->OnKeyEvent(window, key, action, mods);
#endif
}

void Application::FrameBufferSizeCallbackEntry(GLFWwindow* window, int width, int height)
{
	Application* app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
	if (app)
	{
		app->FrameBufferSizeCallback(window, width, height);
	}
}


void Application::FrameBufferSizeCallback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
	m_ScreenSize = glm::vec2(static_cast<float>(width), static_cast<float>(height));
	projection = m_Camera.ProjectionMatrix(m_ScreenSize.x / m_ScreenSize.y, m_Near, m_Far);
}

int Application::Init(std::string_view title)
{
	if (!glfwInit())
	{
		std::println(std::cerr, "[GLFW] : Failed to initialize GLFW!");
		return -1;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	m_Window = glfwCreateWindow(static_cast<int>(m_ScreenSize.x), static_cast<int>(m_ScreenSize.y), title.data(),
		nullptr, nullptr);

	if (!m_Window)
	{
		std::println(std::cerr, "[GLFW] : Failed to create window!");
		return -2;
	}

	glfwMakeContextCurrent(m_Window);
	
	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK)
	{
		std::println(std::cerr, "[GLEW] : Failed to initialize GLEW!");
		return -3;
	}

	glfwSetWindowUserPointer(m_Window, this);
	// Register Callbacks
#if defined (CG_APPLICATION_CUSTOM_CALLBACK_MOUSE_BUTTON)
	glfwSetMouseButtonCallback(m_Window, Application::MouseButtonCallbackEntry);
#else
	glfwSetMouseButtonCallback(m_Window, nullptr);
#endif

	glfwSetCursorPosCallback(m_Window, Application::CursorPosCallbackEntry);

#if defined (CG_APPLICATION_CUSTOM_CALLBACK_KEY)
	glfwSetKeyCallback(m_Window, Application::KeyCallbackEntry);
#else
	glfwSetKeyCallback(m_Window, nullptr);
#endif

#if defined (CG_APPLICATION_CUSTOM_CALLBACK_CHAR)
	glfwSetCharCallback(m_Window, Application::CharCallbackEntry);
#else
	glfwSetCharCallback(m_Window, nullptr);
#endif

	glfwSetFramebufferSizeCallback(m_Window, Application::FrameBufferSizeCallbackEntry);

	// OpenGL Function Settings
	glViewport(0, 0, static_cast<int>(m_ScreenSize.x), static_cast<int>(m_ScreenSize.y));

	OnInit();
	return 0;
}

void Application::Run()
{
	while (!glfwWindowShouldClose(m_Window))
	{
		float currentTime = static_cast<float>(glfwGetTime());
		deltaTime = currentTime - lastTime;
		lastTime = currentTime;

		PollInputs();
		Update();
		Render();

		glfwSwapBuffers(m_Window);
		glfwPollEvents();
	}
}

void Application::CaptureMouse()
{
	glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void Application::EnableOpenGLFeatures(GLenum features)
{
	glEnable(features);
}

void Application::DisableOpenGLFeatures(GLenum features)
{
	glDisable(features);
}

void Application::SetNearPlane(float zNear)
{
	m_Near = zNear;
	projection = m_Camera.ProjectionMatrix(m_ScreenSize.x / m_ScreenSize.y, m_Near, m_Far);
}

void Application::SetFarPlane(float zFar)
{
	m_Far = zFar;
	projection = m_Camera.ProjectionMatrix(m_ScreenSize.x / m_ScreenSize.y, m_Near, m_Far);
}
