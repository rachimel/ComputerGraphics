#define CG_APPLICATION_CUSTOM_CALLBACK_KEY
#include <CG.h>
#include <filesystem>
#include <stb/stb_image.h>

unsigned int LoadCubemap(const std::vector<std::string>& cubemaps)
{
	stbi_set_flip_vertically_on_load(false);
	unsigned int textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

	int width, height, nrChannels;
	for (unsigned int i = 0; i < cubemaps.size(); ++i)
	{
		auto path = std::filesystem::current_path() / cubemaps[i];
		unsigned char* data = stbi_load(path.string().c_str(), &width, &height, &nrChannels, 3);

		if (data)
		{
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width,
				height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
			stbi_image_free(data);
		}
		else
		{
			std::println("[STB Image] : failed to load cube map file : {}", path.string());
			stbi_image_free(data);
		}
	}

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	stbi_set_flip_vertically_on_load(true);
	return textureID;
}

namespace
{
	std::vector<Model> containers{};
	std::optional<Shader> skyboxShader{};
	std::optional<Shader> lightingShader{};

	unsigned int cubeMapTextures{};
	unsigned int skyboxVAO, skyboxVBO;

	float skyboxVertices[] = {
    // -Z
    -0.5f, -0.5f, -0.5f,
     0.5f, -0.5f, -0.5f,
     0.5f,  0.5f, -0.5f,
     0.5f,  0.5f, -0.5f,
    -0.5f,  0.5f, -0.5f,
    -0.5f, -0.5f, -0.5f,

    // +Z
    -0.5f, -0.5f,  0.5f,
     0.5f,  0.5f,  0.5f,
     0.5f, -0.5f,  0.5f,
     0.5f,  0.5f,  0.5f,
    -0.5f, -0.5f,  0.5f,
    -0.5f,  0.5f,  0.5f,

    // -X
    -0.5f,  0.5f,  0.5f,
    -0.5f, -0.5f, -0.5f,
    -0.5f,  0.5f, -0.5f,
    -0.5f, -0.5f, -0.5f,
    -0.5f,  0.5f,  0.5f,
    -0.5f, -0.5f,  0.5f,

    // +X
     0.5f,  0.5f,  0.5f,
     0.5f,  0.5f, -0.5f,
     0.5f, -0.5f, -0.5f,
     0.5f, -0.5f, -0.5f,
     0.5f, -0.5f,  0.5f,
     0.5f,  0.5f,  0.5f,

    // -Y
    -0.5f, -0.5f, -0.5f,
     0.5f, -0.5f,  0.5f,
     0.5f, -0.5f, -0.5f,
     0.5f, -0.5f,  0.5f,
    -0.5f, -0.5f, -0.5f,
    -0.5f, -0.5f,  0.5f,

    // +Y
    -0.5f,  0.5f, -0.5f,
     0.5f,  0.5f, -0.5f,
     0.5f,  0.5f,  0.5f,
     0.5f,  0.5f,  0.5f,
    -0.5f,  0.5f,  0.5f,
    -0.5f,  0.5f, -0.5f
};

	Object plane{};
	Object conveyor{};
	ObjectCollection cubes{};
	ObjectCollection belt{};
	bool beltEnabled{ false };
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
	skyboxShader.emplace("shaders/skybox.vs", "shaders/skybox.fs");
	lightingShader.emplace("shaders/1_opengl_basics/projectionViewNormal.vs",
		"shaders/1_opengl_basics/directionalPhongLightingShader.fs");
	lightingShader->Bind();
	lightingShader->SetUniform("lightColor", glm::vec3(1.0f));

	std::vector<std::string> cubemapFiles{
		"data\\skybox\\right.png",
		"data\\skybox\\left.png",
		"data\\skybox\\up.png",
		"data\\skybox\\down.png",
		"data\\skybox\\front.png",
		"data\\skybox\\back.png",
	};
	cubeMapTextures = LoadCubemap(cubemapFiles);

	glGenVertexArrays(1, &skyboxVAO);
	glBindVertexArray(skyboxVAO);

