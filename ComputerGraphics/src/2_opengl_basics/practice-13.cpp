#define CG_APPLICATION_CUSTOM_CALLBACK_KEY

#include <Application.h>
#include <Application.inl>

#include <gl/glew.h>

int main()
{
	Application app{800, 800};
	int code = app.Init("I hate computer graphics");
	if (code)
		return code;
	app.Run();
}

void Application::Update(float dt)
{

}

void Application::Render()
{
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Application::OnKeyEvent(GLFWwindow* window, int key, int action, int mods)
{
	if (action == GLFW_PRESS)
	{
		switch (key)
		{
		case GLFW_KEY_Q:
			glfwSetWindowShouldClose(window, true);
			break;
		}
	}
}