#define CG_APPLICATION_CUSTOM_CALLBACK_KEY
#include <CG.h>


#include <filesystem>
#include <print>
#include <string>
#include <vector>

#include <stb/stb_image.h>
#include <Shader.h>
#include <Object.h>

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
	//std::vector<Conveyor> conveyor{};
	//std::vector<RotateConveyor> conveyor{};
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

	ObjectTRS planeTRS{};
	Object plane{};
	AnimationID planeRotation{};
	AnimationID planeScale{};

	std::vector<AnimationID> cubeMoveZ{};
	std::vector<AnimationID> cubeRotate{};
	std::vector<AnimationID> cubeMoveX{};

	ObjectTRS conveyorTRS{};
	Object conveyor{};

	ObjectTRS redCubeTRS{};
	Object redCube{};

	ObjectTRS orangeCubeTRS{};
	Object orangeCube{};

	ObjectTRS yellowCubeTRS{};
	Object yellowCube{};

	ObjectTRS greenCubeTRS{};
	Object greenCube{};

	ObjectTRS blueCubeTRS{};
	Object blueCube{};

	std::vector<ObjectTRS> beltTRS{};
	std::vector<AnimationID> moveZ{};
	std::vector<AnimationID> rotate{};
	std::vector<AnimationID> moveX{};

	Object belt{};
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

	plane.AddModel("data/1_opengl_basics/plane.obj");
	planeTRS = plane.SetTRS(0, Vec3{ 0.0f }, Vec3{ 100.0f, 1.0f, 100.0f });
	plane.SetColor(0, Vec3{ 0.5f });
	

	auto& cubeFile = "data/1_opengl_basics/cube.obj";
	// 9
	for (int i = 0; i < 5; ++i)
	{
		conveyor.AddModel(cubeFile);
		conveyor.SetTRS(2 * i, Vec3{ -0.5f, 0.25f, static_cast<float>(i) },
			Vec3{ 0.1f, 0.5f, 0.1f });
		conveyor.SetColor(2 * i, Vec3(0.4f));

		conveyor.AddModel(cubeFile);
		conveyor.SetTRS(2 * i + 1, Vec3{ 0.5f, 0.25f, static_cast<float>(i) },
			Vec3{ 0.1f, 0.5f, 0.1f });
		conveyor.SetColor(2 * i + 1, Vec3(0.4f));
	}
	// 14
	for (int i = 0; i < 3; ++i)
	{
		conveyor.AddModel(cubeFile);
		conveyor.SetTRS(10 + 2 * i, Vec3{ -static_cast<float>(i + 1) - 0.5f, 0.25f, 4.0f},
			Vec3{ 0.1f, 0.5f, 0.1f });
		conveyor.SetColor(10 + 2 * i, Vec3(0.4f));

		conveyor.AddModel(cubeFile);
		conveyor.SetTRS(10 + 2 * i + 1, Vec3{ -static_cast<float>(i + 1) - 0.5f, 0.25f, 3.0f},
			Vec3{ 0.1f, 0.5f, 0.1f });
		conveyor.SetColor(10 + 2 * i + 1, Vec3(0.4f));
	}

	// 15
	conveyor.AddModel(cubeFile);
	conveyor.SetTRS(16, Vec3{0.0f, 0.5f, 1.5f}, Vec3{1.0f, 0.1f, 3.0f});
	conveyor.SetColor(16, Vec3{ 0.4f });

	conveyor.AddModel(cubeFile);
	conveyor.SetTRS(17, Vec3(0.0f, 0.5f, 3.5f), Vec3{ 1.0f, 0.1f, 1.0f });
	conveyor.SetColor(17, Vec3{ 0.6f });

	conveyor.AddModel(cubeFile);
	conveyor.SetTRS(18, Vec3{ -2.0f, 0.5f, 3.5f }, Vec3{3.0f, 0.1f, 1.0f});
	conveyor.SetColor(18, Vec3(0.4f));

	conveyor.AddModel(cubeFile);
	conveyor.SetTRS(19, Vec3{ -0.55f, 0.5f, 1.5f }, Vec3{ 0.1f, 0.2f, 3.0f });
	conveyor.SetColor(19, Vec3{ 0.9f, 0.8f, 0.0f });

	conveyor.AddModel(cubeFile);
	conveyor.SetTRS(20, Vec3{ 0.55f, 0.5f, 2.0f }, Vec3{ 0.1f, 0.2f, 4.0f });
	conveyor.SetColor(20, Vec3{ 0.9f, 0.8f, 0.0f });

	conveyor.AddModel(cubeFile);
	conveyor.SetTRS(21, Vec3{ -2.0f, 0.5f, 2.95f }, Vec3{ 3.0f, 0.2f, 0.1f });
	conveyor.SetColor(21, Vec3{ 0.9f, 0.8f, 0.0f });

	conveyor.AddModel(cubeFile);
	conveyor.SetTRS(22, Vec3{ -1.45f, 0.5f, 4.05f }, Vec3{ 4.1f, 0.2f, 0.1f });
	conveyor.SetColor(22, Vec3{ 0.9f, 0.8f, 0.0f });

	for (int i = 0; i < 18; ++i)
	{
		belt.AddModel(cubeFile);
		beltTRS.push_back(belt.SetTRS(i,
			Vec3{ 0.0f, 0.55f, 0.025f}, Vec3{1.0f, 0.025f, 0.05f}));
		belt.SetColor(i,Vec3{ 0.1f, 0.1f, 0.2f });
		moveZ.push_back(belt.SetTranslateAnimation(Vec3{ 0.0f, 0.0f, 3.0f },
			PlaybackType::OneWay, { .speed = 0.05f }));
		rotate.push_back(belt.SetRotateAnimation(- 90.0f,
			Vec3{ 0.0f, 1.0f, 0.0f }, Vec3{ -0.5f, 0.5f, 0.0f }, PlaybackType::OneWay, { .speed = 0.191f }));
		moveX.push_back(belt.SetTranslateAnimation(Vec3{ -3.0f, 0.0f, 0.0f },
			PlaybackType::OneWay, { .speed = 0.05f }));

		belt.BindAnimation(i, beltTRS[i].t, moveZ[i]);
		belt.BindAnimation(i, beltTRS[i].r, rotate[i]);
		belt.BindAnimation(i, beltTRS[i].t, moveX[i]);

		if(i / 8 == 0)
		{
			belt.DisableAnimation(moveX[i]);
			belt.DisableAnimation(rotate[i]);
			belt.EnableAnimation(moveZ[i]);
			belt.SetAnimationT(moveZ[i], ((0.9f) / 8.0f) * (i + 1));
		}
		else if (i == 8 || i == 9)
		{
			int iT = (i % 2);
			belt.DisableAnimation(moveZ[i]);
			belt.EnableAnimation(rotate[i]);
			belt.DisableAnimation(moveX[i]);
			belt.SetAnimationT(moveZ[i], (1.0f));
			belt.SetAnimationT(rotate[i], ((0.9f) / 2.0f) * (iT + 1));
		}
		else if (i > 9)
		{
			int iT = (i % 8);
			belt.animationHandler.DisableAnimation(moveZ[i]);
			belt.animationHandler.DisableAnimation(rotate[i]);
			belt.EnableAnimation(moveX[i]);
			belt.animationHandler.SetT(moveZ[i], (1.0f));
			belt.animationHandler.SetT(rotate[i], 1.0f);
			belt.animationHandler.SetT(moveX[i], ((0.9f) / 8.0f) * (iT + 1));
		}
	}


	Object* cube{ nullptr };
	ObjectTRS* cubeTRS{ nullptr };
	for (int i = 0; i < 5; ++i)
	{
		switch (i)
		{
		case 0:
			cube = &redCube;
			cubeTRS = &redCubeTRS;
			cube->AddModel(cubeFile, Vec4{1.0f, 0.0f, 0.0f, 1.0f});
			break;
		case 1:
			cube = &orangeCube;
			cubeTRS = &orangeCubeTRS;
			cube->AddModel(cubeFile, Vec4{ 1.0f, 0.5f, 0.0f, 1.0f });
			break;
		case 2:
			cube = &yellowCube;
			cubeTRS = &yellowCubeTRS;
			cube->AddModel(cubeFile, Vec4{ 1.0f, 1.0f, 0.0f, 1.0f });
			break;
		case 3:
			cube = &greenCube;
			cubeTRS = &greenCubeTRS;
			cube->AddModel(cubeFile, Vec4{ 0.0f, 1.0f, 0.0f, 1.0f });
			break;
		case 4:
			cube = &blueCube;
			cubeTRS = &blueCubeTRS;
			cube->AddModel(cubeFile, Vec4{ 0.0f, 0.5f, 1.0f, 1.0f });
			break;
		}

		for (int j = 0; j < (i + 1); ++j)
		{
			cube->AddModel(0, cubeFile, Vec4{0.1f, 0.1f, 0.2f, 1.0f});
			float nextPlacePos{ 0.16f * static_cast<float>((j + 1) / 2) };
			if (j % 2 == 1) {nextPlacePos = nextPlacePos * -1.0f;}
			cube->SetTRS(j + 1, Vec3{nextPlacePos, 0.45f, 0.0f}, Vec3{0.1f, 0.1f, 0.2f});
		}

		*cubeTRS = cube->SetTRS(0, Vec3{ 0.0f, 0.9f, 0.0f }, Vec3{ 0.8f });

		cubeMoveZ.push_back(cube->SetTranslateAnimation(Vec3{ 0.0f, 0.0f, 3.0f },
			PlaybackType::OneWay, { .speed = 0.05f }));
		cubeRotate.push_back(cube->SetRotateAnimation(-90.0f,
			Vec3{ 0.0f, 1.0f, 0.0f }, Vec3{ -0.5f, 0.5f, 0.0f }, PlaybackType::OneWay, { .speed = 0.191f }));
		cubeMoveX.push_back(cube->SetTranslateAnimation(Vec3{ -3.0f, 0.0f, 0.0f },
			PlaybackType::OneWay, { .speed = 0.05f }));

		cube->BindAnimation(0, cubeTRS->t, cubeMoveZ[i]);
		cube->BindAnimation(0, cubeTRS->r, cubeRotate[i]);
		cube->BindAnimation(0, cubeTRS->t, cubeMoveX[i]);

		switch (i)
		{
		case 0:
			cube->DisableAnimation(cubeMoveX[i]);
			cube->DisableAnimation(cubeRotate[i]);
			break;
		case 1:
			cube->DisableAnimation(cubeMoveX[i]);
			cube->DisableAnimation(cubeRotate[i]);
			cube->SetAnimationT(cubeMoveZ[i], 0.4f);
			break;
		case 2:
			cube->DisableAnimation(cubeMoveX[i]);
			cube->DisableAnimation(cubeRotate[i]);
			cube->SetAnimationT(cubeMoveZ[i], 0.8f);
			break;
		case 3:
			cube->DisableAnimation(cubeMoveZ[i]);
			cube->DisableAnimation(cubeRotate[i]);
			cube->SetAnimationT(cubeMoveZ[i], 1.0f);
			cube->SetAnimationT(cubeRotate[i], 1.0f);
			break;
		case 4:
			cube->DisableAnimation(cubeMoveZ[i]);
			cube->DisableAnimation(cubeRotate[i]);
			cube->SetAnimationT(cubeMoveZ[i], 1.0f);
			cube->SetAnimationT(cubeRotate[i], 1.0f);
			cube->SetAnimationT(cubeMoveX[i], 0.4f);
			break;
		}
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
	belt.Update(deltaTime);

	redCube.Update(deltaTime);
	orangeCube.Update(deltaTime);
	yellowCube.Update(deltaTime);
	greenCube.Update(deltaTime);
	blueCube.Update(deltaTime);

	for (int i = 0; i < 18; ++i)
	{
		bool moveXEnded = belt.animationHandler.isEnded(moveX[i]);
		bool rotateEnded = belt.animationHandler.isEnded(rotate[i]);
		bool moveZEnded = belt.animationHandler.isEnded(moveZ[i]);

		if (moveZEnded && rotateEnded && moveXEnded)
		{
			belt.DisableAnimation(moveX[i]);
			belt.animationHandler.SetT(moveX[i], 0.0f);
			belt.animationHandler.SetT(rotate[i], 0.0f);
			belt.animationHandler.SetT(moveZ[i], 0.0f);
			belt.animationHandler.EnableAnimation(moveZ[i]);
		}
		else if (moveZEnded && rotateEnded)
		{
			belt.DisableAnimation(rotate[i]);
			belt.EnableAnimation(moveX[i]); 
		}
		else if (moveZEnded)
		{
			belt.DisableAnimation(moveZ[i]);
			belt.EnableAnimation(rotate[i]);
		}
	}

	for (int i = 0; i < 5; ++i)
	{
		Object* cube{ nullptr };
		ObjectTRS* cubeTRS{ nullptr };
		switch (i)
		{
		case 0:
			cube = &redCube;
			cubeTRS = &redCubeTRS;
			break;
		case 1:
			cube = &orangeCube;
			cubeTRS = &orangeCubeTRS;
			break;
		case 2:
			cube = &yellowCube;
			cubeTRS = &yellowCubeTRS;
			break;
		case 3:
			cube = &greenCube;
			cubeTRS = &greenCubeTRS;
			break;
		case 4:
			cube = &blueCube;
			cubeTRS = &blueCubeTRS;
			break;
		}

		bool moveXEnded = cube->isAnimationEnded(cubeMoveX[i]);
		bool rotateEnded = cube->isAnimationEnded(cubeRotate[i]);
		bool moveZEnded = cube->isAnimationEnded(cubeMoveZ[i]);

		if (moveZEnded && rotateEnded && moveXEnded)
		{
			cube->DisableAnimation(cubeMoveX[i]);
			cube->SetAnimationT(cubeMoveX[i], 0.0f);
			cube->SetAnimationT(cubeRotate[i], 0.0f);
			cube->SetAnimationT(cubeMoveZ[i], 0.0f);
			cube->EnableAnimation(cubeMoveZ[i]);
		}
		else if (moveZEnded && rotateEnded)
		{
			cube->DisableAnimation(cubeRotate[i]);
			cube->EnableAnimation(cubeMoveX[i]);
		}
		else if (moveZEnded)
		{
			cube->DisableAnimation(cubeMoveZ[i]);
			cube->EnableAnimation(cubeRotate[i]);
		}
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

	lightingShader->Bind();
	lightingShader->SetUniform("lightDir", Vec3{ glm::normalize(viewMatrix * glm::vec4(0.0f, -1.0f, 1.0f, 0.0f)) });

	lightingShader->SetUniform("projection", projectionMatrix);
	lightingShader->SetUniform("view", viewMatrix);

	lightingShader->SetUniform("ambient", 0.2f);
	lightingShader->SetUniform("specularStrength", 0.1f);
	lightingShader->SetUniform("shininess", 4);

	plane.Render(*lightingShader, viewMatrix);
	conveyor.Render(*lightingShader, viewMatrix);
	belt.Render(*lightingShader, viewMatrix);
	redCube.Render(*lightingShader, viewMatrix);
	orangeCube.Render(*lightingShader, viewMatrix);
	yellowCube.Render(*lightingShader, viewMatrix);
	greenCube.Render(*lightingShader, viewMatrix);
	blueCube.Render(*lightingShader, viewMatrix);
}

void Application::OnKeyEvent(GLFWwindow* window, int key, int action, int mods)
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