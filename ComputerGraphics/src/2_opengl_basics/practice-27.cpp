#define CG_APPLICATION_CUSTOM_CALLBACK_KEY
#define CG_APPLICATION_CUSTOM_CALLBACK_CHAR

#include <CG.h>

namespace
{
	std::optional<Shader> lightingShader{};
	std::optional<Shader> uniformColorShader{};

	std::vector<Vec3> colors{
		Vec3{ 1.0f, 0.0f, 0.0f },
		Vec3{ 0.0f, 1.0f, 0.0f },
		Vec3{ 0.0f, 0.0f, 1.0f }
	};

	ObjectCollection objects{};
	Object lightCube{};

	Vec3 lightPos{ 0.0f, 0.0f, 20.0f };

	Timeline* rotatelightCube{};

	SpawnedNode lightCubeNode{};

	int lightIdx{ 0 };
	bool enableLight{};
}

int main()
{
	Application app{ 1280, 960 };
	if (int code = app.Init("i hate computer graphics 7")) return code;
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
	AppendCamera(glm::vec3(0.0f, 0.0f, 10.0f));
	m_Cameras[0].FocusAt(Vec3{ 0.0f });

	objects.SpawnOpaque({ .file = "data/1_opengl_basics/sphere.obj",
		.color = Vec3{ 1.0f, 0.0f, 0.0f }, .pos = Vec3{ 0.0f, 0.0f, 0.0f } });
	objects.SpawnOpaque({ .file = "data/1_opengl_basics/sphere.obj",
		.color = Vec3{ 0.0f, 1.0f, 0.0f }, .pos = Vec3{ -2.5f, 0.0f, 0.0f }, .scale = Vec3{ 0.5f } });
	objects.SpawnOpaque({ .file = "data/1_opengl_basics/sphere.obj",
		.color = Vec3{ 0.0f, 0.0f, 1.0f }, .pos = Vec3{ -4.5f, 0.0f, 0.0f }, .scale = Vec3{ 0.25f } });

	lightCubeNode = lightCube.SpawnOpaque({ .file = "data/1_opengl_basics/cubePrimitive.obj",
		.color = colors[0], .pos = lightPos, .scale = Vec3{0.5f}});
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
		lightCubeNode.trs.t) * Vec4 {lightPos, 1.0f} };

	lightingShader->SetUniform("lightPos", Vec3{ viewMatrix * Vec4{ rotatedLightPos, 1.0f } });
	lightingShader->SetUniform("lightColor", colors[lightIdx]);
	lightingShader->SetUniform("projection", projectionMatrix);
	lightingShader->SetUniform("view", viewMatrix);

	lightingShader->SetUniform("enableLight", true);
	lightingShader->SetUniform("ambient", 0.2f);
	lightingShader->SetUniform("specularStrength", 0.25f);
	lightingShader->SetUniform("shininess", 16);

	objects.Render(*lightingShader, viewMatrix);

	lightCube.Render(*lightingShader, viewMatrix);
}

void Application::OnKeyEvent(int key, int action, int mods)
{
	if (action == GLFW_PRESS)
	{
		switch (key)
		{
		case GLFW_KEY_C:
			lightIdx = (lightIdx + 1) % 3;
			lightCube.SetColor(lightCubeNode.nodeID, colors[lightIdx]);
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
	case 'r':
		if (rotatelightCube->isPlaying())
		{
			rotatelightCube->Stop();
		}
		else
		{
			rotatelightCube->Play();
			rotatelightCube->SetDirection(AnimationPlay::Forward);
		}
		break;
	case 'R':
		if (rotatelightCube->isPlaying())
		{
			rotatelightCube->Stop();
		}
		else
		{
			rotatelightCube->Play();
			rotatelightCube->SetDirection(AnimationPlay::Backward);
		}
		break;
	}
}