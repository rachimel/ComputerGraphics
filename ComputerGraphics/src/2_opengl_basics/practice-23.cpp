#define CG_APPLICATION_CUSTOM_CALLBACK_KEY
#define CG_APPLICATION_CUSTOM_CALLBACK_CHAR

#include <CG.h>
#include <array>
#include <algorithm>
#include <cmath>
#include <optional>
#include <random>

using namespace CG;

namespace
{
	constexpr const char* cubeFile = "data/1_opengl_basics/cube.obj";
	constexpr const char* planeFile = "data/1_opengl_basics/cubePlane.obj";
	constexpr float stageHalfX = 5.0f;
	constexpr float stageHalfZ = 4.0f;
	constexpr float stageHeight = 4.4f;
	constexpr float robotRadius = 0.32f;

	struct Obstacle
	{
		Vec3 center{};
		Vec3 size{};
	};

	std::optional<Shader> lightingShader{};
	std::optional<AnimatedCube> stage{};
	Object floorObject{};
	Object obstaclesObject{};
	Object robot{};
	std::array<Obstacle, 3> obstacles{};
	std::array<unsigned int, 4> jointIDs{};
	unsigned int robotRootID{};

	Vec3 position{ 0.0f };
	Vec3 direction{ 0.0f, 0.0f, -1.0f };
	Vec3 cameraPos{};
	float cameraYaw{};
	float cameraPitch{};
	float cameraRotateT{ 0.0f };

	constexpr Vec3 cameraHome{ 8.0f, 6.4f, 11.0f };
	constexpr Vec3 cameraTarget{ 0.0f, 2.1f, 0.0f };
	float walkSpeed = 1.25f;
	float walkPhase = 0.0f;
	float verticalSpeed = 0.0f;
	float frontAmount = 0.0f;

	bool grounded = true;
	bool openFront = false;
	bool rotateCamera{ false };
	float rotateCameraSign{ 1.0f };

	void AddCube(Object& object, const Vec3& pos, const Vec3& size, const Vec4& color)
	{
		object.Spawn({ .file = cubeFile, .color = color, .pos = pos, .scale = size });
	}

	void SetStageFace(CubeFace face, const Vec3& pos, const Vec3& scale)
	{
		auto& node = stage->Face(face);
		node.EditPos(pos);
		node.EditScale(scale);
	}

	void SetFrontHeight(float height)
	{
		SetStageFace(CubeFace::Front,
			Vec3{ 0.0f, (stageHeight - height) * 0.5f, stageHalfZ },
			Vec3{ stageHalfX * 2.0f, 1.0f, height });
	}

	void AddRobotPart(unsigned int parent, const Vec3& pos, const Vec3& size, const Vec4& color)
	{
		robot.Spawn({ .parentID = parent, .file = cubeFile, .color = color, .pos = pos, .scale = size });
	}

	bool Overlaps(const Vec3& pos, const Obstacle& obstacle)
	{
		return std::abs(pos.x - obstacle.center.x) < obstacle.size.x * 0.5f + robotRadius &&
			std::abs(pos.z - obstacle.center.z) < obstacle.size.z * 0.5f + robotRadius;
	}

	bool Outside(const Vec3& pos)
	{
		return std::abs(pos.x) > stageHalfX - 0.42f ||
			std::abs(pos.z) > stageHalfZ - 0.42f;
	}

	bool Blocked(const Vec3& pos)
	{
		if (Outside(pos)) return true;
		for (const auto& obstacle : obstacles)
			if (Overlaps(pos, obstacle) && position.y < obstacle.size.y - 0.08f)
				return true;
		return false;
	}

	float SupportHeight(const Vec3& pos, float previousY)
	{
		float height = 0.0f;
		for (const auto& obstacle : obstacles)
			if (Overlaps(pos, obstacle) && previousY >= obstacle.size.y - 0.1f)
				height = std::max(height, obstacle.size.y);
		return height;
	}

	void Reset(Camera& camera)
	{
		position = Vec3{ 0.0f };
		direction = Vec3{ 0.0f, 0.0f, -1.0f };
		walkSpeed = 1.25f;
		walkPhase = 0.0f;
		verticalSpeed = 0.0f;
		grounded = true;
		frontAmount = 0.0f;
		openFront = false;

		camera.PlaceAt(Vec3{ 0.0f, 5.0f, 10.0f });
		camera.FocusAt(Vec3{ 0.0f, 2.0f, 0.0f });
		robot.EditPos(robotRootID, position);
		robot.EditRotation(robotRootID, 0.0f, Vec3{ 0.0f, 1.0f, 0.0f });
		for (auto id : jointIDs)
			robot.EditRotation(id, 0.0f, Vec3{ 1.0f, 0.0f, 0.0f });
		SetFrontHeight(stageHeight);
	}
}

