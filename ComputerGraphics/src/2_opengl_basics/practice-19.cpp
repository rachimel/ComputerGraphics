#define CG_APPLICATION_CUSTOM_CALLBACK_KEY
#include <CG.h>

int main()
{
	Application app{ 1280,1080 };
	if (int code = app.Init("i hate computer graphics 6")) return code;
	app.CaptureMouse();
	glEnable(GL_DEPTH_TEST);
	app.Run();
}

void Application::OnInit()
{
	
}

void Application::Update()
{

}

void Application::Render()
{
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Application::OnKeyEvent(GLFWwindow* window, int key, int action, int mods)
{
	if (action == GLFW_PRESS)
	{
		switch (key)
		{
		case GLFW_KEY_Q:
			glfwSetWindowShouldClose(m_Window, true);
			break;
		}
	}
}