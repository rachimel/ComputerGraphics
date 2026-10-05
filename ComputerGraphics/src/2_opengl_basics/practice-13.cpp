#define CG_APPLICATION_POLL_INPUT
#define CG_APPLICATION_CUSTOM_CALLBACK_CURSOR_POS
#define CG_APPLICATION_CUSTOM_CALLBACK_KEY
#define CG_MESH_ENABLE_COLOR

#include <optional>

#include <CG.h>

#include <gl/glew.h>
#include <Shader.h>

namespace
{
	std::optional<Model> cube{};
	std::optional<Shader> shader{};
	bool firstMove = true;
	float sensitivity = 0.1f;
	float speed = 5.0f;
}

void PollInputs(GLFWwindow* window, float dt);

int main()
{
	Application app{800, 800};
	int code = app.Init("I hate computer graphics");
	if (code)
		return code;
	app.CaptureMouse();
	app.Run();
}
	
void DrawCube(Shader& shader)
{
	auto& cubeMeshes = cube->Meshes();
	for (auto& cubeMesh : cubeMeshes)
	{
		cubeMesh.Bind();
		glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(cubeMesh.m_Indices.size()), GL_UNSIGNED_INT, (const void*)0);
	}
}

void Application::OnInit()
{
	cube.emplace("data\\1_opengl_basic\\cube.obj");
	shader.emplace("shaders\\1_opengl_basics\\perspectiveProjection.vs", "shaders\\1_opengl_basics\\vertexColor.fs");

	m_Camera.PlaceAt(glm::vec3(0.0f, 3.0f, 3.0f));
	m_Camera.Zoom(45.0f);
	m_Camera.OrientAt(-90.0f, 0.0f);
	m_Camera.RefWorldUp(glm::vec3(0.0f, 1.0f, 0.0f));

	projection = m_Camera.ProjectionMatrix(m_ScreenSize.x / m_ScreenSize.y, m_Near, m_Far);

}
void Application::Update()
{
}

void Application::Render()
{
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	shader->Bind();
	shader->SetUniform("projection", projection);
	shader->SetUniform("view", m_Camera.ViewMatrix());
	shader->SetUniform("model", glm::mat4(1.0f));

	DrawCube(*shader);
}

void Application::OnCursorMoveEvent(GLFWwindow* window, float xPos, float yPos)
{
	if (firstMove)
	{
		m_MousePos = glm::vec2(xPos, yPos);
		firstMove = false;
	}
	if (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED)
	{
		float xOffset = (xPos - m_MousePos.x) * sensitivity;
		float yOffset = (m_MousePos.y - yPos) * sensitivity;

		m_Camera.Rotate(xOffset, yOffset);
	}
}

void Application::OnKeyEvent(GLFWwindow* window, int key, int action, int mods)
{
	if (action == GLFW_PRESS)
	{
		switch (key)
		{
		case GLFW_KEY_ESCAPE:
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			break;
		case GLFW_KEY_Q:
			glfwSetWindowShouldClose(window, true);
			break;
		}
	}
}

void Application::PollInputs()
{
	if (glfwGetKey(m_Window, GLFW_KEY_UP) == GLFW_PRESS)
	{
		m_Camera.Move(CameraDir::Up, speed, deltaTime);
	}
	else if (glfwGetKey(m_Window, GLFW_KEY_LEFT) == GLFW_PRESS)
	{
		m_Camera.Move(CameraDir::Left, speed, deltaTime);
	}
	else if (glfwGetKey(m_Window, GLFW_KEY_RIGHT) == GLFW_PRESS)
	{
		m_Camera.Move(CameraDir::Right, speed, deltaTime);
	}
	else if (glfwGetKey(m_Window, GLFW_KEY_DOWN) == GLFW_PRESS)
	{
		m_Camera.Move(CameraDir::Down, speed, deltaTime);
	}
}