int main()
{
	Application app{ 1280, 960 };
	if (int code = app.Init("Practice 23 - Robot Stage")) return code;
	glEnable(GL_DEPTH_TEST);
	app.Run();
}

void Application::OnInit()
{
	lightingShader.emplace("shaders/1_opengl_basics/projectionViewNormal.vs",
		"shaders/1_opengl_basics/directionalPhongLightingShader.fs");
	lightingShader->Bind();
	lightingShader->SetUniform("lightColor", Vec3{ 1.0f });
	AppendCamera(cameraHome);
	m_Cameras[0].FocusAt(cameraTarget);

	AddCube(floorObject, Vec3{ 0.0f, -0.10f, 0.0f }, Vec3{ 10.2f, 0.20f, 8.2f }, Vec4{ 0.34f, 0.56f, 0.42f, 1.0f });
	stage.emplace(std::string{ planeFile }, std::array<Vec4, CubeSurfaceCount>{
		Vec4{ 0.21f, 0.34f, 0.61f, 1.0f },
		Vec4{ 0.22f, 0.46f, 0.37f, 1.0f },
		Vec4{ 1.0f },
		Vec4{ 1.0f },
		Vec4{ 0.54f, 0.39f, 0.65f, 1.0f },
		Vec4{ 0.66f, 0.25f, 0.30f, 1.0f }
	});
	stage->EditPos(Vec3{ 0.0f, stageHeight * 0.5f, 0.0f });
	SetStageFace(CubeFace::Right,
		Vec3{ stageHalfX, 0.0f, 0.0f },
		Vec3{ stageHeight, 1.0f, stageHalfZ * 2.0f });
	SetStageFace(CubeFace::Left,
		Vec3{ -stageHalfX, 0.0f, 0.0f },
		Vec3{ stageHeight, 1.0f, stageHalfZ * 2.0f });
	SetStageFace(CubeFace::Back,
		Vec3{ 0.0f, 0.0f, -stageHalfZ },
		Vec3{ stageHalfX * 2.0f, 1.0f, stageHeight });
	SetFrontHeight(stageHeight);
	stage->Face(CubeFace::Top).EditScale(Vec3{ 0.0f });
	stage->Face(CubeFace::Bottom).EditScale(Vec3{ 0.0f });

	std::mt19937 generator{ 23 };
	std::uniform_real_distribution<float> xDistribution(-3.3f, 3.3f);
	std::uniform_real_distribution<float> zDistribution(-2.5f, 2.5f);
	std::uniform_real_distribution<float> widthDistribution(0.8f, 1.3f);
	std::uniform_real_distribution<float> heightDistribution(0.40f, 0.93f);

	for (int i = 0; i < 3; ++i)
	{
		Obstacle candidate{};
		for (int attempt = 0; attempt < 200; ++attempt)
		{
			candidate.size = Vec3{ widthDistribution(generator), heightDistribution(generator), widthDistribution(generator) };
			candidate.center = Vec3{ xDistribution(generator), candidate.size.y * 0.5f, zDistribution(generator) };
			bool valid = glm::length(glm::vec2{ candidate.center.x, candidate.center.z }) > 1.4f;
			for (int j = 0; j < i; ++j)
				if (std::abs(candidate.center.x - obstacles[j].center.x) <
					(candidate.size.x + obstacles[j].size.x) * 0.5f + 0.6f &&
					std::abs(candidate.center.z - obstacles[j].center.z) <
					(candidate.size.z + obstacles[j].size.z) * 0.5f + 0.6f)
					valid = false;
			if (valid) break;
		}
		obstacles[i] = candidate;
		AddCube(obstaclesObject, candidate.center, candidate.size,
			Vec4{ 0.65f + i * 0.08f, 0.34f + i * 0.12f, 0.18f + i * 0.13f, 1.0f });
	}

	robotRootID = robot.AddEmptyNode();
	AddRobotPart(robotRootID, Vec3{ 0.0f, 1.34f, 0.0f }, Vec3{ 0.87f, 0.89f, 0.46f }, Vec4{ 0.43f, 0.58f, 0.86f, 1.0f });
	AddRobotPart(robotRootID, Vec3{ 0.0f, 2.06f, 0.0f }, Vec3{ 0.58f, 0.57f, 0.58f }, Vec4{ 0.87f, 0.80f, 0.58f, 1.0f });
	AddRobotPart(robotRootID, Vec3{ 0.0f, 2.05f, -0.38f }, Vec3{ 0.18f, 0.19f, 0.27f }, Vec4{ 0.88f, 0.27f, 0.30f, 1.0f });

	for (int i = 0; i < 4; ++i)
	{
		const bool arm = i < 2;
		const float side = (i % 2 == 0) ? -1.0f : 1.0f;
		const Vec3 pivotPos = arm ? Vec3{ side * 0.59f, 1.64f, 0.0f } : Vec3{ side * 0.25f, 0.92f, 0.0f };
		auto pivot = robot.GetNode(robot.AddEmptyNode());
		pivot.SetParent(robotRootID);
		pivot.EditPos(pivotPos);
		jointIDs[i] = pivot.nodeID;
		AddRobotPart(pivot.nodeID,
			arm ? Vec3{ 0.0f, -0.34f, 0.0f } : Vec3{ 0.0f, -0.44f, 0.0f },
			arm ? Vec3{ 0.26f, 0.68f, 0.29f } : Vec3{ 0.31f, 0.86f, 0.33f },
			arm ? Vec4{ 0.82f, 0.40f, 0.35f, 1.0f } : Vec4{ 0.34f, 0.73f, 0.62f, 1.0f });
	}

	Reset(m_Cameras[0]);
}

