#define CG_APPLICATION_CUSTOM_CALLBACK_KEY
#define CG_APPLICATION_CUSTOM_CALLBACK_CHAR
#include <CG.h>

#include <glm/gtc/matrix_transform.hpp>

#include <random>


namespace
{
	std::optional<Model> plane;
	// Init Tank Model
	// 0 : Core
	// 1 : Left Lower
	// 2 : Right Lower
	// 3 : Left Upper
	// 4 : Right Upper
	// 5 : Left Turret
	// 6 : Right Turret
	// 7 : Left Pole
	// 8 : Right Pole

	std::random_device rd;
	std::default_random_engine dre{ rd() };

	std::vector<Model> tankModel;
	std::vector<glm::vec3> tankColors;

	glm::vec3 tankPos{ 0.0f, 3.0f, 0.0f };
	std::optional<Shader> lightingShader{};
	glm::mat4 planeTransform{ glm::scale(glm::mat4(1.0f), glm::vec3(50.0f, 1.0f, 50.0f))};
	std::vector<glm::mat4> tankTransform;

	// light configs
	glm::vec3 lightDirection{ glm::vec3(0.0f, -1.0f, 1.0f) };
	glm::vec3 lightColor{ glm::vec3(1.0f, 1.0f, 1.0f) };

	constexpr float rotateSpeed{ 60.0f };
	constexpr float animationSpeed{ 5.0f };

	// Tank Status
	float xzRotation{};

	bool rotateCore{};
	float rotateCoreT{};

	bool scaleLower{};
	float scaleLowerT{};

	float rotateTurretT{};
	float rotateTurretSign{};

	float rotatePoleSign{};
	float rotatePoleT{};

	float swapUpperT{};
	float swapUpperSign{};

	float originalCameraYaw{};
	float cameraYawT{};
	float cameraYawSign{};

	glm::vec3 originalCameraPos{};
	float cameraSpinT{};
	float cameraSpinSign{};
}
int main()
{
	Application app{ 1280,960 };
	if (int code = app.Init("i hate computer graphics 6")) return code;
	app.CaptureMouse();
	glEnable(GL_DEPTH_TEST);
	app.Run();


}

void Reset(std::vector<Camera>& cameras)
{

	tankPos = glm::vec3(0.0f, 3.0f, 0.0f);
	cameras[0].FocusAt(tankPos);

	cameras[1].OrientAt(90.0f,-89.9f);

	cameras[2].FocusAt(tankPos);

	rotateCore = false;
	rotateCoreT = 0.0f;
	scaleLower = false;
	scaleLowerT = 0.0f;

	rotateTurretSign = 0.0f;
	rotateTurretT = 0.0f;
	rotatePoleSign = 0.0f;
	rotatePoleT = 0.0f;
	swapUpperSign = 0.0f;
	swapUpperT = 0.0f;
	cameraSpinSign = 0.0f;
	cameraSpinT = 0.0f;
	cameraYawSign = 0.0f;
	cameraYawT = 0.0f;
	// init Colors
	tankColors = {
		glm::vec3(0.5f),
		glm::vec3(0.1f),
		glm::vec3(0.1f),
		glm::vec3(0.4f, 0.6f, 0.4f),
		glm::vec3(0.4f, 0.6f, 0.4f),
		glm::vec3(0.7f, 0.8f, 0.6f),
		glm::vec3(0.7f, 0.8f, 0.6f),
		glm::vec3(0.8f, 0.8f, 0.1f),
		glm::vec3(0.8f, 0.8f, 0.1f),
	};

}
void Application::OnInit()
{
	AppendCamera(glm::vec3(0.0f, 10.0f, -25.0f));
	AppendCamera(glm::vec3(0.0f, 100.0f, 0.0f));
	AppendCamera(glm::vec3(0.0f, 3.0f, -25.0f));


	plane.emplace("data\\1_opengl_basics\\plane.obj");
	lightingShader.emplace("shaders/1_opengl_basics/projectionViewNormal.vs", "shaders/1_opengl_basics/directionalPhongLightingShader.fs");

	lightingShader->Bind();
	lightingShader->SetUniform("lightColor", lightColor);

	// Init Tank Model
	for (int i = 0; i < 9; ++i)
	{
		tankModel.emplace_back("data/1_opengl_basics/cubePrimitive.obj");
		tankTransform.emplace_back(1.0f);
	}

	Reset(m_Cameras);
}

