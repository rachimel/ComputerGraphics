#define CG_APPLICATION_CUSTOM_CALLBACK_KEY
#define CG_APPLICATION_CUSTOM_CALLBACK_CHAR

#include <CG.h>

namespace
{
	std::optional<Shader> lightingShader{};
	std::optional<Shader> uniformColorShader{};

	Object cube{};

	Object trail{};
	Object pyramid{};

	Object lightCube{};
	Object lightCubeTrail{};

	float lightRadius{2.0f};
	Vec3 lightPos{ 0.0f, 0.0f, lightRadius };

	Timeline* rotateCube{};
	Timeline* rotatePyramid{};

	Timeline* rotatelightCube{};

	SpawnedNode lightCubeNode{};
	SpawnedNode trailNode{};

	bool enableLight{};
	bool renderPyramid{};
	bool reverse{};
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
	lightingShader->SetUniform("lightColor", Vec3{ 1.0f });
	AppendCamera(glm::vec3(0.0f, 10.0f, 0.0f));
	m_Cameras[0].FocusAt(Vec3{ 0.0f });

	lightCubeNode = lightCube.SpawnOpaque({ .file = "data/1_opengl_basics/cubePrimitive.obj",
		.color = Vec3{ 1.0f, 1.0f, 1.0f },.pos = lightPos, .scale = Vec3{0.5f}});

	auto cubeNode = cube.SpawnOpaque({ .file = "data/1_opengl_basics/cubePrimitive.obj",
	   .color = Vec3{ 0.0f, 1.0f, 0.0f }, .scale = Vec3{ 1.0f } });

	auto pyramidNode = pyramid.SpawnOpaque({ .file = "data/1_opengl_basics/pyramid.obj",
	   .color = Vec3{ 0.0f, 1.0f, 0.0f }, .scale = Vec3{ 2.0f } });

	trailNode = trail.SpawnOpaque({ .file = "data/1_opengl_basics/circle.obj",
		.color = Vec3{ 0.0f, 1.0f, 0.0f }, .drawingMode{ DrawingMode::Outline } });


	auto& global = AnimationHandler::Global();

	auto rotateCubeID = global.SetRotateAnimation(360.0f, Vec3{ 0.0f, 1.0f, 0.0f });
	auto rotatePyramidID = global.SetRotateAnimation(360.0f, Vec3{ 0.0f, 1.0f, 0.0f });


	auto rotatelightCubeID = global.SetRotateAnimation(360.0f, Vec3{ 0.0f, 1.0f, 0.0f });
	auto rotatelightCubeReverseID = global.SetRotateAnimation(-360.0f, Vec3{ 0.0f, 1.0f, 0.0f });

	auto& cubeT = cube.CreateTimeline();
	auto& pyramidT = pyramid.CreateTimeline();
	auto& lightCubeT = lightCube.CreateTimeline();

	cubeT.AddEntry().Append(rotateCubeID, cubeNode.nodeID, cubeNode.trs.r, 5.0f);
	pyramidT.AddEntry().Append(rotatePyramidID, pyramidNode.nodeID, pyramidNode.trs.r, 5.0f);

	lightCubeT.AddEntry().Append(
		rotatelightCubeID,
		lightCubeNode.nodeID,
		lightCubeNode.trs.t,
		5.0f,
		AnimationPlacement::Before
	);

	lightCubeT.Pause();

	rotateCube = &cubeT;
	rotatePyramid = &pyramidT;
	rotatelightCube = &lightCubeT;

	rotateCube->Stop();
	rotatePyramid->Stop();
	rotatelightCube->Stop();

	rotateCube->SetLoop(true);
	rotatePyramid->SetLoop(true);
	rotatelightCube->SetLoop(true);
}

void Application::Clear()
{
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Application::Update()
{
	cube.Update(deltaTime);
	pyramid.Update(deltaTime);

	float duration = rotatelightCube->GetDuration();
	float direction = reverse ? -1.0f : 1.0f;

	if(rotatelightCube->isPlaying()){
		float next = std::fmod(
			rotatelightCube->GetTime() + deltaTime * direction,
			duration
		);

		if (next < 0.0f)
			next += duration;

		rotatelightCube->SetTime(next);
	}

	trailNode.EditScale(Vec3{ lightRadius, 1.0f, lightRadius });
}

void Application::Render(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix, int cameraIdx)
{
	lightingShader->Bind();
	Vec3 rotatedLightPos = Vec3{ *rotatelightCube->FindTransform(lightCubeNode.nodeID,
	lightCubeNode.trs.t) * Vec4 {
lightPos, 1.0f
} };
	lightingShader->SetUniform("lightPos", Vec3{ viewMatrix * Vec4{rotatedLightPos, 1.0f } });

	lightingShader->SetUniform("projection", projectionMatrix);
	lightingShader->SetUniform("view", viewMatrix);
	
	lightingShader->SetUniform("enableLight", enableLight);
	lightingShader->SetUniform("ambient", 0.2f);
	lightingShader->SetUniform("specularStrength", 0.0f);
	lightingShader->SetUniform("shininess", 1);

	if (renderPyramid)
	{
		pyramid.Render(*lightingShader, viewMatrix);
	}
	else
	{
		cube.Render(*lightingShader, viewMatrix);
	}

	lightCube.Render(*lightingShader, viewMatrix);

	uniformColorShader->Bind();
	uniformColorShader->SetUniform("projection", projectionMatrix);
	uniformColorShader->SetUniform("view", viewMatrix)	;

	trail.Render(*uniformColorShader, viewMatrix);
}

void Application::OnKeyEvent(int key, int action, int mods)
{
	if (action == GLFW_PRESS)
	{
		switch (key)
		{
		case GLFW_KEY_N:
			renderPyramid = !renderPyramid;
			if (renderPyramid)
			{
				if (rotateCube->isPlaying())
				{
					rotateCube->Pause();
					float t = rotateCube->GetT();
					rotatePyramid->SetT(t);
					rotatePyramid->Play();
				}
			}
			else
			{
				if (rotatePyramid->isPlaying())
				{
					rotatePyramid->Pause();
					float t = rotatePyramid->GetT();
					rotateCube->SetT(t);
					rotateCube->Play();
				}
			}
			break;
		case GLFW_KEY_M:
		{
			enableLight = !enableLight;
			break;
		}
		case GLFW_KEY_Y:
		{
			if (renderPyramid)
			{
				if (rotatePyramid->isPlaying())
				{
					rotatePyramid->Stop();
				}
				else
				{
					rotatePyramid->Play();
				}
			}
			else
			{
				if (rotateCube->isPlaying())
				{
					rotateCube->Stop();
				}
				else
				{
					rotateCube->Play();
				}
			}
			break;
		}
		case GLFW_KEY_R:
		{
			if (rotatelightCube->isPlaying() && reverse)
			{
				rotatelightCube->Stop();
			}
			else if (rotatelightCube->isPlaying() &&  !reverse)
			{
				reverse = true;
			}
			else
			{
				rotatelightCube->Play();
			}
			break;
		}
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
		case 'z':
			if (lightPos.z <= 2.0f) break;
			lightPos.z -= 1.0f;
			lightRadius -= 1.0f;
			lightCubeNode.EditPos(lightPos);
			break;
		case 'Z':
			lightPos.z += 1.0f;
			lightRadius += 1.0f;
			lightCubeNode.EditPos(lightPos);
			break;
	}
}