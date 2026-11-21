#define CG_APPLICATION_CUSTOM_CALLBACK_KEY
#define CG_APPLICATION_CUSTOM_CALLBACK_CHAR

#include <CG.h>
#include <filesystem>
#include <stb/stb_image.h>
#include <array>
#include <algorithm>
#include <cmath>
#include <optional>
#include <string>
#include <vector>

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
	constexpr const char* cubeFile = "data/1_opengl_basics/cube.obj";
	constexpr float beltLength = 7.0f;
	constexpr float pi = 3.14159265358979323846f;

	struct Pose
	{
		Vec3 position{};
		float yaw{};
	};

	std::optional<Shader> skyboxShader{};
	std::optional<Shader> lightingShader{};
	unsigned int cubeMapTextures{};
	unsigned int skyboxVAO{};
	unsigned int skyboxVBO{};
	float skyboxVertices[] = {

    -0.5f, -0.5f, -0.5f,
     0.5f, -0.5f, -0.5f,
     0.5f,  0.5f, -0.5f,
     0.5f,  0.5f, -0.5f,
    -0.5f,  0.5f, -0.5f,
    -0.5f, -0.5f, -0.5f,


    -0.5f, -0.5f,  0.5f,
     0.5f,  0.5f,  0.5f,
     0.5f, -0.5f,  0.5f,
     0.5f,  0.5f,  0.5f,
    -0.5f, -0.5f,  0.5f,
    -0.5f,  0.5f,  0.5f,


    -0.5f,  0.5f,  0.5f,
    -0.5f, -0.5f, -0.5f,
    -0.5f,  0.5f, -0.5f,
    -0.5f, -0.5f, -0.5f,
    -0.5f,  0.5f,  0.5f,
    -0.5f, -0.5f,  0.5f,


     0.5f,  0.5f,  0.5f,
     0.5f,  0.5f, -0.5f,
     0.5f, -0.5f, -0.5f,
     0.5f, -0.5f, -0.5f,
     0.5f, -0.5f,  0.5f,
     0.5f,  0.5f,  0.5f,


    -0.5f, -0.5f, -0.5f,
     0.5f, -0.5f,  0.5f,
     0.5f, -0.5f, -0.5f,
     0.5f, -0.5f,  0.5f,
    -0.5f, -0.5f, -0.5f,
    -0.5f, -0.5f,  0.5f,


    -0.5f,  0.5f, -0.5f,
     0.5f,  0.5f, -0.5f,
     0.5f,  0.5f,  0.5f,
     0.5f,  0.5f,  0.5f,
    -0.5f,  0.5f,  0.5f,
    -0.5f,  0.5f, -0.5f
};
	Object plane{};
	Object conveyor{};
	Object truck{};
	std::array<Object, 18> belt{};
	std::array<Timeline*, 18> beltTimelines{};
	std::array<Object, 5> packages{};
	std::array<Object, 5> cargo{};
	std::array<unsigned int, 5> packageRoots{};
	std::array<unsigned int, 5> cargoRoots{};
	std::array<Timeline*, 5> packageTimelines{};
	std::array<Pose, 5> packagePoses{};

	std::array<AnimationID, 3> packageBeltAnimations{};
	std::array<Pose, 5> loadingStart{};
	std::array<Vec3, 5> loadingTarget{};
	std::array<float, 5> loadingTime{};
	std::array<bool, 5> isLoading{};
	std::array<bool, 5> isLoaded{};

	constexpr float liftHeight = 2.0f;
	constexpr float liftDuration = 1.0f;
	constexpr float travelDuration = 2.0f;
	constexpr float lowerDuration = 0.5f;
	constexpr float loadingDuration = liftDuration + travelDuration + lowerDuration;

	unsigned int truckRoot{};

	bool beltEnabled = true;
	bool truckMoving = false;
	bool firstPerson = false;
	bool perspective = true;
	int selectedPackage = 4;
	float beltClock = 0.0f;
	float truckZ = 1.2f;
	float truckDirection = 1.0f;
	float cameraYaw = 0.0f;
	float cameraDistance = 11.5f;

	Pose BeltPose(float time)
	{
		float t = std::fmod(time, beltLength);
		if (t < 0.0f) t += beltLength;
		if (t < 3.0f)
			return { Vec3{ 0.0f, 0.9f, t }, 0.0f };
		if (t < 4.0f)
		{
			float angle = (t - 3.0f) * pi * 0.5f;
			return { Vec3{ -0.5f + 0.5f * std::cos(angle), 0.9f,
				3.0f + 0.5f * std::sin(angle) }, -glm::degrees(angle) };
		}
		return { Vec3{ -0.5f - (t - 4.0f), 0.9f, 3.5f }, -90.0f };
	}

	void AddTruckPart(const Vec3& pos, const Vec3& size, const Vec4& color)
	{
		truck.Spawn({ .parentID = truckRoot, .file = cubeFile, .color = color,
			.pos = pos, .scale = size });
	}

	Vec3 CargoPosition(int index)
	{
		float x = (index % 2 == 0) ? -0.45f : 0.45f;
		float z = -1.12f + static_cast<float>(index / 2) * 0.82f;
		return Vec3{ 2.85f + x, 1.53f, truckZ + z };
	}

	void RefreshPackagePoses()
	{
		for (int i = 0; i < 5; ++i)
		{
			if (isLoaded[i])
			{
				packagePoses[i] = Pose{ CargoPosition(i), 0.0f };
				cargo[i].EditPos(cargoRoots[i], packagePoses[i].position);
			}
			else if (!isLoading[i])
				packagePoses[i] = BeltPose(beltClock + static_cast<float>(i) * beltLength / 5.0f);
		}
	}

	void RefreshLoadingPose(int i)
	{
		const Pose start = loadingStart[i];
		const Vec3 target = loadingTarget[i];
		const Vec3 topStart = start.position + Vec3{ 0.0f, liftHeight, 0.0f };
		const Vec3 topTarget = target + Vec3{ 0.0f, liftHeight, 0.0f };
		const float t = loadingTime[i];

		if (t < liftDuration)
		{
			const float alpha = std::clamp(t / liftDuration, 0.0f, 1.0f);
			packagePoses[i] = Pose{ glm::mix(start.position, topStart, alpha),
				glm::mix(start.yaw, 0.0f, alpha) };
		}
		else if (t < liftDuration + travelDuration)
		{
			const float alpha = std::clamp((t - liftDuration) / travelDuration, 0.0f, 1.0f);
			packagePoses[i] = Pose{ glm::mix(topStart, topTarget, alpha), 0.0f };
		}
		else
		{
			const float alpha = std::clamp((t - liftDuration - travelDuration) / lowerDuration, 0.0f, 1.0f);
			packagePoses[i] = Pose{ glm::mix(topTarget, target, alpha), 0.0f };
		}
	}

	void Reset()
	{
		beltClock = 0.0f;
		beltEnabled = true;
		truckMoving = false;
		truckZ = 1.2f;
		truckDirection = 1.0f;
		firstPerson = false;
		perspective = true;
		selectedPackage = 4;
		cameraYaw = 0.0f;
		cameraDistance = 11.5f;
		isLoading.fill(false);
		isLoaded.fill(false);
		loadingTime.fill(0.0f);
		truck.EditPos(truckRoot, Vec3{ 2.85f, 0.0f, truckZ });

		for (int i = 0; i < 18; ++i)
			beltTimelines[i]->SetTime(static_cast<float>(i) * beltTimelines[i]->GetDuration() / 18.0f);

		for (int i = 0; i < 5; ++i)
		{
			const unsigned int nodeID = packageRoots[i];
			auto& timeline = *packageTimelines[i];
			timeline.Clear();
			packages[i].EditPos(nodeID, Vec3{ 0.0f, 0.9f, 0.0f });
			packages[i].EditRotation(nodeID, 0.0f, Vec3{ 0.0f, 1.0f, 0.0f });
			const auto trs = packages[i].GetTRS(nodeID);
			timeline.Append(packageBeltAnimations[0], nodeID, trs.t, 3.0f)
				.Append(packageBeltAnimations[1], nodeID, trs.r, 1.0f)
				.Append(packageBeltAnimations[2], nodeID, trs.t, 3.0f);
			timeline.SetLoop(true);
			timeline.PlayAll();
			timeline.SetTime(static_cast<float>(i) * timeline.GetDuration() / 5.0f);
			cargo[i].EditPos(cargoRoots[i], CargoPosition(i));
		}
		RefreshPackagePoses();
	}

}

