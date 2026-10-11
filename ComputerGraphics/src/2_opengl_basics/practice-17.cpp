#define CG_APPLICATION_CUSTOM_CALLBACK_KEY
#define CG_MESH_ENABLE_COLOR
#include <CG.h>

#include <optional>

struct Object
{
	glm::vec3 pos{};
	glm::vec4 color{};
	
	float rotateXT{ 0.0f };
	float rotateXSign{ 0.0f };

	float rotateZT{ 0.0f };
	float rotateZSign{ 0.0f };

	void Update(float dt)
	{
		if (rotateXSign != 0.0f)
		{

		}
	}

	glm::mat4 GetWorldTransform()
	{
		 
	}
};

namespace
{
	std::optional<Model> axes{};
	std::optional<Shader> shader{};
	std::optional<Model> objectAModel{};
	std::optional<Model> objectBModel{};

	Object objectA{};
	Object objectB{};
}

int main()
{
	Application app{ 800,800 };
	int code = app.Init("I hate computer graphics 4");
	if (code) return code;
	app.CaptureMouse();
	app.EnableOpenGLFeatures(GL_DEPTH_TEST);
	app.Run();
}

void Application::OnInit()
{
	shader.emplace("shaders\\1_opengl_basics\\perspectiveProjection.vs", "shaders\\1_opengl_basics\\vertexColor.fs");
	axes.emplace("data\\1_opengl_basics\\axes.obj");
	objectAModel.emplace("data\\1_opengl_basics\\sphere.obj");
	objectBModel.emplace("data\\1_opengl_basics\\cube.obj");
	CreateCamera(glm::vec3(-3.0f, 2.0f, -3.0f));
	m_Camera->FocusAt(glm::vec3(0.0f));
}

void Application::Update()
{

}

void Application::Render()
{
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glm::mat4 model{ glm::mat4(1.0f) };;
	shader->Bind();
	shader->SetUniform("projection", projection);
	shader->SetUniform("view", m_Camera->ViewMatrix());
	shader->SetUniform("model", model);

	auto& axesMeshes = axes->Meshes();
	for (auto& axesMesh : axesMeshes)
	{
		axesMesh.Bind();
		glDrawElements(GL_LINES, axesMesh.m_Indices.size(), GL_UNSIGNED_INT, (const void*)0);
	}

	model = glm::mat4(1.0f);
	shader->SetUniform("model", model);
	auto& objectAMeshes = objectAModel->Meshes();
	for (auto& objectAMesh : objectAMeshes)
	{
		objectAMesh.Bind();
		glDrawElements(GL_TRIANGLES, objectAMesh.m_Indices.size(), GL_UNSIGNED_INT, (const void*)0);
	}

	model = glm::mat4(1.0f);
	shader->SetUniform("model", model);
	auto& objectBMeshes = objectBModel->Meshes();
	for (auto& objectBMesh : objectBMeshes)
	{
		objectBMesh.Bind();
		glDrawElements(GL_TRIANGLES, objectBMesh.m_Indices.size(), GL_UNSIGNED_INT, (const void*)0);
	}
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