void Application::Update()
{
	if (scaleLower)
	{
		scaleLowerT += rotateSpeed * deltaTime;
	}
	if (rotateCore)
	{
		rotateCoreT += rotateSpeed * deltaTime;
	}
	if (swapUpperSign != 0.0f)
	{
		swapUpperT = glm::clamp(swapUpperT + swapUpperSign * animationSpeed * deltaTime, 0.0f, 1.0f);
		if (swapUpperT <= 0.0f || swapUpperT >= 1.0f)
		{
			swapUpperSign = 0.0f;
		}
	}

	if (rotateTurretSign != 0.0f)
	{
		rotateTurretT = glm::clamp(rotateTurretT + rotateTurretSign * rotateSpeed * deltaTime, 0.0f, 90.0f);
		if (rotateTurretT <= 0.0f || rotateTurretT >= 90.0f)
		{
			rotateTurretSign = -rotateTurretSign;
		}
	}

	if (rotatePoleSign != 0.0f)
	{
		rotatePoleT = glm::clamp(rotatePoleT + rotatePoleSign * rotateSpeed * deltaTime, -90.0f, 90.0f);
		if (rotatePoleT <= -90.0f || rotatePoleT >= 90.0f)
		{
			rotatePoleSign = -rotatePoleSign;
		}
	}

	if (cameraYawSign != 0.0f)
	{
		cameraYawT += cameraYawSign * rotateSpeed * deltaTime;
		m_Cameras[0].OrientAt(cameraYawT);
	}

	if (cameraSpinSign != 0.0f)
	{
		cameraSpinT += cameraSpinSign * rotateSpeed * deltaTime;
		auto rotationMatrx = glm::rotate(glm::mat4(1.0f), glm::radians(cameraSpinT), glm::vec3(0.0f, 1.0f, 0.0f));
		m_Cameras[0].PlaceAt(glm::vec3(rotationMatrx * glm::vec4(originalCameraPos, 1.0f)));
		m_Cameras[0].FocusAt(glm::vec3(0.0f));
	}
	std::fill(tankTransform.begin(), tankTransform.end(), glm::mat4(1.0f));
	// core transform
	auto coreRotation = glm::rotate(tankTransform[0], glm::radians(rotateCoreT), glm::vec3(0.0f, 1.0f, 0.0f));
	tankTransform[0] = coreRotation * tankTransform[0];
	tankTransform[0] = glm::translate(tankTransform[0], tankPos);

	// Lower
	{
		tankTransform[1] = tankTransform[0] * tankTransform[1];
		tankTransform[1] = glm::translate(tankTransform[1], glm::vec3(4.0f, (0.75f + 0.25f * glm::cos(glm::radians(scaleLowerT))) * -2.0f, 0.0f));

		tankTransform[2] = tankTransform[0] * tankTransform[2];
		tankTransform[2] = glm::translate(tankTransform[2], glm::vec3(-4.0f, (0.75f + 0.25f * glm::cos(glm::radians(scaleLowerT))) *-2.0f, 0.0f));
	}

	// Upper
	tankTransform[3] = tankTransform[0] * tankTransform[3];
	tankTransform[3] = glm::translate(tankTransform[3], glm::vec3(-4.0f, 2.0f, 0.0f));

	tankTransform[4] = tankTransform[0] * tankTransform[4];
	tankTransform[4] = glm::translate(tankTransform[4], glm::vec3(4.0f, 2.0f, 0.0f));

	auto oldLeftUpperTransform = tankTransform[3];
	auto oldRightUpperTransform = tankTransform[4];

	tankTransform[3] = swapUpperT * oldLeftUpperTransform + (1.0f - swapUpperT) * oldRightUpperTransform;
	tankTransform[4] = swapUpperT * oldRightUpperTransform + (1.0f - swapUpperT) * oldLeftUpperTransform; 
	//Turret
	{
		tankTransform[5] = tankTransform[3] * tankTransform[5];
		tankTransform[5] = glm::rotate(tankTransform[5], glm::radians(-rotateTurretT), glm::vec3(0.0f, 1.0f, 0.0f));
		tankTransform[5] = glm::translate(tankTransform[5], glm::vec3(0.0f, 0.0f, -5.0f));

		tankTransform[6] = tankTransform[4] * tankTransform[6];
		tankTransform[6] = glm::rotate(tankTransform[6], glm::radians(rotateTurretT), glm::vec3(0.0f, 1.0f, 0.0f));
		tankTransform[6] = glm::translate(tankTransform[6], glm::vec3(0.0f, 0.0f, -5.0f));
	}

	// Pole
	{
		tankTransform[7] = tankTransform[3] * tankTransform[7];
		tankTransform[7] = glm::rotate(tankTransform[7], glm::radians(-rotatePoleT), glm::vec3(1.0f, 0.0f, 0.0f));
		tankTransform[7] = glm::translate(tankTransform[7],glm::vec3(0.0f, 2.0f, 0.0f));

		tankTransform[8] = tankTransform[4] * tankTransform[8];
		tankTransform[8] = glm::rotate(tankTransform[8], glm::radians(rotatePoleT), glm::vec3(1.0f, 0.0f, 0.0f));
		tankTransform[8] = glm::translate(tankTransform[8],glm::vec3(0.0f, 2.0f, 0.0f));
	}

	// Apply scales
	tankTransform[0] = glm::scale(tankTransform[0], glm::vec3(5.0f, 1.0f, 1.0f));
	tankTransform[1] = glm::scale(tankTransform[1], glm::vec3(2.0f, (0.5f + 0.5f * glm::cos(glm::radians(scaleLowerT))) * 1.0f, 4.0f));
	tankTransform[2] = glm::scale(tankTransform[2], glm::vec3(2.0f, (0.5f + 0.5f * glm::cos(glm::radians(scaleLowerT))) * 1.0f, 4.0f));
	tankTransform[3] = glm::scale(tankTransform[3], glm::vec3(1.5f, 1.0f, 2.0f));
	tankTransform[4] = glm::scale(tankTransform[4], glm::vec3(1.5f, 1.0f, 2.0f));
	tankTransform[5] = glm::scale(tankTransform[5], glm::vec3(0.5f, 0.5f, 3.0f));
	tankTransform[6] = glm::scale(tankTransform[6], glm::vec3(0.5f, 0.5f, 3.0f));
	tankTransform[7] = glm::scale(tankTransform[7], glm::vec3(0.5f, 2.0f, 0.5f));
	tankTransform[8] = glm::scale(tankTransform[8], glm::vec3(0.5f, 2.0f, 0.5f));
}

