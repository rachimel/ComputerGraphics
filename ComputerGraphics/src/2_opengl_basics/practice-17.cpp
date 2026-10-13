#define CG_APPLICATION_CUSTOM_CALLBACK_KEY
#define CG_APPLICATION_CUSTOM_CALLBACK_CHAR
#include <CG.h>

#include <glm/gtc/matrix_transform.hpp>
#include <Shader.h>
#include <optional>

constexpr float angularSpeed{ 60.0f };
constexpr float animationSpeed{ 5.0f };

namespace
{
	float tAnimationT{ 0.0f };
	float tAnimationSign{ 0.0f };

	float uAnimationT{ 0.0f };
	float uAnimationSign{ 0.0f };

	float vAnimationT{ 0.0f };
	float vAnimationSign{ 0.0f };
}

struct Object
{
	glm::vec3 pos{};
	glm::vec3 color{};
	
	float rotateXT{ 0.0f };
	float rotateXSign{ 0.0f };

	float rotateYT{ 0.0f };
	float rotateYSign{ 0.0f };

	float rotateYOriginT{ 0.0f };
	float rotateYOriginSign{ 0.0f };

	float scaleT{ 1.0f };
	float scaleSign{ 0.0f };
	float scaleAboutOriginT{ 1.0f };
	float scaleAboutOriginSign{ 0.0f };

	void Update(float dt)
	{
		if (rotateXSign != 0.0f)
			rotateXT +=  rotateXSign * angularSpeed * dt;
		if (rotateYSign != 0.0f)
			rotateYT += rotateYSign * angularSpeed * dt;
		if (rotateYOriginSign != 0.0f)
			rotateYOriginT += rotateYOriginSign * angularSpeed * dt;
		if (scaleSign != 0.0f)
			scaleT = glm::clamp(scaleT + scaleSign * animationSpeed * dt ,1.0f, 2.0f);
		if (scaleAboutOriginSign != 0.0f)
			scaleAboutOriginT = glm::clamp(scaleAboutOriginT + scaleAboutOriginSign * animationSpeed * dt, 1.0f, 2.0f);
	}