	glGenBuffers(1, &skyboxVBO);
	glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, (const void*)0);

	AppendCamera(glm::vec3(0.0f, 10.0f, 0.0f));

	auto& planeFile = "data/1_opengl_basics/plane.obj";
	plane.Spawn({ .file = planeFile, .color = Vec4{ 0.5f, 0.5f, 0.5f, 1.0f },
		.scale = Vec3{ 100.0f, 1.0f, 100.0f } });
	
	auto& cubeFile = "data/1_opengl_basics/cube.obj";
	for (int i = 0; i < 5; ++i)
	{
		conveyor.Spawn(
			{ .file = cubeFile, .color = Vec4{ Vec3(0.4f),1.0f },
			  .pos = Vec3{ -0.5f, 0.25f, static_cast<float>(i) },
			  .scale = Vec3{ 0.1f, 0.5f, 0.1f }
			}
		);
		conveyor.Spawn(
			{ .file = cubeFile, .color = Vec4{ Vec3(0.4f),1.0f },
			  .pos = Vec3{ 0.5f, 0.25f, static_cast<float>(i) },
			  .scale = Vec3{ 0.1f, 0.5f, 0.1f }
			}
		);
	}

	for (int i = 0; i < 3; ++i)
	{
		conveyor.Spawn(
			{ .file = cubeFile, .color = Vec4{ Vec3(0.4f),1.0f },
			  .pos = Vec3{ -static_cast<float>(i + 1) - 0.5f, 0.25f, 4.0f },
			  .scale = Vec3{ 0.1f, 0.5f, 0.1f }
			}
		);
		conveyor.Spawn(
			{ .file = cubeFile, .color = Vec4{ Vec3(0.4f),1.0f },
			  .pos = Vec3{ -static_cast<float>(i + 1) - 0.5f, 0.25f, 3.0f },
			  .scale = Vec3{ 0.1f, 0.5f, 0.1f }
			}
		);
	}
	conveyor.Spawn({ .file = cubeFile, .color = Vec4{ Vec3(0.4f), 1.0f },
		.pos = Vec3{ 0.0f, 0.5f, 1.5f }, .scale = Vec3{ 1.0f, 0.1f, 3.0f } });

	conveyor.Spawn({ .file = cubeFile, .color = Vec4{ Vec3(0.6f), 1.0f },
		.pos = Vec3(0.0f, 0.5f, 3.5f), .scale = Vec3{ 1.0f, 0.1f, 1.0f } });

	conveyor.Spawn({ .file = cubeFile, .color = Vec4{ Vec3(0.4f), 1.0f },
		.pos = Vec3{ -2.0f, 0.5f, 3.5f }, .scale = Vec3{ 3.0f, 0.1f, 1.0f } });

	conveyor.Spawn({ .file = cubeFile, .color = Vec4{ 0.9f, 0.8f, 0.0f, 1.0f },
		.pos = Vec3{ -0.55f, 0.5f, 1.5f }, .scale = Vec3{ 0.1f, 0.2f, 3.0f } });

	conveyor.Spawn({ .file = cubeFile, .color = Vec4{ 0.9f, 0.8f, 0.0f, 1.0f },
	.pos = Vec3{ 0.55f, 0.5f, 2.0f }, .scale = Vec3{ 0.1f, 0.2f, 4.0f } });

	conveyor.Spawn({ .file = cubeFile, .color = Vec4{ 0.9f, 0.8f, 0.0f, 1.0f },
	.pos = Vec3{ -2.0f, 0.5f, 2.95f }, .scale = Vec3{ 3.0f, 0.2f, 0.1f } });

	conveyor.Spawn({ .file = cubeFile, .color = Vec4{ 0.9f, 0.8f, 0.0f, 1.0f },
	.pos = Vec3{ -1.45f, 0.5f, 4.05f }, .scale = Vec3{ 4.1f, 0.2f, 0.1f } });

	auto& animations = AnimationHandler::Global();
	auto moveZ = animations.SetTranslateAnimation(Vec3{ 0.0f, 0.0f, 3.0f });
	auto rotate = animations.SetRotateAnimation(-90.0f, Vec3{ 0.0f, 1.0f, 0.0f }, Vec3{ -0.5f, 0.0f, 0.0f });
	auto moveX = animations.SetTranslateAnimation(Vec3{ -3.0f, 0.0f, 0.0f });

	for (int i = 0; i < 18; ++i)
	{
		auto [object, nodeID, trs] = belt.Spawn({ .file = cubeFile, .color = Vec4{ 0.1f, 0.1f, 0.2f, 1.0f },
			.pos = Vec3{ 0.0f, 0.55f, 0.025f }, .scale = Vec3{ 1.0f, 0.025f, 0.05f } });

		auto& t = object.CreateTimeline();

		t.Append(moveZ, nodeID, trs.t, 2.0f)
		.Append(rotate, nodeID, trs.r, 1.0f)
		.Append(moveX, nodeID, trs.t, 2.0f);

		t.SetLoop(true);
		t.SetTime(static_cast<float>(i) / 18.0f * t.GetDuration());
	}

	std::array<Color3f, 5> colors{
		Vec3{ 1.0f, 0.0f, 0.0f },
		Vec3{ 1.0f, 0.5f, 0.0f },
		Vec3{ 1.0f, 1.0f, 0.0f },
		Vec3{ 0.0f, 1.0f, 0.0f },
		Vec3{ 0.0f, 0.5f, 1.0f }
	};
	for (int i = 0; i < 5; ++i)
	{
		auto [object, cubeID, trs] =
			cubes.Spawn({
				.file = cubeFile, .color = Vec4{colors[i], 1.0f},
				.pos = Vec3{0.0f, 0.9f, 0.0f}, .scale = Vec3{0.8f}
			});

		for (int j = 0; j < (i + 1); ++j)
		{
			auto nodeID = object.AddModel(cubeID, cubeFile, Vec4{0.1f, 0.1f, 0.2f, 1.0f});
			float nextPlacePos{ 0.16f * static_cast<float>((j + 1) / 2) };
			if (j % 2 == 1) {nextPlacePos = nextPlacePos * -1.0f;}
			object.SetTRS(nodeID, Vec3{nextPlacePos, 0.45f, 0.0f}, Vec3{0.1f, 0.1f, 0.2f});
		}

		auto& timeline = object.CreateTimeline();

		timeline.Append(moveZ, cubeID, trs.t, 3.0f)
			.Append(rotate, cubeID, trs.r, 1.0f)
			.Append(moveX, cubeID, trs.t, 3.0f);

		timeline.SetLoop(true);

		timeline.SetTime(
			static_cast<float>(i) / 5.0f * timeline.GetDuration()
		);
	}

}

void Application::Clear()
{
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Application::Update()
{
	plane.Update(deltaTime);
	conveyor.Update(deltaTime);
	if (beltEnabled)
	{
		belt.Update(deltaTime);
		cubes.Update(deltaTime);
	}
}

void Application::Render(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix, int cameraIdx)
{
	glDepthMask(GL_FALSE);
	glBindVertexArray(skyboxVAO);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMapTextures);
	skyboxShader->Bind();
	skyboxShader->SetUniform("projection", projectionMatrix);
	skyboxShader->SetUniform("view", glm::mat4(glm::mat3(viewMatrix))); // 평행 이동 제거
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glDepthMask(GL_TRUE);

	CG::SetLightingParams(lightingShader,
		viewMatrix, projectionMatrix, glm::vec3(0.0f, -1.0f, 1.0f),
		0.2f, 0.2f, 4);

	plane.Render(*lightingShader, viewMatrix);
	conveyor.Render(*lightingShader, viewMatrix);
	belt.Render(*lightingShader, viewMatrix);
	cubes.Render(*lightingShader, viewMatrix);
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
		case GLFW_KEY_B:
		{
			beltEnabled = !beltEnabled;
			break;
		}
		}
	}
}