#define CG_APPLICATION_CUSTOM_CALLBACK_KEY
#include <CG.h>

namespace
{
	std::optional<Shader> lightingShader{};
	Object plane{};
	std::unique_ptr<CG::AnimatedCube> animatedCube{};
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
		"shaders/1_opengl_basics/directionalPhongLightingShader.fs"); 
	lightingShader->Bind();
	lightingShader->SetUniform("lightColor", Vec3{ 1.0f });
	AppendCamera(glm::vec3(0.0f, 10.0f, 0.0f));
	m_Cameras[0].FocusAt(Vec3{ 0.0f });

	auto& planeFile = "data/1_opengl_basics/plane.obj";
	plane.Spawn({ .file = planeFile, .color = Vec4{ 0.5f, 0.5f, 0.5f, 1.0f },
		.scale = Vec3{ 100.0f, 1.0f, 100.0f } });

	std::array<Vec4, 6> cubeColors{
		Vec4{ 1.0f },
		Vec4{ 1.0f },
		Vec4{ 1.0f },
		Vec4{ 1.0f },
		Vec4{ 1.0f },
		Vec4{ 1.0f }
	};

	auto& animations = AnimationHandler::Global();
	auto rotate = animations.SetRotateAnimation(-90.0f, Vec3{ 1.0f, 0.0f, 0.0f },
		Vec3{0.0f, -0.0f, -0.5f}, {.speed = 0.1f});

	auto& cubePlaneFile = "data/1_opengl_basics/cubePlane.obj";
	animatedCube = std::make_unique<CG::AnimatedCube>(cubePlaneFile,
		cubeColors);
	animatedCube->EditPos(Vec3{ 0.0f, 0.5f, 0.0f });

	auto& frontFace = animatedCube->Face(CG::CubeFace::Front);

	auto& t = animatedCube->object.CreateTimeline();
	t.Append(rotate, frontFace.nodeID, frontFace.trs.r);
}

void Application::Clear()
{
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Application::Update()
{
	plane.Update(deltaTime);
	animatedCube->Update(deltaTime);
}

void Application::Render(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix, int cameraIdx)
{
	CG::SetLightingParams(lightingShader,
		viewMatrix, projectionMatrix, glm::vec3(0.0f, -1.0f, 1.0f),
		0.2f, 0.2f, 4);

	plane.Render(*lightingShader, viewMatrix);
	animatedCube->Render(*lightingShader, viewMatrix);
}

void Application::OnKeyEvent(int key, int action, int mods)
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