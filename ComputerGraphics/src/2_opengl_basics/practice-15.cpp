#define CG_APPLICATION_CUSTOM_CALLBACK_KEY
#include <CG.h>

#include <Shader.h>

#include <random>
#include <algorithm>

#include <optional>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace
{
	std::random_device rd;
	std::default_random_engine dre{ rd() };

	std::optional<Model> axes{};
	std::optional<Model> object{};

	std::vector<Vertex> objectUpperVertices{};

	std::optional<Shader> shader{};
	bool isCube{ true };
	bool rotateYaw{ false };
	float yaw{ 0.0f };
	float animationAngularSpeed{ 90.0f };
	float animationSpeed{ 1.5f };
	
	bool rotateUpperByZ{ false };
	float rotateUpperByZT{ 0.0f };

	float frontMovingSign{ 0.0f };
	float frontOpenT{ 0.0f };

	bool rotateSideByMiddle{ false };
	float rotateSideByMiddleT{0.0f};

	bool scaleBack{ false };
	float scaleBackT{ 0.0f };

	std::pair<int, int> doubleFaces{ -1, -1 };
	float spinDoubleByYT{ 0.0f };
	float swapDoubleByYT{ 0.0f };

	bool sideQuadSpin{ false };
	float sideQuadSpinAngle{ 0.0f };
	bool BaseTwoFaceSpin{ false };
	float BaseTwoFaceSpinAngle{ 0.0f };

	float cubeUnfoldSign{ 0.0f };
	float cubeUnfoldT{ 0.0f };

	float objectTransitionSign{ 0.0f };
	float objectTransitionT{ 0.0f };

	// Pyramid status
	float pyramidUnfoldOverPlaneSign{ 0.0f };
	float pyramidUnfoldOverPlaneT{ 0.0f };

	float pyramidUnfoldSign{ 0.0f };
	float pyramidUnfoldT{ 0.0f };
	int UnfoldCount{ -1 };
	
	float swapDoublePyramidFacesByApexT{ 0.0f };

	float openPyramidSideT{ 0.0f };
	float openPyramidSideSign{ 0.0f };
}

glm::vec3 GetSharedEdgePivot(int a, int b);
float GetSharedFacesSwapRotateSign(int a, int b);
int main()
{
	Application app{ 800, 800 };
	int code = app.Init("i hate computer graphics 3");
	if (code) return code;
	app.EnableOpenGLFeatures(GL_DEPTH_TEST);
	app.Run();
}

void Application::OnInit()
{
	shader.emplace("shaders\\1_opengl_basics\\projectionView.vs", "shaders\\1_opengl_basics\\vertexColor.fs");
	axes.emplace("data\\1_opengl_basics\\axes.obj");
	object.emplace("data\\1_opengl_basics\\cube.obj");

	CreateCamera(glm::vec3(-3.0f, 2.0f, -3.0f));
	m_Camera->FocusAt(glm::vec3(0.0f, 0.0f, 0.0f));

}