void Application::Clear()
{
	glClearColor(0.09f, 0.11f, 0.18f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Application::Update()
{
	float dt = std::clamp(deltaTime, 0.0f, 0.05f);
	frontAmount = std::clamp(frontAmount + (openFront ? 1.0f : -1.0f) * dt * 1.25f, 0.0f, 1.0f);
	float frontHeight = glm::mix(stageHeight, 0.06f, frontAmount);
	SetFrontHeight(frontHeight);

	Vec3 proposed = position + direction * walkSpeed * dt;
	if (Blocked(proposed))
	{
		if (Outside(proposed))
			direction = -direction;
		else
		{
			std::array<Vec3, 3> turns{
				Vec3{ -direction.z, 0.0f, direction.x },
				Vec3{ direction.z, 0.0f, -direction.x },
				-direction
			};
			for (const auto& turn : turns)
				if (!Blocked(position + turn * walkSpeed * dt * 3.0f))
				{
					direction = turn;
					break;
				}
		}
		proposed = position + direction * walkSpeed * dt;
	}
	if (!Blocked(proposed))
	{
		position.x = proposed.x;
		position.z = proposed.z;
	}

	float previousY = position.y;
	float support = SupportHeight(position, previousY);
	if (grounded && position.y > support + 0.015f)
		grounded = false;
	if (!grounded)
	{
		position.y += verticalSpeed * dt;
		verticalSpeed -= 12.5f * dt;
		if (verticalSpeed <= 0.0f && position.y <= support && previousY >= support)
		{
			position.y = support;
			verticalSpeed = 0.0f;
			grounded = true;
		}
	}
	else
		position.y = support;

	walkPhase += dt * (3.1f + walkSpeed * 3.5f);
	float amplitude = std::clamp(13.0f + walkSpeed * 13.0f, 12.0f, 58.0f);
	float swing = std::sin(walkPhase) * amplitude;
	for (int i = 0; i < 4; ++i)
	{
		float sign = (i == 0 || i == 3) ? 1.0f : -1.0f;
		robot.EditRotation(jointIDs[i], swing * sign, Vec3{ 1.0f, 0.0f, 0.0f });
	}
	robot.EditPos(robotRootID, position);
	robot.EditRotation(robotRootID,
		glm::degrees(std::atan2(-direction.x, -direction.z)), Vec3{ 0.0f, 1.0f, 0.0f });

	stage->Update(dt);
	floorObject.Update(dt);
	obstaclesObject.Update(dt);
	robot.Update(dt);

	if (rotateCamera)
	{
		cameraRotateT += 60.0f * deltaTime * rotateCameraSign;
		Vec3 newPos = Vec3{
			glm::rotate(Mat4{ 1.0f }, glm::radians(cameraRotateT),
			Vec3{ 0.0f, 1.0f, 0.0f }) * Vec4{ cameraPos, 1.0f }};

		m_Cameras[0].PlaceAt(newPos);
		m_Cameras[0].FocusAt(Vec3{ 0.0f, cameraPos.y, 0.0f });

	}
}

void Application::Render(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix, int cameraIdx)
{
	CG::SetLightingParams(lightingShader, viewMatrix, projectionMatrix,
		Vec3{ -0.4f, -1.0f, -0.5f }, 0.45f, 0.25f, 16);

	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CCW);
	glCullFace(GL_FRONT);
	stage->Render(*lightingShader, viewMatrix);
	glCullFace(GL_BACK);
	floorObject.Render(*lightingShader, viewMatrix);
	obstaclesObject.Render(*lightingShader, viewMatrix);
	robot.Render(*lightingShader, viewMatrix);
	glDisable(GL_CULL_FACE);
}

void Application::OnKeyEvent(int key, int action, int mods)
{
	if (action != GLFW_PRESS && action != GLFW_REPEAT) return;
	bool shift = (mods & GLFW_MOD_SHIFT) != 0;
	switch (key)
	{
	case GLFW_KEY_W: direction = Vec3{ 0.0f, 0.0f, -1.0f }; break;
	case GLFW_KEY_A: direction = Vec3{ -1.0f, 0.0f, 0.0f }; break;
	case GLFW_KEY_S: direction = Vec3{ 0.0f, 0.0f, 1.0f }; break;
	case GLFW_KEY_D: direction = Vec3{ 1.0f, 0.0f, 0.0f }; break;
	case GLFW_KEY_O: if (action == GLFW_PRESS) openFront = !openFront; break;
	case GLFW_KEY_J:
		if (action == GLFW_PRESS && grounded)
		{
			grounded = false;
			verticalSpeed = 5.8f;
		}
		break;
	case GLFW_KEY_I:
		if (action == GLFW_PRESS)
		{
			Reset(m_Cameras[0]);
			m_Cameras[0].PlaceAt(cameraHome);
			m_Cameras[0].FocusAt(cameraTarget);
		}
		break;
	case GLFW_KEY_EQUAL:
	case GLFW_KEY_KP_ADD:
		walkSpeed = std::min(3.6f, walkSpeed + 0.2f);
		break;
	case GLFW_KEY_MINUS:
	case GLFW_KEY_KP_SUBTRACT:
		walkSpeed = std::max(0.35f, walkSpeed - 0.2f);
		break;
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
	case 'z':
		m_Cameras[0].Move(CameraDir::Up, deltaTime);
		break;
	case 'Z':
		m_Cameras[0].Move(CameraDir::Down, deltaTime);
		break;
	case 'x':
		m_Cameras[0].Move(CameraDir::Left, deltaTime);
		break;
	case 'X':
		m_Cameras[0].Move(CameraDir::Right, deltaTime);
		break;
	case 'y':
		if (rotateCamera && rotateCameraSign == 1.0f)
		{
			rotateCameraSign = -rotateCameraSign; break;
		}
		rotateCamera = !rotateCamera;
		if (rotateCamera)
		{
			cameraPos = m_Cameras[0].QueryPos();
			cameraYaw = m_Cameras[0].QueryYaw();
			cameraPitch = m_Cameras[0].QueryPitch();

			rotateCameraSign = -1.0f;
		}
		else
		{
			m_Cameras[0].PlaceAt(cameraPos);
			m_Cameras[0].OrientAt(cameraYaw, cameraPitch);
			cameraRotateT = 0.0f;
		}
		break;
	case 'Y':
		if (rotateCamera && rotateCameraSign == -1.0f)
		{
			rotateCameraSign = -rotateCameraSign; break;
		}
		rotateCamera = !rotateCamera;
		if (rotateCamera)
		{
			cameraPos = m_Cameras[0].QueryPos();
			cameraYaw = m_Cameras[0].QueryYaw();
			cameraPitch = m_Cameras[0].QueryPitch();

			rotateCameraSign = 1.0f;
		}
		else
		{
			m_Cameras[0].PlaceAt(cameraPos);
			m_Cameras[0].OrientAt(cameraYaw, cameraPitch);
			cameraRotateT = 0.0f;
		}
		break;
	}
}