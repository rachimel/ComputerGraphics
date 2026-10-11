#define CG_APPLICATION_CUSTOM_POLL_INPUT
#define CG_APPLICATION_CUSTOM_CALLBACK_KEY
#define CG_APPLICATION_CUSTOM_CALLBACK_CHAR

#define CG_MESH_ENABLE_COLOR

#include <optional>
#include <CG.h>
#include <gl/glew.h>
#include <Shader.h>


#include <random>
#include <numeric>

#include <glm/gtc/matrix_transform.hpp>

namespace
{
	std::optional<Model> object{};
	std::optional<Model> axes{};

	std::optional<Shader> shader{};
	float speed{ 5.0f };
	std::random_device rd;
	std::default_random_engine dre{ rd() };

	GLenum drawingMode = GL_TRIANGLES;
	float yawSign{ 1.0f };
	float pitchSign{ 1.0f };
	float angularSpeed{ 60.0f };
	float yaw{ 0.0f };
	float pitch{ 0.0f };

	glm::vec3 translationPos{};

	bool rotateYaw{};
	bool rotatePitch{};
}

int main()
{
	Application app{ 800, 800 };
	int code = app.Init("I hate computer graphics");
	if (code)
		return code;
	app.CaptureMouse();
	app.EnableOpenGLFeatures(GL_DEPTH_TEST);
	app.Run();
}

void Draw(Shader& shader)
{
	auto& meshes = object->Meshes();
	for (auto& mesh : meshes)
	{
		mesh.Bind();
		glDrawElements(drawingMode, static_cast<GLsizei>(mesh.m_Indices.size()), GL_UNSIGNED_INT, (const void*)0);
	}
}

void Application::OnInit()
{
	axes.emplace("data\\1_opengl_basics\\axes.obj");

	shader.emplace("shaders\\1_opengl_basics\\perspectiveProjection.vs", "shaders\\1_opengl_basics\\vertexColor.fs");

	CreateCamera(glm::vec3(1.0f, 1.0f, 1.0f));
	m_Camera->FocusAt(glm::vec3(0.0f, 0.0f, 0.0f));
}

void Application::Update()
{
	if (rotatePitch && rotateYaw)
	{
		pitch += pitchSign * angularSpeed * deltaTime;
		yaw += yawSign * angularSpeed * deltaTime;
	}
	else if (rotatePitch)
	{
		pitch += pitchSign * angularSpeed * deltaTime;
		yaw = 0.0f;
	}
	else if (rotateYaw)
	{
		pitch = 0.0f;
		yaw += yawSign *  angularSpeed * deltaTime;
	}
}

void Application::Render()
{
	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glm::mat4 model{ glm::mat4(1.0f) };

	shader->Bind();
	shader->SetUniform("projection", projection);
	shader->SetUniform("view", m_Camera->ViewMatrix());
	shader->SetUniform("model", model);

	auto& axesMeshes = axes->Meshes();
	for (auto& mesh : axes->Meshes())
	{
		mesh.Bind();
		glLineWidth(2.0f);
		glDrawElements(GL_LINES, mesh.m_Indices.size(), GL_UNSIGNED_INT, (const void*)0);
	}

	if(object)
	{
		model = glm::translate(model, translationPos);
		model = glm::rotate(model, glm::radians(pitch), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(yaw), glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::scale(model, glm::vec3(0.5f));
		shader->SetUniform("model", model);
		Draw(*shader);
	}
}

void Application::OnKeyEvent(GLFWwindow* window, int key, int action, int mods)
{
	if (action == GLFW_PRESS)
	{
		switch (key)
		{
		case GLFW_KEY_C:
			object.emplace("data\\1_opengl_basics\\cube.obj");
			break;
		case GLFW_KEY_P:
			object.emplace("data\\1_opengl_basics\\pyramid.obj");
			break;
		case GLFW_KEY_H:
			if (IsOpenGLFeatureEnabled(GL_DEPTH_TEST))
				DisableOpenGLFeatures(GL_DEPTH_TEST);
			else
				EnableOpenGLFeatures(GL_DEPTH_TEST);
			break;
		case GLFW_KEY_S:
			translationPos = glm::vec3(0.0f);
			rotateYaw = false;
			rotatePitch = false;
			break;
		case GLFW_KEY_ESCAPE:
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			isMouseOutOfFocus = true;
			break;
		case GLFW_KEY_Q:
			glfwSetWindowShouldClose(window, true);
			break;
		}
	}
}

void Application::OnCharEvent(GLFWwindow* window, unsigned int codepoint)
{
	switch (codepoint)
	{
	case 'w':
		drawingMode = GL_LINE_LOOP;
		break;
	case 'W':
		drawingMode = GL_TRIANGLES;
		break;
	case 'x':
		if (pitchSign == 1.0f && rotatePitch)
		{
			rotatePitch = false;
			break;
		}
		pitchSign = 1.0f;
		rotatePitch = true;
		break;
	case 'X':
		if (pitchSign == -1.0f && rotatePitch)
		{
			rotatePitch = false;
			break;
		}
		pitchSign = -1.0f;
		rotatePitch = true;
		break;
	case 'y':
		if (yawSign == 1.0f && rotateYaw)
		{
			rotateYaw = false;
			break;
		}
		yawSign = 1.0f;
		rotateYaw = !rotateYaw;
		break;
	case 'Y':
		if (yawSign == -1.0f && rotateYaw)
		{
			rotateYaw = false;
			break;
		}
		yawSign = -1.0f;
		rotateYaw = !rotateYaw;
		break;
	}
}
void Application::OnInputPoll()
{
	if (glfwGetKey(m_Window, GLFW_KEY_UP) == GLFW_PRESS)
	{
		if(object)
			translationPos += glm::vec3(0.0f, 1.0f, 0.0f) * deltaTime * speed;
	}
	else if (glfwGetKey(m_Window, GLFW_KEY_DOWN) == GLFW_PRESS)
	{
		if (object)
			translationPos += glm::vec3(0.0f, -1.0f, 0.0f) * deltaTime * speed;
	}
	if (glfwGetKey(m_Window, GLFW_KEY_LEFT) == GLFW_PRESS)
	{
		if (object)
			translationPos += glm::vec3(1.0f, 0.0f, 0.0f) * deltaTime * speed;
	}
	else if (glfwGetKey(m_Window, GLFW_KEY_RIGHT) == GLFW_PRESS)
	{
		if (object)
			translationPos += glm::vec3(-1.0f, 0.0f, 0.0f) * deltaTime * speed;
	}
}