void Application::Clear()
{
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Application::Render(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix, int cameraIdx)
{
	lightingShader->Bind();
	// Camera
	lightingShader->SetUniform("view", viewMatrix);
	lightingShader->SetUniform("projection", projectionMatrix);

	lightingShader->SetUniform("model", planeTransform);
	lightingShader->SetUniform("normalMatrix", glm::mat3(glm::inverse(glm::transpose(viewMatrix * planeTransform))));
	lightingShader->SetUniform("objectColor", glm::vec3(0.8f, 0.8f, 0.8f));
	lightingShader->SetUniform("ambient", 0.2f);
	lightingShader->SetUniform("specularStrength", 0.0f);
	lightingShader->SetUniform("shininess", 1);
	glm::vec3 lightDirView{ glm::normalize(glm::mat3(viewMatrix) * (lightDirection))};
	lightingShader->SetUniform("lightDir", lightDirView);
	auto& planeMeshes = plane->Meshes();
	for (auto& mesh : planeMeshes)
	{
		mesh.Bind();
		glDrawElements(GL_TRIANGLES, static_cast<int>(mesh.m_Indices.size()), GL_UNSIGNED_INT, (const void*)0);
	}

	int tankTransformSize{ static_cast<int>(tankTransform.size()) };
	for (int i = 0; i < 9; ++i)
	{
		lightingShader->SetUniform("model", tankTransform[i]);
		lightingShader->SetUniform("objectColor", tankColors[i]);
		lightingShader->SetUniform("normalMatrix", glm::mat3(glm::inverse(glm::transpose(viewMatrix * tankTransform[i]))));
		auto& meshes = tankModel[i].Meshes();
		for (auto& mesh : meshes)
		{
			mesh.Bind();
			glDrawElements(GL_TRIANGLES, static_cast<int>(mesh.m_Indices.size()), GL_UNSIGNED_INT, (const void*)0);
		}
	}
}

void Application::OnKeyEvent(GLFWwindow* window, int key, int action, int mods)
{
	if (action == GLFW_PRESS)
	{
		switch (key)
		{
		case GLFW_KEY_LEFT:
			tankPos.x += 1.0f;
			break;
		case GLFW_KEY_RIGHT:
			tankPos.x -= 1.0f;
			break;
		case GLFW_KEY_UP:
			tankPos.z += 1.0f;
			break;
		case GLFW_KEY_DOWN:
			tankPos.z -= 1.0f;
			break;
		case GLFW_KEY_Q:
			glfwSetWindowShouldClose(m_Window, true);
			break;
		case GLFW_KEY_ESCAPE:
			glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			mouseActiveCamera = -1;
			break;
		case GLFW_KEY_T:
			rotateCore = !rotateCore;
			rotateCoreT = 0.0f;
			break;
		case GLFW_KEY_D:
			scaleLower = !scaleLower;
			scaleLowerT = 0.0f;
			break;
		case GLFW_KEY_L:
			if (swapUpperT > 0.0f && swapUpperT < 1.0f) break;
			swapUpperSign = (swapUpperT <= 0.0f) ? 1.0f : -1.0f;
			break;
		case GLFW_KEY_G:
			rotateTurretSign = (rotateTurretSign == 0.0f) ? 1.0f : 0.0f;
			rotateTurretT = 0.0f;
			break;
		case GLFW_KEY_P:
			rotatePoleSign = (rotatePoleSign == 0.0f) ? 1.0f : 0.0f;
			rotatePoleT = 0.0f;
			break;
		case GLFW_KEY_O:
		{
			std::uniform_real_distribution<float> colorRange{ 0.1f, 0.95f };
			for (int i = 0; i < 9; ++i)
				tankColors[i] = glm::vec3(colorRange(dre), colorRange(dre), colorRange(dre));
			break;
		}
		case GLFW_KEY_C:
			Reset(m_Cameras);
			break;
		}
	}
}

void Application::OnCharEvent(GLFWwindow* window, unsigned int codepoint)
{
	switch (codepoint)
	{
	case 'x':
		m_Cameras[0].Move(glm::vec3(1.0f, 0.0f, 0.0f));
		break;
	case 'X':
		m_Cameras[0].Move(glm::vec3(-1.0f, 0.0f, 0.0f));
		break;
	case 'z':
		m_Cameras[0].Move(glm::vec3(0.0f, 0.0f, 1.0f));
		break;
	case 'Z':
		m_Cameras[0].Move(glm::vec3(0.0f, 0.0f, -1.0f));
		break;
	case 'y':
		cameraYawSign = (cameraYawSign == -1.0f) ? 0.0f : -1.0f;
		if (cameraYawSign == -1.0f)
			originalCameraYaw = m_Cameras[0].QueryYaw();
		else
			m_Cameras[0].OrientAt(originalCameraYaw);
		cameraYawT = 0.0f;
		break;
	case 'Y':
		cameraYawSign = (cameraYawSign == 1.0f) ? 0.0f : 1.0f;
		if (cameraYawSign == 1.0f)
			originalCameraYaw = m_Cameras[0].QueryYaw();
		else
			m_Cameras[0].OrientAt(originalCameraYaw);
		cameraYawT = 0.0f;
		break;
	case 'r':
		cameraSpinSign = (cameraSpinSign == -1.0f) ? 0.0f : -1.0f;
		if (cameraSpinSign == -1.0f)
			originalCameraPos = m_Cameras[0].QueryPos();
		else
		{
			m_Cameras[0].PlaceAt(originalCameraPos);
		}
		cameraSpinT = 0.0f;
		break;
	case 'R':
		cameraSpinSign = (cameraSpinSign == 1.0f) ? 0.0f : 1.0f;
		if (cameraSpinSign == 1.0f)
			originalCameraPos = m_Cameras[0].QueryPos();
		else
		{
			m_Cameras[0].PlaceAt(originalCameraPos);
		}
		cameraSpinT = 0.0f;
		break;
	}
}