int main()
{
	Application app{ 1280, 960 };
	if (int code = app.Init("Practice 24 - Logistics Center")) return code;
	glEnable(GL_DEPTH_TEST);
	app.Run();
}

void Application::OnInit()
{
	skyboxShader.emplace("shaders/skybox.vs", "shaders/skybox.fs");
	lightingShader.emplace("shaders/1_opengl_basics/projectionViewNormal.vs",
		"shaders/1_opengl_basics/directionalPhongLightingShader.fs");
	lightingShader->Bind();
	lightingShader->SetUniform("lightColor", Vec3{ 1.0f });

	std::vector<std::string> cubemapFiles{
		"data\\skybox\\right.png",
		"data\\skybox\\left.png",
		"data\\skybox\\up.png",
		"data\\skybox\\down.png",
		"data\\skybox\\front.png",
		"data\\skybox\\back.png"
	};
	cubeMapTextures = LoadCubemap(cubemapFiles);
	glGenVertexArrays(1, &skyboxVAO);
	glBindVertexArray(skyboxVAO);
	glGenBuffers(1, &skyboxVBO);
	glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, (const void*)0);
	AppendCamera(Vec3{ 7.0f, 8.0f, 11.0f });
	m_Cameras[0].FocusAt(Vec3{ -0.6f, 0.7f, 1.8f });

	plane.Spawn({ .file = "data/1_opengl_basics/cubePlane.obj",
		.color = Vec4{ 0.37f, 0.43f, 0.39f, 1.0f }, .scale = Vec3{ 24.0f, 1.0f, 24.0f } });
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
	packageBeltAnimations = { moveZ, rotate, moveX };

	for (int i = 0; i < 18; ++i)
	{
		auto stripe = belt[i].Spawn({ .file = cubeFile,
			.color = Vec4{ 0.11f, 0.11f, 0.20f, 1.0f },
			.pos = Vec3{ 0.0f, 0.55f, 0.025f }, .scale = Vec3{ 1.0f, 0.025f, 0.05f } });
		auto& timeline = belt[i].CreateTimeline();
		timeline.Append(moveZ, stripe.nodeID, stripe.trs.t, 3.0f)
			.Append(rotate, stripe.nodeID, stripe.trs.r, 1.0f)
			.Append(moveX, stripe.nodeID, stripe.trs.t, 3.0f);
		timeline.SetLoop(true);
		beltTimelines[i] = &timeline;
	}

	std::array<Vec3, 5> colors{
		Vec3{ 0.95f, 0.23f, 0.25f },
		Vec3{ 1.00f, 0.53f, 0.16f },
		Vec3{ 0.97f, 0.88f, 0.18f },
		Vec3{ 0.21f, 0.76f, 0.38f },
		Vec3{ 0.14f, 0.48f, 0.93f }
	};
	for (int i = 0; i < 5; ++i)
	{
		auto box = packages[i].Spawn({ .file = cubeFile, .color = Vec4{ colors[i], 1.0f },
			.pos = Vec3{ 0.0f, 0.9f, 0.0f }, .scale = Vec3{ 0.8f } });
		packageRoots[i] = box.nodeID;
		auto cargoBox = cargo[i].Spawn({ .file = cubeFile, .color = Vec4{ colors[i], 1.0f },
			.pos = Vec3{ 0.0f, 1.53f, 0.0f }, .scale = Vec3{ 0.8f } });
		cargoRoots[i] = cargoBox.nodeID;
		for (int j = 0; j <= i; ++j)
		{
			float x = (static_cast<float>(j) - static_cast<float>(i) * 0.5f) * 0.135f;
			packages[i].Spawn({ .parentID = box.nodeID, .file = cubeFile,
				.color = Vec4{ 0.09f, 0.09f, 0.13f, 1.0f },
				.pos = Vec3{ x, 0.0f, -0.41f }, .scale = Vec3{ 0.075f, 0.22f, 0.025f } });
			cargo[i].Spawn({ .parentID = cargoBox.nodeID, .file = cubeFile,
				.color = Vec4{ 0.09f, 0.09f, 0.13f, 1.0f },
				.pos = Vec3{ x, 0.0f, -0.41f }, .scale = Vec3{ 0.075f, 0.22f, 0.025f } });
		}
		auto& timeline = packages[i].CreateTimeline();
		packageTimelines[i] = &timeline;
	}

	truckRoot = truck.AddEmptyNode();
	AddTruckPart(Vec3{ 0.0f, 0.52f, 0.0f }, Vec3{ 2.1f, 0.27f, 3.6f }, Vec4{ 0.17f, 0.20f, 0.25f, 1.0f });
	AddTruckPart(Vec3{ 0.0f, 1.03f, -0.48f }, Vec3{ 2.05f, 0.17f, 2.50f }, Vec4{ 0.57f, 0.59f, 0.65f, 1.0f });
	AddTruckPart(Vec3{ -0.99f, 1.45f, -0.48f }, Vec3{ 0.13f, 0.85f, 2.50f }, Vec4{ 0.36f, 0.40f, 0.48f, 1.0f });
	AddTruckPart(Vec3{ 0.99f, 1.45f, -0.48f }, Vec3{ 0.13f, 0.85f, 2.50f }, Vec4{ 0.36f, 0.40f, 0.48f, 1.0f });
	AddTruckPart(Vec3{ 0.0f, 1.45f, -1.71f }, Vec3{ 2.05f, 0.85f, 0.13f }, Vec4{ 0.36f, 0.40f, 0.48f, 1.0f });
	AddTruckPart(Vec3{ 0.0f, 1.28f, 1.15f }, Vec3{ 1.88f, 1.55f, 1.15f }, Vec4{ 0.22f, 0.42f, 0.68f, 1.0f });
	AddTruckPart(Vec3{ 0.0f, 1.61f, 1.76f }, Vec3{ 1.53f, 0.65f, 0.07f }, Vec4{ 0.24f, 0.32f, 0.41f, 1.0f });
	for (float x : { -0.93f, 0.93f })
		for (float z : { -1.22f, 1.18f })
			AddTruckPart(Vec3{ x, 0.41f, z }, Vec3{ 0.35f, 0.80f, 0.80f }, Vec4{ 0.07f, 0.07f, 0.09f, 1.0f });

	Reset();
}

