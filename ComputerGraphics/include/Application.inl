#include <iostream>
#include <print>

#include <gl/glew.h>
#include <GLFW/glfw3.h>
#include <Application.h>
#include <Camera.h>

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
	: 
	m_Window{},
	m_Cameras{},
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
	Application* app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
	if (app)
	{
#if defined(CG_APPLICATION_CUSTOM_CALLBACK_MOUSE_BUTTON)
		app->OnMouseButtonEvent(button, action, mods);
#endif
		app->MouseButtonCallback(button, action, mods);
	}
}

void Application::MouseButtonCallback(int button, int action, int mods)
{
	CaptureMouse();
}

void Application::CursorPosCallbackEntry(GLFWwindow* window, double xPos, double yPos) 
{
	Application* app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
	if (app)
	{
#if defined(CG_APPLICATION_CUSTOM_CALLBACK_CURSOR_POS)
		app->OnCursorMoveEvent(static_cast<float>(xPos), static_cast<float>(yPos));
#endif
		app->CursorPosCallback(static_cast<float>(xPos), static_cast<float>(yPos));
	}
}

void Application::CursorPosCallback(float xPos, float yPos)
{
	CameraMouseCallback(xPos, yPos);
	// update mouse position.
	m_MousePos = glm::vec2(xPos, yPos);
}

void Application::CameraMouseCallback(float xPos, float yPos)
{
	if (mouseActiveCamera < 0 || static_cast<size_t>(mouseActiveCamera) >= m_Cameras.size())
		return;

	if (glfwGetInputMode(m_Window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED)
	{
		float xOffset = (xPos - m_MousePos.x);
		float yOffset = (m_MousePos.y - yPos);

		m_Cameras[mouseActiveCamera].Rotate(xOffset, yOffset);
	}
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
		app->OnCharEvent(window, codepoint);
#endif
}

void Application::FrameBufferSizeCallbackEntry(GLFWwindow* window, int width, int height)
{
	Application* app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
	if (app)
	{
		app->FrameBufferSizeCallback(width, height);
	}
}


void Application::FrameBufferSizeCallback(int width, int height)
{
	glViewport(0, 0, width, height);
	m_ScreenSize = glm::vec2(static_cast<float>(width), static_cast<float>(height));
	if(!m_CameraViewports.empty())
		UpdateCameraViewportLayout();
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
	glfwSetMouseButtonCallback(m_Window, Application::MouseButtonCallbackEntry);
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
		glfwPollEvents();

		PollInputs();
		PollCameraInputs(mouseActiveCamera);
		Update();

		Clear();
		// Render with multiple cameras.
		int cameraListSize = static_cast<int>(m_Cameras.size());
		for (int i = 0; i < cameraListSize; ++i)
		{
			int width = m_CameraViewports[i].z - m_CameraViewports[i].x;
			int height = m_CameraViewports[i].w - m_CameraViewports[i].y;
			glViewport(m_CameraViewports[i].x, m_ScreenSize.y - m_CameraViewports[i].w, width, height);
			Render(m_Cameras[i].ViewMatrix(), m_Cameras[i].ProjectionMatrix(width / static_cast<float>(height)), i);
		}

		glfwSwapBuffers(m_Window);
	}
}

void Application::AppendCamera(
	const glm::vec3& pos, 
	const glm::vec3& worldUp /*= glm::vec3(0.0f, 1.0f, 0.0f)*/, 
	float fovy /*= 45.0f*/, 
	float yaw /*= 0.0f*/, 
	float pitch /*= 0.0f*/, 
	float near /*= 0.1f*/, 
	float far /*= 1000.0f*/
)
{
	Camera newCamera{pos, worldUp, fovy, near, far};
	newCamera.OrientAt(yaw, pitch);
	m_Cameras.push_back(newCamera);

	m_CameraViewports.push_back(glm::ivec4{});
	UpdateCameraViewportLayout();
}

void Application::PollInputs()
{
#if defined(CG_APPLICATION_CUSTOM_POLL_INPUT)
	OnInputPoll();
#endif
}

