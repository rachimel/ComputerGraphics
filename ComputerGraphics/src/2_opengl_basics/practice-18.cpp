#define CG_APPLICATION_CUSTOM_CALLBACK_KEY
#define CG_APPLICATION_CUSTOM_CALLBACK_CHAR

#include <CG.h>
#include <optional>

#include <glm/gtc/matrix_transform.hpp>
#include <Shader.h>

namespace
{
	std::optional<Model> sun{};
	std::vector<Model> orbit{};
	std::vector<glm::mat4> orbitTransforms(6);

	std::vector<Model> planets{};
	std::vector<glm::mat4> planetTransforms(6);

	glm::vec3 globalTranslation{};
	glm::vec3 sunPos{};
	glm::vec4 sunColor{};
	glm::vec4 sunlightColor{};
	int rimLightIntensity{2};

	std::optional<Shader> sunShader{};
	std::optional<Shader> blurShader{};
	std::optional<Shader> compositeShader{};
	std::optional<Shader> orbitShader{};
	std::optional<Shader> planetShader{};

	unsigned int HDRFBO;
	unsigned int HDRTexture[2];
	unsigned int HDRRBO;

	unsigned int bloomVAO, bloomVBO;
	unsigned int bloomFBO[2];
	unsigned int bloomTexture[2];

	bool applyOrtho{ false };
	bool renderWired{false};
	float orbitRadius{ 10.0 };
	float planetOrbitT{ 0.0f };
	float planetOrbitSpeed{ 30.0f };
	float spinSpeed{ 60.0f };
	float animationSpeed{ 5.0f };

	float radiusTransitionSign{ 0.0f };

	bool sunScale{ false };
	float sunScaleT{ 0.0f };
	
	float spinByZT{ 0.0f };
	bool isSpinZ{ false };
}

int main()
{
	Application app{ 800, 800 };
	int code = app.Init("i hate computer graphics 5");
	if (code) return code;
	app.EnableOpenGLFeatures(GL_DEPTH_TEST);
	app.EnableOpenGLFeatures(GL_CULL_FACE);
	//app.EnableOpenGLFeatures(GL_BLEND);
	app.CaptureMouse();
	app.Run();
}

void InitFBO(const glm::ivec2& screenSize)
{
	glm::ivec2 bloomSize = glm::max(screenSize / 4, glm::ivec2(1));
	float quadVertices[] = {
		-1.0f,  1.0f,  0.0f, 1.0f,
		-1.0f, -1.0f,  0.0f, 0.0f,
		 1.0f,  -1.0f, 1.0f, 0.0f,

		-1.0f,  1.0f, 0.0f, 1.0f,
		 1.0f, -1.0f, 1.0f, 0.0f,
		 1.0f,  1.0f, 1.0f, 1.0f,
	};

	glGenVertexArrays(1, &bloomVAO);
	glGenBuffers(1, &bloomVBO);
	glBindVertexArray(bloomVAO);
	glBindBuffer(GL_ARRAY_BUFFER, bloomVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (const void*)0);

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (const void*)(sizeof(float) * 2));

	glBindVertexArray(0);

	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glGenFramebuffers(1, &HDRFBO);
	glGenFramebuffers(2, bloomFBO);
	glGenRenderbuffers(1, &HDRRBO);
	glBindRenderbuffer(GL_RENDERBUFFER, HDRRBO);
	glRenderbufferStorage(
		GL_RENDERBUFFER,
		GL_DEPTH_COMPONENT24,
		screenSize.x,
		screenSize.y
	);

	glGenTextures(2, HDRTexture);
	glBindTexture(GL_TEXTURE_2D, HDRTexture[0]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, screenSize.x, screenSize.y, 0, GL_RGBA, GL_FLOAT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glBindTexture(GL_TEXTURE_2D, HDRTexture[1]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, screenSize.x, screenSize.y, 0, GL_RGBA, GL_FLOAT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// connect target textures to framebuffers.
	glBindFramebuffer(GL_FRAMEBUFFER, HDRFBO);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, HDRRBO);
	for (int i = 0; i < 2; ++i)
	{
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, HDRTexture[i], 0);
	}

	// connect color results to HDR Framebuffer.
	GLenum attachments[] = {
		GL_COLOR_ATTACHMENT0,
		GL_COLOR_ATTACHMENT1
	};

	glDrawBuffers(2, attachments);

	// Generate Textures for bloom.
	glGenTextures(2, bloomTexture);
	glBindTexture(GL_TEXTURE_2D, bloomTexture[0]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, bloomSize.x, bloomSize.y, 0, GL_RGBA, GL_FLOAT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glBindTexture(GL_TEXTURE_2D, bloomTexture[1]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, bloomSize.x, bloomSize.y, 0, GL_RGBA, GL_FLOAT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	for (int i = 0; i < 2; ++i)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, bloomFBO[i]);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bloomTexture[i], 0);
		glDrawBuffer(GL_COLOR_ATTACHMENT0);
	}

	glBindFramebuffer(GL_FRAMEBUFFER, HDRFBO);
	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	std::println("HDR FBO status : {:#x}", static_cast<unsigned>(status));

	glBindFramebuffer(GL_FRAMEBUFFER, bloomFBO[0]);
	status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	std::println("bloom FBO #0 status : {:#x}", static_cast<unsigned>(status));

	glBindFramebuffer(GL_FRAMEBUFFER, bloomFBO[1]);
	status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	std::println("bloom FBO #1 status : {:#x}", static_cast<unsigned>(status));
}

