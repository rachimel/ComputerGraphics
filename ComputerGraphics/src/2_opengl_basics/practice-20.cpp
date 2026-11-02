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
struct Conveyor
{

};

struct RotateConveyor
{

};

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

	Object plane{};
	AnimationID planeRotation{};
	AnimationID planeScale{};
}

int main()
{
	Application app{ 1280, 960 };
	if (int code = app.Init("i hate computer graphics 7")) return code;
	app.Run();
}

void Application::OnInit()
{
	skyboxShader.emplace("shaders/skybox.vs", "shaders/skybox.fs");
	lightingShader.emplace("shaders/1_opengl_basics/projectionViewNormal.vs",
		"shaders/1_opengl_basics/directionalPhongLightingShader.fs");
	lightingShader->SetUniform("lightDir", glm::normalize(glm::vec3(0.0f, 1.0f, -1.0f)));
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
	plane.SetColor(0, Vec3{ 0.5f });
	plane.SetPos(0, Vec3{ 0.0f });
	plane.SetRotation(0, 0.0f, glm::vec3(0.0f, 1.0f, 0.0f));
	plane.SetScale(0, Vec3{ 100.0f, 1.0f, 100.0f });


	planeRotation = plane.animationHandler.SetRotateAnimation(
		360.0, glm::vec3(0.0f, 1.0f, 0.0f), PlaybackType::Default);
	plane.animationHandler.SetSpeed(planeRotation, 0.05f);
	plane.animationHandler.SetLoop(planeRotation, true);

	plane.BindAnimation(0, 1, planeRotation);

	planeScale = plane.animationHandler.SetScaleAnimation(
		glm::vec3(0.1f, 1.0f, 0.1f), PlaybackType::PingPong);
	plane.animationHandler.SetSpeed(planeScale, 0.05f);
	plane.animationHandler.SetLoop(planeScale, true);

	plane.BindAnimation(0, 2, planeScale);
}

void Application::Clear()
{
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Application::Update()
{
	plane.Update(deltaTime);
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
	lightingShader->SetUniform("projection", projectionMatrix);
	lightingShader->SetUniform("view", viewMatrix);

	lightingShader->SetUniform("ambient", 0.2f);
	lightingShader->SetUniform("specularStrength", 0.1f);
	lightingShader->SetUniform("shininess", 4);

	plane.Render(*lightingShader, viewMatrix);
	
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