void Application::PollCameraInputs(int cameraIdx)
{
	if (mouseActiveCamera == -1) return;

	if (glfwGetKey(m_Window, GLFW_KEY_KP_8) == GLFW_PRESS)
	{
		m_Cameras[cameraIdx].Move(CameraDir::Up, deltaTime);
	}
	else if (glfwGetKey(m_Window, GLFW_KEY_KP_2) == GLFW_PRESS)
	{
		m_Cameras[cameraIdx].Move(CameraDir::Down, deltaTime);
	}
	if (glfwGetKey(m_Window, GLFW_KEY_KP_4) == GLFW_PRESS)
	{
		m_Cameras[cameraIdx].Move(CameraDir::Left, deltaTime);
	}
	else if (glfwGetKey(m_Window, GLFW_KEY_KP_6) == GLFW_PRESS)
	{
		m_Cameras[cameraIdx].Move(CameraDir::Right, deltaTime);
	}

	if (glfwGetKey(m_Window, GLFW_KEY_KP_9) == GLFW_PRESS)
	{
		m_Cameras[cameraIdx].Move(CameraDir::Ascend, deltaTime);
	}
	else if (glfwGetKey(m_Window, GLFW_KEY_KP_3) == GLFW_PRESS)
	{
		m_Cameras[cameraIdx].Move(CameraDir::Descend, deltaTime);
	}

}


void Application::CaptureMouse()
{
	int cameraViewportSize{ static_cast<int>(m_CameraViewports.size()) };
	for (int i = 0; i < cameraViewportSize; ++i)
	{
		if (m_MousePos.x >= m_CameraViewports[i].x && m_MousePos.x < m_CameraViewports[i].z &&
			m_MousePos.y >= m_CameraViewports[i].y && m_MousePos.y < m_CameraViewports[i].w)
		{
			mouseActiveCamera = i;
			glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

			double mouseX, mouseY;
			glfwGetCursorPos(m_Window, &mouseX, &mouseY);
			return;
		}
	}
}

void Application::UpdateCameraViewportLayout()
{
	glm::ivec4 framebufferSize{ 0,0,static_cast<int>(m_ScreenSize.x),static_cast<int>(m_ScreenSize.y)};

	const int cameraViewportCount = static_cast<int>(m_CameraViewports.size());
	if (cameraViewportCount == 1)
		m_CameraViewports[0] = framebufferSize;
	else if (cameraViewportCount == 2)
	{
		int framebufferHalfW = framebufferSize.z / 2;
		m_CameraViewports[0] = glm::ivec4(0, 0, framebufferHalfW, framebufferSize.w);
		m_CameraViewports[1] = glm::ivec4(framebufferHalfW, 0, framebufferSize.z, framebufferSize.w);
	}
	else if (cameraViewportCount == 3)
	{
		int framebufferHalfW = framebufferSize.z / 2;
		int framebufferHalfH = framebufferSize.w / 2;
		m_CameraViewports[0] = glm::ivec4(0, 0, framebufferHalfW, framebufferSize.w);
		m_CameraViewports[1] = glm::ivec4(framebufferHalfW, 0, framebufferSize.z, framebufferHalfH);
		m_CameraViewports[2] = glm::ivec4(framebufferHalfW, framebufferHalfH, framebufferSize.z, framebufferSize.w);
	}
	else
	{
		// split viewports by the 
		float bestCost = std::numeric_limits<float>::infinity();
		int bestCols{ 1 };
		int bestRows{cameraViewportCount};
		// penalty to empty grid.
		float lambda{0.4f};
		for (int cols = 1; cols <= cameraViewportCount; ++cols)
		{
			int rows = (cameraViewportCount + cols - 1) / cols;

			float tileWidth = m_ScreenSize.x / cols;
			float tileHeight = m_ScreenSize.y / rows;

			float asepct = tileWidth / tileHeight;
			float emptyRatio = float(cols * rows - cameraViewportCount) / (cols * rows);
			
			float targetAspect = m_ScreenSize.x / m_ScreenSize.y;
			float emptyCellRatio = (cols * rows - cameraViewportCount) / static_cast<float>(cols * rows);
			float cost = glm::abs(glm::log2(asepct / targetAspect)) + lambda * emptyRatio;

			if (cost < bestCost)
			{
				bestCost = cost;
				bestCols = cols;
				bestRows = rows;
			}
		}

		// place best grids
		for (int y = 0; y < bestRows; ++y)
		{
			for (int x = 0; x < bestCols; ++x)
			{
				int currentGridIdx = y * bestCols + x;
				if (currentGridIdx >= cameraViewportCount) break;

				int gridX0 = x * framebufferSize.z / bestCols;
				int gridY0 = y * framebufferSize.w / bestRows;
				int gridX1 = (x + 1) * framebufferSize.z / bestCols;
				int gridY1 = (y + 1) * framebufferSize.w / bestRows;

				m_CameraViewports[currentGridIdx] = glm::ivec4{gridX0, gridY0, gridX1, gridY1};
			}
		}
	}
}