void Application::Clear()
{
	glClearColor(0.62f, 0.75f, 0.92f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Application::Update()
{
	float dt = std::clamp(deltaTime, 0.0f, 0.05f);
	if (beltEnabled)
	{
		beltClock = std::fmod(beltClock + dt, beltLength);
		for (auto& stripe : belt)
			stripe.Update(dt);
	}
	if (truckMoving)
	{
		truckZ += truckDirection * dt * 2.2f;
		if (truckZ >= 10.2f || truckZ <= -10.2f)
		{
			truckZ = std::clamp(truckZ, -10.2f, 10.2f);
			truckMoving = false;
			truckDirection = -truckDirection;
		}
	}
	truck.EditPos(truckRoot, Vec3{ 2.85f, 0.0f, truckZ });
	plane.Update(dt);
	conveyor.Update(dt);
	truck.Update(dt);
	for (int i = 0; i < 5; ++i)
	{
		if (isLoading[i])
		{
			packages[i].Update(dt);
			loadingTime[i] = std::min(loadingTime[i] + dt, loadingDuration);
			RefreshLoadingPose(i);
			if (loadingTime[i] >= loadingDuration)
			{
				isLoading[i] = false;
				isLoaded[i] = true;
			}
		}
		else if (!isLoaded[i] && beltEnabled)
			packages[i].Update(dt);
	}
	RefreshPackagePoses();
	for (int i = 0; i < 5; ++i)
		if (isLoaded[i]) cargo[i].Update(dt);
}

void Application::Render(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix, int cameraIdx)
{
	int width = 1280, height = 960;
	glfwGetFramebufferSize(m_Window, &width, &height);
	float aspect = static_cast<float>(std::max(width, 1)) / static_cast<float>(std::max(height, 1));
	Mat4 view{ 1.0f };
	Mat4 projection{ 1.0f };

	if (firstPerson)
	{
		const auto& pose = packagePoses[selectedPackage];
		float yaw = glm::radians(pose.yaw);
		Vec3 forward{ std::sin(yaw), -0.05f, std::cos(yaw) };
		Vec3 eye = pose.position + Vec3{ 0.0f, 0.55f, 0.0f };
		view = glm::lookAt(eye, eye + forward, Vec3{ 0.0f, 1.0f, 0.0f });
		projection = glm::perspective(glm::radians(65.0f), aspect, 0.03f, 120.0f);
	}
	else
	{
		Vec3 target{ -0.6f, 0.7f, 1.8f };
		Mat4 orbit = glm::translate(Mat4{ 1.0f }, target)
			* glm::rotate(Mat4{ 1.0f }, cameraYaw, Vec3{ 0.0f, 1.0f, 0.0f })
			* glm::translate(Mat4{ 1.0f }, -target);
		view = glm::translate(Mat4{ 1.0f }, Vec3{ 0.0f, 0.0f, 11.5f - cameraDistance })
			* viewMatrix * glm::inverse(orbit);
		projection = projectionMatrix;
		if (!perspective)
		{
			Vec3 eye = Vec3(glm::inverse(view)[3]);
			float halfHeight = std::max(2.0f, glm::length(eye - target) * 0.54f);
			projection = glm::ortho(-halfHeight * aspect, halfHeight * aspect,
				-halfHeight, halfHeight, 0.05f, 120.0f);
		}
	}

	glDepthMask(GL_FALSE);
	glBindVertexArray(skyboxVAO);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMapTextures);
	skyboxShader->Bind();
	skyboxShader->SetUniform("projection", projection);
	skyboxShader->SetUniform("view", Mat4{ glm::mat3(view) });
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glDepthMask(GL_TRUE);

	CG::SetLightingParams(lightingShader, view, projection, Vec3{ 0.0f, -1.0f, 1.0f }, 0.30f, 0.20f, 8);
	plane.Render(*lightingShader, view);
	conveyor.Render(*lightingShader, view);
	for (auto& stripe : belt)
		stripe.Render(*lightingShader, view);
	truck.Render(*lightingShader, view);
	for (int i = 0; i < 5; ++i)
	{
		if (isLoaded[i]) cargo[i].Render(*lightingShader, view);
		else packages[i].Render(*lightingShader, view);
	}
}

void Application::OnKeyEvent(int key, int action, int mods)
{
	if (action != GLFW_PRESS && action != GLFW_REPEAT) return;
	bool shift = (mods & GLFW_MOD_SHIFT) != 0;
	if (key >= GLFW_KEY_1 && key <= GLFW_KEY_5 && action == GLFW_PRESS)
	{
		const unsigned int idx = key - GLFW_KEY_1;
		if (isLoading[idx] || isLoaded[idx]) return;
		if (glm::distance(packagePoses[idx].position, Vec3{ 0.0f, 0.5f, truckZ }) >= 2.0f) return;
		truckMoving = false;
		loadingStart[idx] = packagePoses[idx];
		loadingTarget[idx] = CargoPosition(idx);
		loadingTime[idx] = 0.0f;

		auto& timeline = *packageTimelines[idx];
		timeline.Clear();
		const unsigned int nodeID = packageRoots[idx];
		packages[idx].EditPos(nodeID, loadingStart[idx].position);
		packages[idx].EditRotation(nodeID, loadingStart[idx].yaw, Vec3{ 0.0f, 1.0f, 0.0f });
		const auto trs = packages[idx].GetTRS(nodeID);

		auto& global = AnimationHandler::Global();
		const auto upPath = global.SetTranslateAnimation(Vec3{ 0.0f, liftHeight, 0.0f });
		const auto toTargetPath = global.SetTranslateAnimation(loadingTarget[idx] - loadingStart[idx].position);
		const auto toDownPath = global.SetTranslateAnimation(Vec3{ 0.0f, -liftHeight, 0.0f });
		const auto rotate = global.SetRotateAnimation(
			AnimationAngle{ .from = 0.0f, .to = -loadingStart[idx].yaw },
			Vec3{ 0.0f, 1.0f, 0.0f });

		auto& lift = timeline.AddEntry();
		lift.Append(upPath, nodeID, trs.t, liftDuration);
		lift.Append(rotate, nodeID, trs.r, liftDuration);
		timeline.AddEntry().Append(toTargetPath, nodeID, trs.t, travelDuration);
		timeline.AddEntry().Append(toDownPath, nodeID, trs.t, lowerDuration);
		timeline.SetLoop(false);
		timeline.SetTime(0.0f);
		timeline.PlayAll();
		isLoading[idx] = true;
		return;
	}

	switch (key)
	{
	case GLFW_KEY_B:
		if (action == GLFW_PRESS) beltEnabled = !beltEnabled;
		break;
	case GLFW_KEY_V:
		if (action == GLFW_PRESS) firstPerson = !firstPerson;
		break;
	case GLFW_KEY_G:
		if (action == GLFW_PRESS && std::none_of(isLoading.begin(), isLoading.end(), [](bool value) { return value; }))
			truckMoving = !truckMoving;
		break;
	case GLFW_KEY_P:
		if (action == GLFW_PRESS) perspective = !perspective;
		break;
	case GLFW_KEY_LEFT:
		cameraYaw += 0.065f;
		break;
	case GLFW_KEY_RIGHT:
		cameraYaw -= 0.065f;
		break;
	case GLFW_KEY_EQUAL:
	case GLFW_KEY_KP_ADD:
		cameraDistance = std::max(3.0f, cameraDistance - 0.55f);
		break;
	case GLFW_KEY_MINUS:
	case GLFW_KEY_KP_SUBTRACT:
		cameraDistance = std::min(28.0f, cameraDistance + 0.55f);
		break;
	case GLFW_KEY_R:
		if (action == GLFW_PRESS) Reset();
		break;
	case GLFW_KEY_Q:
		glfwSetWindowShouldClose(m_Window, true);
		break;
	}
}

void Application::OnCharEvent(unsigned int codepoint)
{
	switch (codepoint)
	{
	case 'n':
		if (firstPerson)
			selectedPackage = (selectedPackage + 1) % 5;
		break;
	case 'N':
		if (firstPerson)
			selectedPackage = (selectedPackage + (4)) % 5;
		break;
	}
}