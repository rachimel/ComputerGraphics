#define CG_APPLICATION_CUSTOM_CALLBACK_KEY
#define CG_APPLICATION_CUSTOM_CALLBACK_CHAR

#include <CG.h>

namespace
{
	std::optional<Shader> lightingShader{};
	std::optional<Shader> uniformColorShader{};

	std::vector<Vec3> colors{
		Vec3{ 1.0f, 1.0f, 1.0f },
		Vec3{ 0.0f, 1.0f, 0.0f },
		Vec3{ 0.0f, 0.0f, 1.0f }
	};

	Object mercury{};
	Object venus{};
	Object earth{};

	Object mercuryTrail{};
	Object venusTrail{};
	Object earthTrail{};

	Object plane{};
	Object pyramid{};
	Object lightCube{};

	ObjectCollection snow{};

	Vec3 tankPos{ 0.0f, 3.0f, 0.0f };
	Vec3 lightPos{ 10.0f, 10.0f, 0.0f };
	Timeline* rotatelightCube{};

	SpawnedNode tankNode{};
	SpawnedNode lightCubeNode{};

	int lightIdx{ 0 };
	bool enableLight{ true };
	bool rotateLight{ false };
}

int main()
{
	Application app{ 1280, 960 };
	if (int code = app.Init("i hate computer graphics")) return code;
	glEnable(GL_DEPTH_TEST);
	app.Run();
}

void Application::OnInit()
{
	lightingShader.emplace("shaders/1_opengl_basics/projectionViewNormal.vs",
		"shaders/1_opengl_basics/pointPhongLight.fs");
	uniformColorShader.emplace("shaders/1_opengl_basics/projectionView.vs",
		"shaders/1_opengl_basics/uniformColor.fs");

	lightingShader->Bind();
	AppendCamera(glm::vec3(0.0f, 10.0f, 20.0f));
	m_Cameras[0].FocusAt(Vec3{ 0.0f });

	/*tankColors = {
		glm::vec3(0.5f),
		glm::vec3(0.1f),
		glm::vec3(0.1f),
		glm::vec3(0.4f, 0.6f, 0.4f),
		glm::vec3(0.4f, 0.6f, 0.4f),
		glm::vec3(0.7f, 0.8f, 0.6f),
		glm::vec3(0.7f, 0.8f, 0.6f),
		glm::vec3(0.8f, 0.8f, 0.1f),
		glm::vec3(0.8f, 0.8f, 0.1f),

	};*/
	plane.SpawnOpaque({ .file = "data/1_opengl_basics/plane.obj" ,
		.color = Vec3{ 0.3f }, .pos = Vec3{ 0.0f, 0.0f, 0.0f }, .scale = Vec3{ 100.0f, 1.0f, 100.0f }
		});

	lightCubeNode = lightCube.SpawnOpaque({ .file = "data/1_opengl_basics/cubePrimitive.obj",
		.color = colors[0], .pos = lightPos, .scale = Vec3{ 0.5f } });
	auto& global = AnimationHandler::Global();
	auto rotateID = global.SetRotateAnimation(360.0f, Vec3{ 0.0f, 1.0f, 0.0f });
	auto& t = lightCube.CreateTimeline();
	t.AddEntry().Append(rotateID, lightCubeNode.nodeID, lightCubeNode.trs.t, 5.0f,
		AnimationPlacement::Before);
	t.SetLoop(true);
	t.Stop();
	rotatelightCube = &t;

}

void Application::Clear()
{
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Application::Update()
{
	lightCube.Update(deltaTime);
}

void Application::Render(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix, int cameraIdx)
{
	lightingShader->Bind();
	Vec3 rotatedLightPos = Vec3{ *rotatelightCube->FindTransform(lightCubeNode.nodeID,
		lightCubeNode.trs.t) * Vec4 {
lightPos, 1.0f
} };

	lightingShader->SetUniform("lightPos", Vec3{ viewMatrix * Vec4{ rotatedLightPos, 1.0f } });
	lightingShader->SetUniform("lightColor", colors[lightIdx]);
	lightingShader->SetUniform("projection", projectionMatrix);
	lightingShader->SetUniform("view", viewMatrix);

	lightingShader->SetUniform("enableLight", enableLight);
	lightingShader->SetUniform("ambient", 0.2f);
	lightingShader->SetUniform("specularStrength", 0.25f);
	lightingShader->SetUniform("shininess", 16);

	plane.Render(*lightingShader, viewMatrix);

	lightCube.Render(*lightingShader, viewMatrix);
}

void Application::OnKeyEvent(int key, int action, int mods)
{
	if (action == GLFW_PRESS)
	{
		switch (key)
		{
		case GLFW_KEY_S:
			break;
		case GLFW_KEY_R:
			break;
		case GLFW_KEY_N:
			break;
		case GLFW_KEY_F:
			break;
		case GLFW_KEY_Q:
			glfwSetWindowShouldClose(m_Window, true);
			break;
		}
	}
}

void Application::OnCharEvent(unsigned int codepoint)
{
	switch (codepoint)
	{
	case '+':
		break;
	case '-':
		break;
	}
}