void Application::OnInit()
{
	InitFBO(glm::ivec2(static_cast<int>(m_ScreenSize.x), static_cast<int>(m_ScreenSize.y)));
	CreateCamera(glm::vec3(0.0f, 3.0f, -10.0f));
	m_Camera->FocusAt(glm::vec3(0.0f));
	sun.emplace("data\\1_opengl_basics\\sphere.obj");
	// Sun Color
	sunColor = glm::vec4(1.0f, 0.82f, 0.48f, 1.0f);
	sunlightColor = glm::vec4(1.0f, 0.2f, 0.03f, 1.0f);

	sunShader.emplace("shaders\\1_opengl_basics\\projectionViewNormal.vs", "shaders\\1_opengl_basics\\sunColor.fs");
	blurShader.emplace("shaders\\1_opengl_basics\\quadShader.vs", "shaders\\1_opengl_basics\\gaussianBlur.fs");
	compositeShader.emplace("shaders\\1_opengl_basics\\quadShader.vs", "shaders\\1_opengl_basics\\HDRBlurCompositeShader.fs");
	orbitShader.emplace("shaders\\1_opengl_basics\\projectionView.vs", "shaders\\1_opengl_basics\\uniformColor.fs");
	planetShader.emplace("shaders\\1_opengl_basics\\projectionViewNormal.vs", "shaders\\1_opengl_basics\\singlePointLightShaderForHDRPass.fs");
	for (int i = 0; i < 6; ++i)
		orbit.push_back(std::move(Model{ "data\\1_opengl_basics\\circle.obj" }));

	for (int i = 0; i < 6; ++i)
		planets.push_back(std::move(Model{"data\\1_opengl_basics\\sphere.obj"}));
	glCullFace(GL_BACK);
}