void Application::Update()
{
	if (rotateYaw)
	{
		yaw += animationAngularSpeed * deltaTime;
	}
	if (rotateUpperByZ)
		rotateUpperByZT += animationAngularSpeed * deltaTime;
	if (frontMovingSign != 0.0f)
	{
		frontOpenT = glm::clamp(frontOpenT + (frontMovingSign * animationSpeed * deltaTime), 0.0f, 1.0f);
		if (frontOpenT <= 0.0f || frontOpenT >= 1.0f)
			frontMovingSign = 0.0f;
	}
	if (rotateSideByMiddle)
		rotateSideByMiddleT += animationAngularSpeed * deltaTime;
	if (scaleBack)
		scaleBackT += animationSpeed * deltaTime;
	if (isCube && doubleFaces.first != -1 && doubleFaces.second != -1)
	{
		if(spinDoubleByYT > 0.0f)
		{
			spinDoubleByYT = glm::clamp(spinDoubleByYT + animationSpeed * deltaTime, 0.0f, 1.0f);
			if (spinDoubleByYT >= 1.0f)
			{
				spinDoubleByYT = 0.0f;
				doubleFaces = { -1, -1 };
			}
		}
		else if (swapDoubleByYT > 0.0f)
			swapDoubleByYT = glm::clamp(swapDoubleByYT + animationSpeed * deltaTime, 0.0f, 1.0f);
	}

	if (sideQuadSpin)
		sideQuadSpinAngle += animationAngularSpeed * deltaTime;

	if (BaseTwoFaceSpin)
		BaseTwoFaceSpinAngle += animationAngularSpeed * deltaTime;

	if (cubeUnfoldSign != 0.0f)
	{
		cubeUnfoldT = glm::clamp(cubeUnfoldT + cubeUnfoldSign * animationSpeed * deltaTime, 0.0f, 1.0f);
		if (cubeUnfoldT <= 0.0f || cubeUnfoldT >= 1.0f)
			cubeUnfoldSign = 0.0f;
	}

	if (objectTransitionSign != 0.0f)
	{
		objectTransitionT = glm::clamp(objectTransitionT + objectTransitionSign * animationSpeed * deltaTime, 0.0f, 1.0f);
		auto& objectVertices = object->Meshes().front().m_Vertices;
		for (int i = 0; i < 6; ++i)
		{
			objectVertices[12 + i].pos = (1.0f - objectTransitionT) * objectUpperVertices[i].pos;
			objectVertices[12 + i].pos.y = objectUpperVertices[i].pos.y;
		}

		objectVertices[1].pos = (1.0f - objectTransitionT) * objectUpperVertices[2].pos;
		objectVertices[4].pos = (1.0f - objectTransitionT) * objectUpperVertices[0].pos;
		objectVertices[5].pos = (1.0f - objectTransitionT) * objectUpperVertices[2].pos;

		objectVertices[1].pos.y = objectUpperVertices[2].pos.y;
		objectVertices[4].pos.y = objectUpperVertices[0].pos.y;
		objectVertices[5].pos.y = objectUpperVertices[2].pos.y;

		objectVertices[7].pos = (1.0f - objectTransitionT) * objectUpperVertices[4].pos;
		objectVertices[8].pos = (1.0f - objectTransitionT) * objectUpperVertices[0].pos;
		objectVertices[11].pos = (1.0f - objectTransitionT) * objectUpperVertices[4].pos;

		objectVertices[7].pos.y = objectUpperVertices[4].pos.y;
		objectVertices[8].pos.y = objectUpperVertices[0].pos.y;
		objectVertices[11].pos.y = objectUpperVertices[4].pos.y;

		objectVertices[19].pos = (1.0f - objectTransitionT) * objectUpperVertices[2].pos;
		objectVertices[20].pos = (1.0f - objectTransitionT) * objectUpperVertices[1].pos;
		objectVertices[22].pos = (1.0f - objectTransitionT) * objectUpperVertices[1].pos;

		objectVertices[19].pos.y = objectUpperVertices[2].pos.y;
		objectVertices[20].pos.y = objectUpperVertices[1].pos.y;
		objectVertices[22].pos.y = objectUpperVertices[1].pos.y;

		objectVertices[32].pos = (1.0f - objectTransitionT) * objectUpperVertices[1].pos;
		objectVertices[34].pos = (1.0f - objectTransitionT) * objectUpperVertices[1].pos;
		objectVertices[35].pos = (1.0f - objectTransitionT) * objectUpperVertices[4].pos;

		objectVertices[32].pos.y = objectUpperVertices[1].pos.y;
		objectVertices[34].pos.y = objectUpperVertices[1].pos.y;
		objectVertices[35].pos.y = objectUpperVertices[4].pos.y;

		object->Meshes().front().UpdateVertices();
		if (objectTransitionT <= 0.0f || objectTransitionT >= 1.0f)
		{
			objectTransitionSign = 0.0f;
			isCube = !isCube;
			if (!isCube) object.emplace("data\\1_opengl_basics\\pyramid.obj");
		}
	}

	if (pyramidUnfoldOverPlaneSign != 0.0f)
	{
		pyramidUnfoldOverPlaneT = glm::clamp(pyramidUnfoldOverPlaneT + pyramidUnfoldOverPlaneSign * animationSpeed * deltaTime, 0.0f, 1.0f);
		if (pyramidUnfoldOverPlaneT <= 0.0f || pyramidUnfoldOverPlaneT >= 1.0f)
			pyramidUnfoldOverPlaneSign = 0.0f;
	}

	if (pyramidUnfoldSign != 0.0f)
	{
		pyramidUnfoldT = glm::clamp(pyramidUnfoldT + pyramidUnfoldSign * animationSpeed * deltaTime, 0.0f, 1.0f);
		if (pyramidUnfoldT <= 0.0f || pyramidUnfoldT >= 1.0f)
		{
			if (UnfoldCount >= 8) { UnfoldCount = -1; pyramidUnfoldSign = 0.0f; pyramidUnfoldT = 0.0f; }
			else
			{
				pyramidUnfoldSign = -pyramidUnfoldSign;
				UnfoldCount++;
			}
		}
	}

	if (!isCube && doubleFaces.first != -1 && doubleFaces.second != -1)
	{
		swapDoublePyramidFacesByApexT = glm::clamp(swapDoublePyramidFacesByApexT + animationSpeed * deltaTime, 0.0f, 1.0f);
	}

	if (openPyramidSideSign != 0.0f)
	{
		openPyramidSideT = glm::clamp(openPyramidSideT + openPyramidSideSign * animationSpeed * deltaTime, 0.0f, 1.0f);
		if (openPyramidSideT <= 0.0f || openPyramidSideT >= 1.0f)
		{
			openPyramidSideSign = 0.0f;
		}
	}
}