	glm::mat4 GetWorldTransform()
	{
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::scale(model, glm::vec3(scaleAboutOriginT));
		model = glm::rotate(model, glm::radians(rotateYOriginT), glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::translate(model, pos);
		model = glm::rotate(model, glm::radians(rotateXT), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(rotateYT), glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::scale(model, glm::vec3(scaleT));
		return model;
	}
};

namespace
{
	std::optional<Model> axes{};
	std::optional<Shader> shader{};
	std::optional<Shader> uniformColorShader{};
	std::optional<Model> objectAModel{};
	std::optional<Model> objectBModel{};

	Object objectA{};
	Object objectB{};

	enum ApplyMode
	{
		APPLY_A = 0b1,
		APPLY_B = 0b10,
		APPLY_BOTH = 0b11,
	};

	ApplyMode applyMode{ APPLY_A };
	bool swapModels{};
}

int main()
{
	Application app{ 800,800 };
	int code = app.Init("I hate computer graphics 4");
	if (code) return code;
	app.CaptureMouse();
	app.EnableOpenGLFeatures(GL_DEPTH_TEST);
	app.Run();
}

void Application::OnInit()
{
	shader.emplace("shaders\\1_opengl_basics\\projectionView.vs", "shaders\\1_opengl_basics\\vertexColor.fs");
	uniformColorShader.emplace("shaders\\1_opengl_basics\\projectionView.vs", "shaders\\1_opengl_basics\\uniformColor.fs");

	axes.emplace("data\\1_opengl_basics\\axes.obj");
	objectAModel.emplace("data\\1_opengl_basics\\sphere.obj");
	objectA.pos = glm::vec3(-3.0f, 0.0f, 0.0f);
	objectA.color = glm::vec3(0.0f, 0.0f, 1.0f);
	objectBModel.emplace("data\\1_opengl_basics\\cube.obj");
	objectB.pos = glm::vec3(3.0f, 0.0f, 0.0f);
	CreateCamera(glm::vec3(0.0f, 5.0f, 10.0f));
	m_Camera->FocusAt(glm::vec3(0.0f));
}

void Application::Update()
{
	if (vAnimationSign != 0.0f)
	{
		if (vAnimationSign > 0.0f)
		{
			objectA.scaleSign = 1.0f;
			objectB.scaleSign = -1.0f;
		}
		else
		{
			objectA.scaleSign = -1.0f;
			objectB.scaleSign = 1.0f;
		}
		vAnimationT = glm::clamp(vAnimationT + vAnimationSign * deltaTime, 0.0f, 1.0f);
		if (vAnimationT <= 0.0f || vAnimationT >= 1.0f) vAnimationSign = (vAnimationT <= 0.0f) ? 1.0f : -1.0f;
	}

	objectA.Update(deltaTime);
	objectB.Update(deltaTime);

	if (tAnimationSign != 0.0f)
	{
		tAnimationT = glm::clamp(tAnimationT + tAnimationSign * deltaTime, 0.0f, 1.0f);
		if (tAnimationT <= 0.0f || tAnimationT >= 1.0f) tAnimationSign = (tAnimationT <= 0.0f) ? 1.0f : -1.0f;
	}

	if (uAnimationSign != 0.0f)
	{
		uAnimationT = glm::clamp(uAnimationT + uAnimationSign * deltaTime, 0.0f, 1.0f);
		if (uAnimationT <= 0.0f || uAnimationT >= 1.0f) uAnimationSign = (uAnimationT <= 0.0f) ? 1.0f : -1.0f;
	}
}

void Application::Render()
{
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glm::mat4 model{ glm::mat4(1.0f) };;
	shader->Bind();
	shader->SetUniform("projection", projection);
	shader->SetUniform("view", m_Camera->ViewMatrix());
	shader->SetUniform("model", model);

	auto& axesMeshes = axes->Meshes();
	for (auto& axesMesh : axesMeshes)
	{
		axesMesh.Bind();
		glDrawElements(GL_LINES, axesMesh.m_Indices.size(), GL_UNSIGNED_INT, (const void*)0);
	}

	// 변환 정보 저장
	Object renderA = objectA;
	Object renderB = objectB;

	auto objectAModelMatrix = objectA.GetWorldTransform();
	auto objectBModelMatrix = objectB.GetWorldTransform();

	glm::vec3 objectAWorldPos = objectAModelMatrix[3];
	glm::vec3 objectBWorldPos = objectBModelMatrix[3];

	auto InterpolateThroughOrigin = [](const glm::vec3& from, const glm::vec3& to, float t) -> glm::vec3 {
		if (t < 0.5f)
			return glm::mix(from, glm::vec3(0.0f), t * 2.0f);
		return glm::mix(glm::vec3(0.0f), to, (t - 0.5f) * 2.0f);
		};
	
	glm::mat4 renderAInverse = glm::scale(glm::mat4(1.0f), glm::vec3(objectA.scaleAboutOriginT));
	renderAInverse = glm::inverse(glm::rotate(renderAInverse, glm::radians(renderA.rotateYOriginT), glm::vec3(0.0f, 1.0f, 0.0f)));
	glm::mat4 renderBInverse = glm::scale(glm::mat4(1.0f), glm::vec3(objectB.scaleAboutOriginT));
	renderBInverse = glm::inverse(glm::rotate(renderBInverse, glm::radians(renderB.rotateYOriginT), glm::vec3(0.0f, 1.0f, 0.0f)));

	if(tAnimationSign != 0.0f)
	{
		renderA.pos = renderAInverse * glm::vec4(InterpolateThroughOrigin(objectAWorldPos, objectBWorldPos, tAnimationT), 1.0f);
		renderB.pos = renderBInverse * glm::vec4(InterpolateThroughOrigin(objectBWorldPos, objectAWorldPos, tAnimationT), 1.0f);
	}

	// U Animation

	if(uAnimationSign != 0.0f)
	{
		if (uAnimationT < 0.5f)
		{
			renderA.pos = renderAInverse * glm::vec4(glm::mix(objectAWorldPos, glm::vec3(0.0f, 2.0f * uAnimationSign, 0.0f), uAnimationT * 2.0f), 1.0f);
			renderB.pos = renderBInverse * glm::vec4(glm::mix(objectBWorldPos, glm::vec3(0.0f, 2.0f * uAnimationSign * -1.0f, 0.0f), uAnimationT * 2.0f), 1.0f);
		}
		else
		{
			renderA.pos = renderAInverse * glm::vec4(glm::mix(glm::vec3(0.0f, 2.0f * uAnimationSign, 0.0f), objectBWorldPos, (uAnimationT - 0.5f) * 2.0f), 1.0f);
			renderB.pos = renderBInverse * glm::vec4(glm::mix(glm::vec3(0.0f, 2.0f * uAnimationSign * -1.0f, 0.0f), objectAWorldPos, (uAnimationT - 0.5f) * 2.0f), 1.0f);
		}
	}

	uniformColorShader->Bind();
	uniformColorShader->SetUniform("projection", projection);
	uniformColorShader->SetUniform("view", m_Camera->ViewMatrix());

	model = renderA.GetWorldTransform();

	uniformColorShader->SetUniform("model", model);
	uniformColorShader->SetUniform("a_Color", objectA.color);
	auto& objectAMeshes = objectAModel->Meshes();
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	for (auto& objectAMesh : objectAMeshes)
	{
		objectAMesh.Bind();
		glDrawElements(GL_TRIANGLES, objectAMesh.m_Indices.size(), GL_UNSIGNED_INT, (const void*)0);
	}
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	shader->Bind();
	model = renderB.GetWorldTransform();
	shader->SetUniform("model", model);
	auto& objectBMeshes = objectBModel->Meshes();
	for (auto& objectBMesh : objectBMeshes)
	{
		objectBMesh.Bind();
		glDrawElements(GL_TRIANGLES, objectBMesh.m_Indices.size(), GL_UNSIGNED_INT, (const void*)0);
	}
}

void Application::OnKeyEvent(GLFWwindow* window, int key, int action, int mods)
{
	if (action == GLFW_PRESS)
	{
		switch (key)
		{
		case GLFW_KEY_1:
			applyMode = APPLY_A;
			break;
		case GLFW_KEY_2:
			applyMode = APPLY_B;
			break;
		case GLFW_KEY_3:
			applyMode = APPLY_BOTH;
			break;
		case GLFW_KEY_C:
			swapModels = !swapModels;
			if (swapModels)
			{
				objectAModel.emplace("data\\1_opengl_basics\\cone.obj");
				objectBModel.emplace("data\\1_opengl_basics\\pyramid.obj");
			}
			else
			{
				objectAModel.emplace("data\\1_opengl_basics\\sphere.obj");
				objectBModel.emplace("data\\1_opengl_basics\\cube.obj");
			}
			break;
		case GLFW_KEY_S:
		{
			objectA = Object{};
			objectB = Object{};

			objectA.pos = glm::vec3(-3.0f, 0.0f, 0.0f);
			objectA.color = glm::vec3(0.0f, 0.0f, 1.0f);

			objectB.pos = glm::vec3(3.0f, 0.0f, 0.0f);

			tAnimationSign = 0.0f;
			tAnimationT = 0.0f;
			
			uAnimationSign = 0.0f;
			uAnimationT = 0.0f;

			vAnimationSign = 0.0f;
			vAnimationT = 0.0f;
			break;
		}
		case GLFW_KEY_T:
			uAnimationSign = 0.0f;
			uAnimationT = 0.0f;

			vAnimationSign = 0.0f;
			vAnimationT = 0.0f;

			tAnimationT = 0.0f;
			tAnimationSign = (tAnimationSign == 0.0f) ? 1.0f : 0.0f;
			break;
		case GLFW_KEY_U:
			objectA.rotateYOriginSign = 0.0f;
			objectB.rotateYOriginSign = 0.0f;

			tAnimationSign = 0.0f;
			tAnimationT = 0.0f;

			vAnimationSign = 0.0f;
			vAnimationT = 0.0f;

			uAnimationT = 0.0f;
			uAnimationSign = (uAnimationSign == 0.0f) ? 1.0f : 0.0f;
			break;
		case GLFW_KEY_V:
			tAnimationSign = 0.0f;
			tAnimationT = 0.0f;

			uAnimationSign = 0.0f;
			uAnimationT = 0.0f;

			vAnimationT = 0.0f;

			objectA.rotateYOriginSign = 1.0f;
			objectB.rotateYOriginSign = 1.0f;
			objectA.rotateYSign = 1.0f;
			objectB.rotateYSign = 1.0f;
			vAnimationSign = (vAnimationSign == 0.0f) ? 1.0f : 0.0f;
			if (vAnimationSign == 0.0f)
			{
				objectA.rotateYOriginSign = 0.0f;
				objectB.rotateYOriginSign = 0.0f;
				objectA.rotateYSign = 0.0f;
				objectB.rotateYSign = 0.0f;
			}
			break;
		case GLFW_KEY_Q:
			glfwSetWindowShouldClose(window, true);
			break;
		}
	}
}

void Application::OnCharEvent(GLFWwindow* window, unsigned int codepoint)
{
	switch (codepoint)
	{
	case 'x':
		if (applyMode & APPLY_A)
			objectA.rotateXSign = (objectA.rotateXSign == 1.0f) ? 0.0f : 1.0f;
		if (applyMode & APPLY_B) 
			objectB.rotateXSign = (objectB.rotateXSign == 1.0f) ? 0.0f : 1.0f;
		break;
	case 'X':
		if (applyMode & APPLY_A)
			objectA.rotateXSign = (objectA.rotateXSign == -1.0f) ? 0.0f : -1.0f;
		if (applyMode & APPLY_B)
			objectB.rotateXSign = (objectB.rotateXSign == -1.0f) ? 0.0f : -1.0f;
		break;
	case 'y':
		if (vAnimationSign != 0.0f) break;
		if (applyMode & APPLY_A)
			objectA.rotateYSign = (objectA.rotateYSign == 1.0f) ? 0.0f : 1.0f;
		if (applyMode & APPLY_B)
			objectB.rotateYSign = (objectB.rotateYSign == 1.0f) ? 0.0f : 1.0f;
		break;
	case 'Y':
		if (vAnimationSign != 0.0f) break;
		if (applyMode & APPLY_A)
			objectA.rotateYSign = (objectA.rotateYSign == -1.0f) ? 0.0f : -1.0f;
		if (applyMode & APPLY_B)
			objectB.rotateYSign = (objectB.rotateYSign == -1.0f) ? 0.0f : -1.0f;
		break;
	case 'r':
		if (vAnimationSign != 0.0f || uAnimationSign != 0.0f) break;
		if (applyMode & APPLY_A)
			objectA.rotateYOriginSign = (objectA.rotateYOriginSign == 1.0f) ? 0.0f : 1.0f;
		if (applyMode & APPLY_B)
			objectB.rotateYOriginSign = (objectB.rotateYOriginSign == 1.0f) ? 0.0f : 1.0f;
		break;
	case 'R':
		if (vAnimationSign != 0.0f || uAnimationSign != 0.0f) break;
		if (applyMode & APPLY_A)
			objectA.rotateYOriginSign = (objectA.rotateYOriginSign == -1.0f) ? 0.0f : -1.0f;
		if (applyMode & APPLY_B)
			objectB.rotateYOriginSign = (objectB.rotateYOriginSign == -1.0f) ? 0.0f : -1.0f;
		break;
	case 'a':
		if (vAnimationSign != 0.0f) break;
		if (applyMode & APPLY_A)
			objectA.scaleSign = (objectA.scaleSign == 1.0f) ? 0.0f : 1.0f;
		if (applyMode & APPLY_B)
			objectB.scaleSign = (objectB.scaleSign == 1.0f) ? 0.0f : 1.0f;
		break;
	case 'A':
		if (vAnimationSign != 0.0f) break;
		if (applyMode & APPLY_A)
			objectA.scaleSign = (objectA.scaleSign == -1.0f) ? 0.0f : -1.0f;
		if (applyMode & APPLY_B)
			objectB.scaleSign = (objectB.scaleSign == -1.0f) ? 0.0f : -1.0f;
		break;
	case 'b':
		if (applyMode & APPLY_A)
			objectA.scaleAboutOriginSign = (objectA.scaleAboutOriginSign == 1.0f) ? 0.0f : 1.0f;
		if (applyMode & APPLY_B)
			objectB.scaleAboutOriginSign = (objectB.scaleAboutOriginSign == 1.0f) ? 0.0f : 1.0f;
		break;
	case 'B':
		if (applyMode & APPLY_A)
			objectA.scaleAboutOriginSign = (objectA.scaleAboutOriginSign == -1.0f) ? 0.0f : -1.0f;
		if (applyMode & APPLY_B)
			objectB.scaleAboutOriginSign = (objectB.scaleAboutOriginSign == -1.0f) ? 0.0f : -1.0f;
		break;
	case 'd':
		if (applyMode & APPLY_A)
			objectA.pos.x -= 0.1f;
		if (applyMode & APPLY_B)
			objectB.pos.x -= 0.1f;
		break;
	case 'D':
		if (applyMode & APPLY_A)
			objectA.pos.x += 0.1f;
		if (applyMode & APPLY_B)
			objectB.pos.x += 0.1f;
		break;
	case 'e':
		if (applyMode & APPLY_A)
			objectA.pos.y -= 0.1f;
		if (applyMode & APPLY_B)
			objectB.pos.y -= 0.1f;
		break;
	case 'E':
		if (applyMode & APPLY_A)
			objectA.pos.y += 0.1f;
		if (applyMode & APPLY_B)
			objectB.pos.y += 0.1f;
		break;
	}
}