void Application::Update()
{
	// 
	planetOrbitT += planetOrbitSpeed * deltaTime;

	if (radiusTransitionSign != 0.0f)
	{
		orbitRadius = glm::clamp(orbitRadius + radiusTransitionSign * animationSpeed * deltaTime, 5.0f, 7.5f);
		if (orbitRadius <= 5.0f || orbitRadius >= 7.5f)
			radiusTransitionSign = 0.0f;
	}
	
	if (sunScale)
	{
		sunScaleT += animationSpeed * deltaTime;
	}
	if (isSpinZ)
	{
		spinByZT += spinSpeed * deltaTime;
	}
	// Transforms
	std::fill(orbitTransforms.begin(), orbitTransforms.end(), glm::mat4(1.0f));
	std::fill(planetTransforms.begin(), planetTransforms.end(), glm::mat4(1.0f));

	// apply transforms
	for (int i = 0; i < 6; ++i)
	{
		orbitTransforms[i] = glm::translate(orbitTransforms[i], sunPos);
		planetTransforms[i] = glm::translate(planetTransforms[i], sunPos);
	}

	// update orbit size
	orbitTransforms[1] = glm::rotate(orbitTransforms[1], glm::radians(45.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	orbitTransforms[2] = glm::rotate(orbitTransforms[2], glm::radians(-45.0f), glm::vec3(0.0f, 0.0f, 1.0f));

	// scale
	for (int i = 0; i < 3; ++i)
		orbitTransforms[i] = glm::scale(orbitTransforms[i], glm::vec3(orbitRadius * 0.5f));

	// first planet
	planetTransforms[0] = glm::rotate(planetTransforms[0], glm::radians(planetOrbitT), glm::vec3(0.0f, 1.0f, 0.0f));
	planetTransforms[0] = glm::translate(planetTransforms[0], glm::vec3(0.0f, 0.0f, 0.5f) * orbitRadius);
	//planetTransforms[0] = glm::scale(planetTransforms[0], glm::vec3(0.8f));

	planetTransforms[1] = glm::rotate(planetTransforms[1], glm::radians(45.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	planetTransforms[1] = glm::rotate(planetTransforms[1], glm::radians(planetOrbitT + 90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	planetTransforms[1] = glm::translate(planetTransforms[1], glm::vec3(0.0f, 0.0f, 0.5f) * orbitRadius);
	//planetTransforms[1] = glm::scale(planetTransforms[1], glm::vec3(0.8f));

	planetTransforms[2] = glm::rotate(planetTransforms[2], glm::radians(-45.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	planetTransforms[2] = glm::rotate(planetTransforms[2], glm::radians(planetOrbitT - 90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	planetTransforms[2] = glm::translate(planetTransforms[2], glm::vec3(0.0f, 0.0f, 0.5f) * orbitRadius);
	//planetTransforms[2] = glm::scale(planetTransforms[2], glm::vec3(0.8f));

	// update satellite orbits
	orbitTransforms[3] = glm::translate(orbitTransforms[3], glm::vec3(planetTransforms[0] * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)));
	orbitTransforms[3] = glm::rotate(orbitTransforms[3], glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)); 
	orbitTransforms[3] = glm::scale(orbitTransforms[3], glm::vec3(orbitRadius * 0.25f));

	orbitTransforms[4] = glm::translate(orbitTransforms[4], glm::vec3(planetTransforms[1] * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)));
	orbitTransforms[4] = glm::rotate(orbitTransforms[4], glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	orbitTransforms[4] = glm::scale(orbitTransforms[4], glm::vec3(orbitRadius * 0.25f));

	orbitTransforms[5] = glm::translate(orbitTransforms[5], glm::vec3(planetTransforms[2] * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)));
	orbitTransforms[5] = glm::rotate(orbitTransforms[5], glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	orbitTransforms[5] = glm::scale(orbitTransforms[5], glm::vec3(orbitRadius * 0.25f));

	// update satellites
	planetTransforms[3] = glm::translate(planetTransforms[3], glm::vec3(planetTransforms[0] * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)));
	planetTransforms[3] = glm::rotate(planetTransforms[3], glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	planetTransforms[3] = glm::rotate(planetTransforms[3], glm::radians(planetOrbitT), glm::vec3(0.0f, 1.0f, 0.0f));
	planetTransforms[3] = glm::translate(planetTransforms[3], glm::vec3(0.0f, 0.0f, 0.25f) * orbitRadius);
	planetTransforms[3] = glm::rotate(planetTransforms[3], glm::radians(spinByZT), glm::vec3(0.0f, 0.0f, 1.0f));
	planetTransforms[3] = glm::scale(planetTransforms[3], glm::vec3(0.25f));


	planetTransforms[4] = glm::translate(planetTransforms[4], glm::vec3(planetTransforms[1] * glm::vec4(0.0f, 0.0f, 0.0, 1.0f)));
	//planetTransforms[4] = glm::rotate(planetTransforms[4], glm::radians(45.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	planetTransforms[4] = glm::rotate(planetTransforms[4], glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	planetTransforms[4] = glm::rotate(planetTransforms[4], glm::radians(planetOrbitT), glm::vec3(0.0f, 1.0f, 0.0f));
	planetTransforms[4] = glm::translate(planetTransforms[4], glm::vec3(0.0f, 0.0f, 0.25f) * orbitRadius);
	planetTransforms[4] = glm::rotate(planetTransforms[4], glm::radians(spinByZT), glm::vec3(0.0f, 0.0f, 1.0f));
	planetTransforms[4] = glm::scale(planetTransforms[4], glm::vec3(0.25f));

	planetTransforms[5] = glm::translate(planetTransforms[5], glm::vec3(planetTransforms[2] * glm::vec4(0.0f, 0.0f, 0.0, 1.0f)));
	//planetTransforms[5] = glm::rotate(planetTransforms[5], glm::radians(-45.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	planetTransforms[5] = glm::rotate(planetTransforms[5], glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	planetTransforms[5] = glm::rotate(planetTransforms[5], glm::radians(planetOrbitT), glm::vec3(0.0f, 1.0f, 0.0f));
	planetTransforms[5] = glm::translate(planetTransforms[5], glm::vec3(0.0f, 0.0f, 0.25f) * orbitRadius);
	planetTransforms[5] = glm::rotate(planetTransforms[5], glm::radians(spinByZT), glm::vec3(0.0f, 0.0f, 1.0f));
	planetTransforms[5] = glm::scale(planetTransforms[5], glm::vec3(0.25f));

}

void Application::Render()
{
	if (applyOrtho)
	{
		float aspect = m_ScreenSize.x / m_ScreenSize.y;
		float halfHeight = 8.0f;
		projection = glm::ortho(-halfHeight * aspect, halfHeight * aspect, -halfHeight, halfHeight, m_Near, m_Far);
	}
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glBindFramebuffer(GL_FRAMEBUFFER, HDRFBO);
	glm::ivec2 screenSize = glm::ivec2(m_ScreenSize);
	glm::ivec2 bloomSize = glm::max(screenSize / 4, glm::ivec2(1));
	glViewport(0, 0, screenSize.x, screenSize.y);
	glEnable(GL_DEPTH_TEST);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// pass 1 : Render Scene
	glm::mat4 model = glm::mat4(1.0f);
	glm::mat4 view = m_Camera->ViewMatrix();

	sunShader->Bind();
	sunShader->SetUniform("projection", projection);
	sunShader->SetUniform("view", view);
	model = glm::translate(model, sunPos);
	model = glm::scale(model, glm::vec3(0.5f * glm::sin(sunScaleT) + 1.0f));
	sunShader->SetUniform("model", model);

	model = glm::mat4(1.0f);
	glm::vec3 sunPosView = view * model * glm::vec4(sunPos, 1.0f);
	glm::mat3 normalMatrix = glm::inverse(glm::transpose(view * model));
	sunShader->SetUniform("normalMatrix", normalMatrix);
	sunShader->SetUniform("sunColor", sunColor);
	sunShader->SetUniform("sunlightColor", sunlightColor);
	sunShader->SetUniform("intensity", rimLightIntensity);

	if (renderWired) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	auto& sunMeshes = sun->Meshes();
	for (auto& sunMesh : sunMeshes)
	{
		sunMesh.Bind();
		glDrawElements(GL_TRIANGLES, sunMesh.m_Indices.size(), GL_UNSIGNED_INT, (const void*)0);
	}
	if (renderWired) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	// Render Objects Here
	orbitShader->Bind();
	orbitShader->SetUniform("projection", projection);
	orbitShader->SetUniform("view", view);

	orbitShader->SetUniform("a_Color", glm::vec3(0.5f, 0.5f, 0.5f));
	int orbitSize = orbit.size();
	glLineWidth(2.0f);
	for (int i = 0; i < orbitSize; ++i)
	{
		orbitShader->SetUniform("model", orbitTransforms[i]);
		auto& orbitMeshes = orbit[i].Meshes();
		for (auto& orbitMesh : orbitMeshes)
		{
			orbitMesh.Bind();
			glDrawElements(GL_LINES, orbitMesh.m_Indices.size(), GL_UNSIGNED_INT, (const void*)0);
		}
	}
	
	planetShader->Bind();
	planetShader->SetUniform("projection", projection);
	planetShader->SetUniform("view", view);

	planetShader->SetUniform("lightPos", sunPosView);
	planetShader->SetUniform("lightColor", sunColor);
	planetShader->SetUniform("objectColor", glm::vec3(0.0f, 0.5f, 0.0f));

	planetShader->SetUniform("ambient", 0.2f);
	planetShader->SetUniform("shininess", 1);

	if (renderWired) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	for (int i = 0; i < 3; ++i)
	{
		planetShader->SetUniform("model", planetTransforms[i]);
		glm::mat3 normalMatrix = glm::inverse(glm::transpose(view * planetTransforms[i]));
		planetShader->SetUniform("normalMatrix", normalMatrix);

		auto& meshes = planets[i].Meshes();
		for (auto& mesh : meshes)
		{
			mesh.Bind();
			glDrawElements(GL_TRIANGLES, mesh.m_Indices.size(), GL_UNSIGNED_INT, (const void*)0);
		}
	}

	planetShader->SetUniform("objectColor", glm::vec3(0.5f, 0.0f, 0.0f));
	for (int i = 0; i < 3; ++i)
	{
		planetShader->SetUniform("model", planetTransforms[i + 3]);
		glm::mat3 normalMatrix = glm::inverse(glm::transpose(view * planetTransforms[i + 3]));
		planetShader->SetUniform("normalMatrix", normalMatrix);

		auto& meshes = planets[i + 3].Meshes();
		for (auto& mesh : meshes)
		{
			mesh.Bind();
			glDrawElements(GL_TRIANGLES, mesh.m_Indices.size(), GL_UNSIGNED_INT, (const void*)0);
		}
	}
	
	if (renderWired) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	// pass 2 : Ping-Pong Bloom
	glViewport(0, 0, bloomSize.x, bloomSize.y);
	glDisable(GL_DEPTH_TEST);
	unsigned int inputTexture = HDRTexture[1];
	blurShader->Bind();
	blurShader->SetUniform("screenTexture", 0);

	glBindVertexArray(bloomVAO);
	for (int pass = 0; pass < 5; ++pass)
	{
		for (int k = 0; k < 2; ++k)
		{
			glBindFramebuffer(GL_FRAMEBUFFER, bloomFBO[k]);
			blurShader->SetUniform("blurHorizontal", k == 0);

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, inputTexture);
			//
			glDrawArrays(GL_TRIANGLES, 0, 6);
			inputTexture = bloomTexture[k];
		}
	}

	// pass 3 : Composite Result
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDisable(GL_DEPTH_TEST);
	glViewport(0, 0, screenSize.x, screenSize.y);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	compositeShader->Bind();
	compositeShader->SetUniform("hdrTexture", 0);
	compositeShader->SetUniform("bloomTexture", 1);
	compositeShader->SetUniform("exposure", 2.0f);
	compositeShader->SetUniform("bloomStrength", 0.5f);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, HDRTexture[0]);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, bloomTexture[1]);

	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);
}

void Application::OnKeyEvent(GLFWwindow* window, int key, int action, int mods)
{
	if (action == GLFW_PRESS)
	{
		switch (key)
		{
		case GLFW_KEY_1:
			if (radiusTransitionSign != 0.0f) break;
			radiusTransitionSign = (orbitRadius <= 5.0f) ? 1.0f : -1.0f;
			orbitRadius = glm::clamp(orbitRadius + radiusTransitionSign * animationSpeed * deltaTime, 5.0f, 7.5f);
			break;
		case GLFW_KEY_2:
			sunScale = !sunScale;
			sunScaleT = 0.0f;
			break;
		case GLFW_KEY_3:
			isSpinZ = !isSpinZ;
			spinByZT = 0.0f;
			break;
		case GLFW_KEY_Q:
			glfwSetWindowShouldClose(window, true);
			break;
		case GLFW_KEY_ESCAPE:
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			isMouseOutOfFocus = true;
			break;
		}
	}
}

void Application::OnCharEvent(GLFWwindow* window, unsigned int codepoint)
{
	switch (codepoint)
	{
	case 'p':
		applyOrtho = true;
		break;
	case 'P':
		applyOrtho = false;
		break;
	case 'm':
		renderWired = false;
		break;
	case 'M':
		renderWired = true;
		break;
	case 'w':
		sunPos.y += 1.0f;
		break;
	case 'a':
		sunPos.x += 1.0f;
		break;
	case 's':
		sunPos.y -= 1.0f;
		break;
	case 'd':
		sunPos.x -= 1.0f;
		break;
	case '+':
		sunPos.z += 1.0f;
		break;
	case '-':
		sunPos.z -= 1.0f;
		break;
	}
}