#define CG_APPLICATION_CUSTOM_CALLBACK_KEY
#define CG_MESH_ENABLE_COLOR

#include <optional>
#include <CG.h>
#include <gl/glew.h>
#include <Shader.h>


#include <random>
#include <numeric>

#include <glm/gtc/matrix_transform.hpp>

enum DrawingModel {
	MODEL_CUBE,
	MODEL_PYRAMID,
};
namespace
{
	std::optional<Model> cube{};
	std::optional<Model> pyramid{};
	std::optional<Model> axes{};

	std::optional<Shader> shader{};

	int drawingPlane1{-1};
	int drawingPlane2{ -1 };

	DrawingModel drawingModel{ MODEL_CUBE };

	std::random_device rd;
	std::default_random_engine dre{ rd() };
}

int main()
{
	Application app{800, 800};
	int code = app.Init("I hate computer graphics");
	if (code)
		return code;
	app.CaptureMouse();
	app.EnableOpenGLFeatures(GL_DEPTH_TEST);
	app.Run();
}
	
void DrawCube(Shader& shader)
{
	auto& cubeMeshes = cube->Meshes();
	for (auto& cubeMesh : cubeMeshes)
	{
		cubeMesh.Bind();
		if (drawingPlane2 >= 0 && drawingPlane1 >= 0)
		{
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (const void*)(drawingPlane1 * 6 * sizeof(unsigned int)));
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (const void*)(drawingPlane2 * 6 * sizeof(unsigned int)));
		}
		else if (drawingPlane1 >= 0)
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (const void*)(drawingPlane1 * 6 * sizeof(unsigned int)));
		else
			glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(cubeMesh.m_Indices.size()), GL_UNSIGNED_INT, (const void*)0);
	}
}

void DrawPyramid(Shader& shader)
{
	auto& pyramidMeshes = pyramid->Meshes();
	for (auto& pyramidMesh : pyramidMeshes)
	{
		pyramidMesh.Bind();
		if (drawingPlane2 >= 0 && drawingPlane1 >= 1)
		{
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (const void*)(0));
			glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, (const void*)(drawingPlane1 * 3 * sizeof(unsigned int)));
		}
		else if (drawingPlane1 >= 1)
			glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, (const void*)(drawingPlane1 * 3 * sizeof(unsigned int)));
		else
			glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(pyramidMesh.m_Indices.size()), GL_UNSIGNED_INT, (const void*)0);
	}
}

void Application::OnInit()
{
	cube.emplace("data\\1_opengl_basic\\cube.obj");
	pyramid.emplace("data\\1_opengl_basic\\pyramid.obj");
	axes.emplace("data\\1_opengl_basic\\axes.obj");

	shader.emplace("shaders\\1_opengl_basics\\perspectiveProjection.vs", "shaders\\1_opengl_basics\\vertexColor.fs");

	CreateCamera(glm::vec3(0.0f, 3.0f, 3.0f));
	m_Camera->FocusAt(glm::vec3(0.0f, 0.0f, 0.0f));
}

void Application::Update() {}
void Application::Render()
{
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
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
	model = glm::scale(model, glm::vec3(0.5f));
	shader->SetUniform("model", model);
	switch (drawingModel)
	{
	case MODEL_CUBE:
		DrawCube(*shader);
		break;
	case MODEL_PYRAMID:
 		DrawPyramid(*shader);
		break;
	}
}

void ChangeCubeSingleDrawPlane(int planeIndex)
{
	if (drawingModel == MODEL_PYRAMID) drawingModel = MODEL_CUBE;
	drawingPlane1 = (drawingPlane1 == planeIndex) ? -1 : planeIndex;
	if (drawingPlane2 >= 0)
		drawingPlane2 = -1;
}

void ChangeCubeDoubleDrawPlane()
{
	if (drawingModel == MODEL_PYRAMID) drawingModel = MODEL_CUBE;
	std::vector<int> indices(6);
	std::iota(indices.begin(), indices.end(), 0);
	std::shuffle(indices.begin(), indices.end(), dre);

	drawingPlane1 = indices.front();
	drawingPlane2 = indices.back();
}

void ChangePyramidDoubleDrawPlane()
{
	if (drawingModel == MODEL_CUBE) drawingModel = MODEL_PYRAMID;
	std::vector<int> indices(4);
	std::iota(indices.begin(), indices.end(), 2);
	std::shuffle(indices.begin(), indices.end(), dre);

	drawingPlane1 = indices.back();
	drawingPlane2 = 0;
}

void ChangePyramidSingleDrawPlane(int planeIndex)
{
	if (drawingModel == MODEL_CUBE) drawingModel = MODEL_PYRAMID;
	drawingPlane1 = (drawingPlane1 == planeIndex) ? -1 : planeIndex;
	if (drawingPlane2 >= 0)
		drawingPlane2 = -1;
}
void Application::OnKeyEvent(GLFWwindow* window, int key, int action, int mods)
{
	if (action == GLFW_PRESS)
	{
		switch (key)
		{
		case GLFW_KEY_1:
			ChangeCubeSingleDrawPlane(0);
			break;
		case GLFW_KEY_2:
			ChangeCubeSingleDrawPlane(1);
			break;
		case GLFW_KEY_3:
			ChangeCubeSingleDrawPlane(2);
			break;
		case GLFW_KEY_4:
			ChangeCubeSingleDrawPlane(3);
			break;
		case GLFW_KEY_5:
			ChangeCubeSingleDrawPlane(4);
			break;
		case GLFW_KEY_6:
			ChangeCubeSingleDrawPlane(5);
			break;
		case GLFW_KEY_7:
			ChangePyramidSingleDrawPlane(2);
			break;
		case GLFW_KEY_8:
			ChangePyramidSingleDrawPlane(3);
			break;
		case GLFW_KEY_9:
			ChangePyramidSingleDrawPlane(4);
			break;
		case GLFW_KEY_0:
			ChangePyramidSingleDrawPlane(5);
			break;
		case GLFW_KEY_C:
			ChangeCubeDoubleDrawPlane();
			break;
		case GLFW_KEY_T:
			ChangePyramidDoubleDrawPlane();
			break;
		case GLFW_KEY_ESCAPE:
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			break;
		case GLFW_KEY_Q:
			glfwSetWindowShouldClose(window, true);
			break;
		}
	}
}