void DrawObject()
{
	glm::mat4 model{ glm::mat4(1.0f) };
	if (rotateYaw)
	{
		model = glm::rotate(model, glm::radians(yaw), glm::vec3(0.0f, 1.0f, 0.0f));
	}
	auto& objectMeshes = object->Meshes();
	if (isCube)
	{
		objectMeshes.front().Bind();
		for (int i = 0; i < 6; ++i)
		{
			glm::mat4 animationTransform{ model };
			if (i == 0)
			{
				animationTransform = glm::rotate(animationTransform, glm::radians(rotateSideByMiddleT), glm::vec3(0.0f, 0.0f, 1.0f));

				animationTransform = glm::rotate(animationTransform, glm::radians(sideQuadSpinAngle), glm::vec3(0.0f, 1.0f, 0.0f));

				// unfold
				animationTransform = glm::translate(animationTransform, glm::vec3(0.0f, -0.5f, -0.5f));
				animationTransform = glm::rotate(animationTransform, glm::radians(cubeUnfoldT * -90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
				animationTransform = glm::translate(animationTransform, glm::vec3(0.0f, 0.5f, 0.5f));
			}
			// Open Front
			if (i == 1)
			{
				animationTransform = glm::translate(animationTransform, glm::vec3(-0.5f, 0.5f, 0.0f));
				animationTransform = glm::rotate(animationTransform, glm::radians(-frontOpenT * 90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
				animationTransform = glm::translate(animationTransform, glm::vec3(0.5f,-0.5f, 0.0f));

				animationTransform = glm::rotate(animationTransform, glm::radians(sideQuadSpinAngle), glm::vec3(0.0f, 1.0f, 0.0f));
				animationTransform = glm::rotate(animationTransform, glm::radians(BaseTwoFaceSpinAngle), glm::vec3(1.0f, 0.0f, 0.0f));

				animationTransform = glm::translate(animationTransform, glm::vec3(-0.5f, -0.5f, 0.0f));
				animationTransform = glm::rotate(animationTransform, glm::radians(cubeUnfoldT * 90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
				animationTransform = glm::translate(animationTransform, glm::vec3(0.5f, 0.5f, 0.0f));

			}
			// Rotate Up by Z
			if (i == 2)
			{
				animationTransform = glm::rotate(animationTransform, glm::radians(rotateUpperByZT), glm::vec3(0.0f, 0.0f, 1.0f));
				animationTransform = glm::rotate(animationTransform, glm::radians(BaseTwoFaceSpinAngle), glm::vec3(1.0f, 0.0f, 0.0f));
				
				animationTransform = glm::translate(animationTransform, glm::vec3(0.0f, -0.5f, -0.5f));
				animationTransform = glm::rotate(animationTransform, glm::radians(cubeUnfoldT * -90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
				animationTransform = glm::translate(animationTransform, glm::vec3(0.0f, 0.5f, 0.5f));


				animationTransform = glm::translate(animationTransform, glm::vec3(0.0f, 0.5f, -0.5f));
				animationTransform = glm::rotate(animationTransform, glm::radians(cubeUnfoldT * -90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
				animationTransform = glm::translate(animationTransform, glm::vec3(0.0f, -0.5f, 0.5f));

			}
			// back
			if (i == 3)
			{
				if (scaleBack)
				{
					animationTransform = glm::translate(animationTransform, glm::vec3(0.5f, 0.0f, 0.0f));
					animationTransform = glm::scale(animationTransform, glm::vec3(0.5f * glm::sin(scaleBackT) + 0.5f));
					animationTransform = glm::translate(animationTransform, glm::vec3(-0.5f, 0.0f, 0.0f));
				}

				animationTransform = glm::rotate(animationTransform, glm::radians(sideQuadSpinAngle), glm::vec3(0.0f, 1.0f, 0.0f));
				animationTransform = glm::rotate(animationTransform, glm::radians(BaseTwoFaceSpinAngle), glm::vec3(1.0f, 0.0f, 0.0f));

				animationTransform = glm::translate(animationTransform, glm::vec3(0.5f, -0.5f, 0.0f));
				animationTransform = glm::rotate(animationTransform, glm::radians(cubeUnfoldT * -90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
				animationTransform = glm::translate(animationTransform, glm::vec3(-0.5f, 0.5f, 0.0f));
			}
			if (i == 4)
			{
				animationTransform = glm::rotate(animationTransform, glm::radians(BaseTwoFaceSpinAngle), glm::vec3(1.0f, 0.0f, 0.0f));

			}
			if (i == 5)
			{
				animationTransform = glm::rotate(animationTransform, glm::radians(sideQuadSpinAngle), glm::vec3(0.0f, 1.0f, 0.0f));

				animationTransform = glm::translate(animationTransform, glm::vec3(0.0f, -0.5f, 0.5f));
				animationTransform = glm::rotate(animationTransform, glm::radians(cubeUnfoldT * 90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
				animationTransform = glm::translate(animationTransform, glm::vec3(0.0f, 0.5f, -0.5f));
			}
			if (i == doubleFaces.first || i == doubleFaces.second)
			{
				glm::vec3 axisPivot = GetSharedEdgePivot(doubleFaces.first, doubleFaces.second);
				animationTransform = glm::translate(animationTransform, axisPivot);
				animationTransform = glm::rotate(animationTransform, glm::radians(spinDoubleByYT * 360.0f) ,glm::vec3(0.0f, 1.0f, 0.0f));
				animationTransform = glm::translate(animationTransform, -axisPivot);

				float sign = GetSharedFacesSwapRotateSign(doubleFaces.first, doubleFaces.second);
				sign = (i == doubleFaces.first) ? sign : -sign;
				animationTransform = glm::rotate(animationTransform, glm::radians(sign * swapDoubleByYT * 90.0f), glm::vec3(0.0f, 1.0f, 0.0f));

			}
			shader->SetUniform("model", animationTransform);
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (const void*)(i * 6 * sizeof(unsigned int)));
		}
	}
	else
	{
		if(objectTransitionT >= 1.0f){
			auto& objectMesh = objectMeshes.front();
			objectMesh.Bind();
			for (int i = 0; i < 4; ++i)
			{
				glm::mat4 animationTransform{ model };
				if (i == 0)
				{
					animationTransform = glm::translate(animationTransform, glm::vec3(0.0f, -0.5f, -0.5f));
					animationTransform = glm::rotate(animationTransform, pyramidUnfoldOverPlaneT * -2.0f * (glm::half_pi<float>() + glm::atan(0.5f)), glm::vec3(1.0f, 0.0f, 0.0f));
					animationTransform = glm::translate(animationTransform, glm::vec3(0.0f, 0.5f, 0.5f));

					if(UnfoldCount / 2 == 0)
					{
						animationTransform = glm::translate(animationTransform, glm::vec3(0.0f, -0.5f, -0.5f));
						animationTransform = glm::rotate(animationTransform, -pyramidUnfoldT * (glm::half_pi<float>() + glm::atan(0.5f)), glm::vec3(1.0f, 0.0f, 0.0f));
						animationTransform = glm::translate(animationTransform, glm::vec3(0.0f, 0.5f, 0.5f));
					}

					animationTransform = glm::translate(animationTransform, glm::vec3(0.0f, 0.5f, 0.0f));
					animationTransform = glm::rotate(animationTransform, openPyramidSideT * (glm::atan(2.0f)), glm::vec3(1.0f, 0.0f, 0.0f));
					animationTransform = glm::translate(animationTransform, glm::vec3(0.0f, -0.5f, 0.0f));
				}
				if (i == 1)
				{
					animationTransform = glm::translate(animationTransform, glm::vec3(-0.5f, -0.5f, 0.0f));
					animationTransform = glm::rotate(animationTransform, pyramidUnfoldOverPlaneT * 2.0f * (glm::half_pi<float>() + glm::atan(0.5f)), glm::vec3(0.0f, 0.0f, 1.0f));
					animationTransform = glm::translate(animationTransform, glm::vec3(0.5f, 0.5f, 0.0f));

					if (UnfoldCount / 2 == 1)
					{
						animationTransform = glm::translate(animationTransform, glm::vec3(-0.5f, -0.5f, 0.0f));
						animationTransform = glm::rotate(animationTransform, pyramidUnfoldT * (glm::half_pi<float>() + glm::atan(0.5f)), glm::vec3(0.0f, 0.0f, 1.0f));
						animationTransform = glm::translate(animationTransform, glm::vec3(0.5f, 0.5f, 0.0f));
					}

					animationTransform = glm::translate(animationTransform, glm::vec3(0.0f, 0.5f, 0.0f));
					animationTransform = glm::rotate(animationTransform, -openPyramidSideT * (glm::atan(2.0f)), glm::vec3(0.0f, 0.0f, 1.0f));
					animationTransform = glm::translate(animationTransform, glm::vec3(0.0f, -0.5f, 0.0f));
				}
				if (i == 2)
				{
					animationTransform = glm::translate(animationTransform, glm::vec3(0.0f, -0.5f, 0.5f));
					animationTransform = glm::rotate(animationTransform, pyramidUnfoldOverPlaneT * 2.0f * (glm::half_pi<float>() + glm::atan(0.5f)), glm::vec3(1.0f, 0.0f, 0.0f));
					animationTransform = glm::translate(animationTransform, glm::vec3(0.0f, 0.5f, -0.5f));

					if (UnfoldCount / 2 == 2)
					{
						animationTransform = glm::translate(animationTransform, glm::vec3(0.0f, -0.5f, 0.5f));
						animationTransform = glm::rotate(animationTransform, pyramidUnfoldT * (glm::half_pi<float>() + glm::atan(0.5f)), glm::vec3(1.0f, 0.0f, 0.0f));
						animationTransform = glm::translate(animationTransform, glm::vec3(0.0f, 0.5f, -0.5f));
					}

					animationTransform = glm::translate(animationTransform, glm::vec3(0.0f, 0.5f, 0.0f));
					animationTransform = glm::rotate(animationTransform, -openPyramidSideT * (glm::atan(2.0f)), glm::vec3(1.0f, 0.0f, 0.0f));
					animationTransform = glm::translate(animationTransform, glm::vec3(0.0f, -0.5f, 0.0f));
				}
				if (i == 3)
				{

					animationTransform = glm::translate(animationTransform, glm::vec3(0.5f, -0.5f, 0.0f));
					animationTransform = glm::rotate(animationTransform, pyramidUnfoldOverPlaneT * -2.0f * (glm::half_pi<float>() + glm::atan(0.5f)), glm::vec3(0.0f, 0.0f, 1.0f));
					animationTransform = glm::translate(animationTransform, glm::vec3(-0.5f, 0.5f, 0.0f));

					if (UnfoldCount / 2 == 3)
					{
						animationTransform = glm::translate(animationTransform, glm::vec3(0.5f, -0.5f, 0.0f));
						animationTransform = glm::rotate(animationTransform, -pyramidUnfoldT * (glm::half_pi<float>() + glm::atan(0.5f)), glm::vec3(0.0f, 0.0f, 1.0f));
						animationTransform = glm::translate(animationTransform, glm::vec3(-0.5f, 0.5f, -0.0f));
					}

					animationTransform = glm::translate(animationTransform, glm::vec3(0.0f, 0.5f, 0.0f));
					animationTransform = glm::rotate(animationTransform, openPyramidSideT * (glm::atan(2.0f)), glm::vec3(0.0f, 0.0f, 1.0f));
					animationTransform = glm::translate(animationTransform, glm::vec3(0.0f, -0.5f, 0.0f));
				}

				if (i == doubleFaces.first || i == doubleFaces.second)
				{
					animationTransform = glm::rotate(animationTransform, swapDoublePyramidFacesByApexT * glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
				}
				shader->SetUniform("model", animationTransform);
				glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, (const void*)(i * 3 * sizeof(unsigned int)));
			}
			shader->SetUniform("model", model);
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (const void*)(12 * sizeof(unsigned int)));
		}
		else
		{
			objectMeshes.front().Bind();
			glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(objectMeshes.front().m_Indices.size()), GL_UNSIGNED_INT, (const void*)(0));
		}
	}
}

void Application::Render()
{
	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glm::mat4 model{ glm::mat4(1.0f) };

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
	DrawObject();
}

void ResetSingleFaceAnimations()
{
	rotateUpperByZ = false;
	rotateUpperByZT = 0.0f;
	frontMovingSign = 0.0f;
	frontOpenT = 0.0f;
	rotateSideByMiddle = false;
	rotateSideByMiddleT = 0.0f;
	scaleBack = false;
	scaleBackT = 0.0f;
}

void ResetDoubleFaceAnimations()
{
	doubleFaces = std::pair<int, int>{ -1, -1 };
	spinDoubleByYT = 0.0f;
	swapDoubleByYT = 0.0f;
}
void SelectAxisSharingFaces()
{
	std::vector<int> sharedIndices{ 0,1,3,5 };
	std::uniform_int_distribution<int> selectedFaces{ 0,1 };

	std::shuffle(sharedIndices.begin(), sharedIndices.end(), dre);
	doubleFaces.first = sharedIndices.back();
	switch (doubleFaces.first)
	{
	case 0:
		doubleFaces.second = (selectedFaces(dre)) ? 1 : 3;
		break;
	case 1:
		doubleFaces.second = (selectedFaces(dre)) ? 0 : 5;
		break;
	case 3:
		doubleFaces.second = (selectedFaces(dre)) ? 0 : 5;
		break;
	case 5:
		doubleFaces.second = (selectedFaces(dre)) ? 1 : 3;
		break;
	}
}

void ResetQuadFaceAnimations()
{
	sideQuadSpin = false;
	sideQuadSpinAngle = 0.0f;
	BaseTwoFaceSpin = false;
	BaseTwoFaceSpinAngle = 0.0f;
}

void ResetPyramidStatus()
{
	pyramidUnfoldOverPlaneSign = 0.0f;
	pyramidUnfoldOverPlaneT = 0.0f;
	pyramidUnfoldSign = 0.0f;
	pyramidUnfoldT = 0.0f;
	UnfoldCount = -1;
	doubleFaces = { -1, -1 };
	swapDoublePyramidFacesByApexT = 0.0f;
	openPyramidSideT = 0.0f;
	openPyramidSideSign = 0.0f;
}

void SelectPyramidSideFaces()
{
	std::uniform_int_distribution<int> selectedFaces{ 0,1 };
	int selectedFace = selectedFaces(dre);
	doubleFaces = { selectedFace, selectedFace + 2 };
}
void Application::OnKeyEvent(GLFWwindow* window, int key, int action, int mods)
{
	if(action == GLFW_PRESS)
	{
		switch (key)
		{
		case GLFW_KEY_H:
			if (IsOpenGLFeatureEnabled(GL_DEPTH_TEST)) DisableOpenGLFeatures(GL_DEPTH_TEST); else EnableOpenGLFeatures(GL_DEPTH_TEST);
			break;
		case GLFW_KEY_P:
			if (objectTransitionT > 0.0f && objectTransitionT < 1.0f || cubeUnfoldT > 0.0f) return;
			rotateYaw = false;
			yaw = 0.0f;
			ResetSingleFaceAnimations();
			ResetDoubleFaceAnimations();
			ResetQuadFaceAnimations();
			ResetPyramidStatus();
			objectTransitionSign = (objectTransitionT <= 0.0f) ? 1.0f : -1.0f;
			if (object && isCube)
			{
				auto& vertices = object->Meshes().front().m_Vertices;
				objectUpperVertices = std::vector<Vertex>(vertices.begin() + 12, vertices.begin() + 18);
			}
			else if(!isCube) object.emplace("data\\1_opengl_basics\\cube.obj");
			break;
		case GLFW_KEY_Y:
			rotateYaw = !rotateYaw;
			break;
		case GLFW_KEY_R:
			if(!isCube) object.emplace("data\\1_opengl_basics\\cube.obj");
			isCube = true;
			objectTransitionT = 0.0f;
			objectTransitionSign = 0.0f;

			ResetSingleFaceAnimations();
			ResetDoubleFaceAnimations();
			ResetQuadFaceAnimations();
			break;
		case GLFW_KEY_T:
			if (!isCube) return;
			ResetDoubleFaceAnimations();
			ResetQuadFaceAnimations();
			rotateUpperByZ = !rotateUpperByZ;
			break;
		case GLFW_KEY_F:
			if (frontOpenT > 0.0f && frontOpenT < 1.0f) break;
			ResetDoubleFaceAnimations();
			ResetQuadFaceAnimations();
			frontMovingSign = (frontOpenT <= 0.0f) ? 1.0f : -1.0f;
			break;
		case GLFW_KEY_S:
			ResetDoubleFaceAnimations();
			ResetQuadFaceAnimations();
			rotateSideByMiddle = !rotateSideByMiddle;
			break;
		case GLFW_KEY_B:
			if (cubeUnfoldT > 0.0f) return;
			ResetDoubleFaceAnimations();
			ResetQuadFaceAnimations();
			scaleBack = !scaleBack;
			break;
		case GLFW_KEY_1:
			if (spinDoubleByYT > 0.0f || swapDoubleByYT > 0.0f && swapDoubleByYT < 1.0f || objectTransitionT > 0.0f) break;
			spinDoubleByYT = std::numeric_limits<float>::epsilon();
			ResetSingleFaceAnimations();
			ResetQuadFaceAnimations();
			SelectAxisSharingFaces();
			break;
		case GLFW_KEY_2:
			if (spinDoubleByYT > 0.0f || swapDoubleByYT > 0.0f && swapDoubleByYT < 1.0f || objectTransitionT > 0.0f) break;
			swapDoubleByYT = std::numeric_limits<float>::epsilon();
			ResetSingleFaceAnimations();
			ResetQuadFaceAnimations();
			SelectAxisSharingFaces();
			break;
		case GLFW_KEY_3:
			if (cubeUnfoldT > 0.0f && cubeUnfoldT < 1.0f || objectTransitionT > 0.0f) break;
			ResetSingleFaceAnimations();
			ResetDoubleFaceAnimations();
			ResetQuadFaceAnimations();
			cubeUnfoldSign = (cubeUnfoldT <= 0.0f) ? 1.0f : -1.0f;
			break;
		case GLFW_KEY_4:
			if(objectTransitionT > 0.0f)break;
			ResetSingleFaceAnimations();
			ResetDoubleFaceAnimations();
			sideQuadSpin = !sideQuadSpin;
			break;
		case GLFW_KEY_5:
			if (objectTransitionT > 0.0f)break;
			ResetSingleFaceAnimations();
			ResetDoubleFaceAnimations();
			BaseTwoFaceSpin = !BaseTwoFaceSpin;
			break;
		case GLFW_KEY_6:
			if (isCube || pyramidUnfoldOverPlaneT > 0.0f && pyramidUnfoldOverPlaneT < 1.0f) break;
			pyramidUnfoldSign = 0.0f;
			pyramidUnfoldT = 0.0f;
			UnfoldCount = -1;
			doubleFaces = { -1, -1 };
			swapDoublePyramidFacesByApexT = 0.0f;
			openPyramidSideT = 0.0f;
			pyramidUnfoldOverPlaneSign = (pyramidUnfoldOverPlaneT<= 0.0f) ? 1.0f : -1.0f;
			openPyramidSideT = 0.0f;
			openPyramidSideSign = 0.0f;
			break;
		case GLFW_KEY_7:
			if (isCube || UnfoldCount >= 0) break;
			pyramidUnfoldOverPlaneSign = 0.0f;
			pyramidUnfoldOverPlaneT = 0.0f;
			UnfoldCount = 0;
			pyramidUnfoldSign = 1.0f;
			doubleFaces = { -1, -1 };
			swapDoublePyramidFacesByApexT = 0.0f;
			openPyramidSideT = 0.0f;
			openPyramidSideSign = 0.0f;
			break;
		case GLFW_KEY_8:
			if (isCube) break;
			pyramidUnfoldSign = 0.0f;
			pyramidUnfoldT = 0.0f;
			UnfoldCount = -1;
			pyramidUnfoldOverPlaneSign = 0.0f;
			pyramidUnfoldOverPlaneT = 0.0f;
			swapDoublePyramidFacesByApexT = 0.0f;
			SelectPyramidSideFaces();
			openPyramidSideT = 0.0f;
			openPyramidSideSign = 0.0f;
			break;
		case GLFW_KEY_9:
			if (isCube) break;
			pyramidUnfoldSign = 0.0f;
			pyramidUnfoldT = 0.0f;
			UnfoldCount = -1;
			pyramidUnfoldOverPlaneSign = 0.0f;
			pyramidUnfoldOverPlaneT = 0.0f;
			swapDoublePyramidFacesByApexT = 0.0f;
			doubleFaces = { -1, -1 };

			openPyramidSideSign = (openPyramidSideT <= 0.0f) ? 1.0f : -1.0f;
			break;
		case GLFW_KEY_Q:
			glfwSetWindowShouldClose(m_Window, true);
			break;
		case GLFW_KEY_ESCAPE:
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			isMouseOutOfFocus = true;
			break;
		}
	}
}

glm::vec3 GetSharedEdgePivot(int a, int b)
{
	int xFace, zFace;
	if (a == 1 || a == 3) { xFace = a; zFace = b; }
	else { xFace = b; zFace = a; }

	float x = (xFace == 1) ? -0.5f : 0.5f;
	float z = (zFace == 0) ? -0.5f : 0.5f;

	return { x, 0.0f, z };
}

float GetSharedFacesSwapRotateSign(int a, int b)
{
	if ((a == 5 && b == 3) || (a == 3 && b == 0) || (a == 0 && b == 1) || (a == 1 && b == 5))
		return 1.0f;
	return -1.0f;
}