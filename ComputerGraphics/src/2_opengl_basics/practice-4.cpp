// Standard C++ Libraries
#include <iostream>
#include <print>
#include <array>

#include <random>
// OpenGL Libraries
#include <GL/glew.h>
#include <glfw/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

constexpr float epsilon{ 0.001f };
std::random_device rd;
std::default_random_engine dre{ rd() };
std::uniform_real_distribution<float> colorUrd{0.0f, 1.0f};

void FrameBufferSizeCallback(GLFWwindow* window, int width, int height);
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void CursorPosCallback(GLFWwindow* window, double xPosIn, double yPosIn);
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

void UpdateRects();
void DrawScene();

struct Rect
{
	glm::vec2 pos{};
	glm::vec2 size{};
	glm::vec2 dir{};
	glm::vec3 color{};

	float screenDistanceTick{ 5.0f };
};

glm::vec2 screenSize{ 800, 600 };
glm::vec2 mousePos;

size_t rectCount{};
const int maxRects{ 5 };

std::array<Rect, 5> rects{};
std::array<glm::vec2, 5> originalPositions{};

bool disableMouse{ false };

bool moveDiag{false};
bool moveZigzag{false};
bool moveClock{false};
bool scaleMode{ false };
bool colorMode{ false };
bool pauseAnimation{false};

double lastTime{};
float speed{ 0.5f };

float colorInterpolationValue{};
int colorRepCount{ 0 };
std::array<glm::vec3, 5> colorA{};
std::array<glm::vec3, 5> colorB{};

int scaleRepCount{ 0 };
float scaleInterpolationValue{};
std::array<glm::vec2, 5> scaleA{};
std::array<glm::vec2, 5> scaleB{};

int main()
{
	if (!glfwInit())
	{
		std::println(std::cerr, "failed to initialize GLFW!");
		return -1;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);

	GLFWwindow* window = glfwCreateWindow(800, 600, "practice 4", nullptr, nullptr);
	if (!window)
	{
		std::println(std::cerr, "failed to create Window!");
		glfwTerminate();
		return -1;
	}

	glfwMakeContextCurrent(window);

	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK)
	{
		std::println(std::cerr, "Failed to initialize GLEW!");
		return -1;
	}

	glfwSetFramebufferSizeCallback(window, FrameBufferSizeCallback);
	glfwSetKeyCallback(window, KeyCallback);
	glfwSetMouseButtonCallback(window, MouseButtonCallback);
	glfwSetCursorPosCallback(window, CursorPosCallback);

	glViewport(0, 0, 800, 600);
	screenSize = glm::vec2(800.f, 600.f);

	while (!glfwWindowShouldClose(window))
	{
		if(!pauseAnimation)
		{
			UpdateRects();
		}
		DrawScene();

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwDestroyWindow(window);
	glfwTerminate();
}

void FrameBufferSizeCallback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
	screenSize = glm::vec2(static_cast<float>(width), static_cast<float>(height));
}

void InsertRect()
{
	if (rectCount >= 5) return;
	auto& rect = rects[rectCount];
	originalPositions[rectCount] = mousePos;
	rect.pos = mousePos;
	rect.size = glm::vec2(0.1f);
	rect.color = glm::vec3(colorUrd(dre), colorUrd(dre), colorUrd(dre));
	++rectCount;
}

void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	if (!disableMouse && button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
	{
		InsertRect();
	}
}

void CursorPosCallback(GLFWwindow* window, double xPosIn, double yPosIn)
{
	mousePos.x = static_cast<float>(xPosIn) / screenSize.x * 2.0f - 1.0f;
	mousePos.y = 1.0f - static_cast<float>(yPosIn) / screenSize.y * 2.0f;
}

void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (action == GLFW_PRESS)
	{
		switch (key)
		{
		case GLFW_KEY_1:
			if (moveZigzag || moveClock) break;
			if (!moveDiag)
			{
				for (int i = 0; i < rectCount; ++i)
				{
					rects[i].dir = glm::vec2(glm::cos(glm::radians(45.f)), glm::sin(glm::radians(45.f)));
				}
				moveDiag = true;
				disableMouse = true;
			}
			else
			{
				moveDiag = false;
			}
			break;
		case GLFW_KEY_2:
			if (moveDiag || moveClock) break;
			if (!moveZigzag)
			{
				for (int i = 0; i < rectCount; ++i)
				{
					rects[i].dir = glm::vec2(glm::cos(glm::radians(0.0f)), -glm::sin(glm::radians(0.f)));
				}
				moveZigzag = true;
				disableMouse = true;
			}
			else
			{
				moveZigzag = false;
			}
			break;
		case GLFW_KEY_3:
			if (moveDiag || moveZigzag) break;
			if (!moveClock)
			{
				moveClock = true;
				disableMouse = true;
				glfwSetTime(0);
				lastTime = 0;
			}
			else
			{
				moveClock = false;
			}
			break;
		case GLFW_KEY_4:
			scaleMode = !scaleMode;
			scaleInterpolationValue = 0.0f;
			scaleRepCount = 0;
			break;
		case GLFW_KEY_5:
			colorMode = !colorMode;
			colorInterpolationValue = 0.0f;
			colorRepCount = 0;
			break;
		case GLFW_KEY_S:
			glfwSetTime(0);
			lastTime = 0;
			pauseAnimation = !pauseAnimation;
			break;
		case GLFW_KEY_M:
			for (int i = 0; i < rectCount; ++i)
			{
				rects[i].pos = originalPositions[i];
			}
			break;
		case GLFW_KEY_R:
			rectCount = 0;
			disableMouse = false;
			pauseAnimation = false;
			scaleMode = false;
			colorMode = false;
			moveDiag = false;
			moveZigzag = false;
			moveClock = false;
			lastTime = 0;
			break;
		case GLFW_KEY_Q:
			glfwSetWindowShouldClose(window, true);
			break;
		case GLFW_KEY_EQUAL:
			if (mods & GLFW_MOD_SHIFT)
			{

			}
			break;
		default:
			break;
		}
	}
}

void UpdateRects()
{
	double currentTime = glfwGetTime();
	float deltaTime = static_cast<float>(currentTime - lastTime);
	lastTime = currentTime;

	float c = std::cos(currentTime);
	float s = -std::sin(currentTime);

	float scale = std::max(std::abs(c), std::abs(s));

	for (int i = 0; i < rectCount; ++i)
	{
		auto& rect = rects[i];
		if(moveDiag || moveZigzag)
		{
			rect.pos += rect.dir * speed * deltaTime;
		}

		if (moveDiag)
		{
			// collision check
			auto halfSize = rect.size * 0.5f;
			if (rect.pos.x - halfSize.x < -1.f || rect.pos.x + halfSize.x > 1.0f ||
				rect.pos.y - halfSize.y < -1.0f || rect.pos.y + halfSize.y > 1.0f)
			{
				rect.dir = glm::vec2(rect.dir.y, -rect.dir.x);
			}
		}
		else if (moveZigzag)
		{
			auto halfSize = rect.size * 0.5f;
			if (rect.pos.x - halfSize.x < -1.f || rect.pos.x + halfSize.x > 1.0f)
			{
				if(rect.pos.x - halfSize.x < -1.f) {
					rect.dir = glm::vec2(glm::cos(glm::radians(0.0f)), -glm::sin(glm::radians(0.0f)));
					rect.pos.x = -1.f + halfSize.x + 0.0001f;
				}
				else
				{
					rect.dir = glm::vec2(glm::cos(glm::radians(180.0f)), -glm::sin(glm::radians(180.f)));
					rect.pos.x = 1.f - halfSize.x - 0.0001f;
				}

				rect.pos.y -= rect.size.y;
			}

			if (rect.pos.y - halfSize.y < -1.f)
			{
				rect.pos.y = 1.f + -halfSize.y;
			}
		}
		else if (moveClock)
		{
			rect.pos = glm::vec2(c / scale , s / scale);
			// 충돌 체크로 각 사각형들을 따로 보이게 하면 더 좋을듯
		}
	}

	if (scaleMode)
	{
		scaleInterpolationValue += deltaTime * 2 * glm::pi<float>();
		std::uniform_real_distribution scaleDistribution{ 0.2f, 0.5f };
		if (scaleInterpolationValue > (scaleRepCount * glm::pi<float>()) + (0.5f * glm::pi<float>()))
		{
			if (scaleRepCount % 2 == 0)
			{
				for (int i = 0; i < rectCount; ++i)
				{
					scaleB[i] = glm::vec2(0.1f, scaleDistribution(dre));
				}
			}
			else
			{
				for (int i = 0; i < rectCount; ++i)
				{
					scaleA[i] = glm::vec2(scaleDistribution(dre), 0.1f);
				}
			}
			++scaleRepCount;
		}
		float t = glm::sin(scaleInterpolationValue) * 0.5f + 0.5f; 

		for (int i = 0; i < rectCount; ++i)
		{
			rects[i].size = scaleA[i] * t + (1.f - t) * scaleB[i];
		}
	}
	if (colorMode)
	{
		colorInterpolationValue += deltaTime * 2 * glm::pi<float>();
		if (colorInterpolationValue > (colorRepCount * glm::pi<float>()) + (0.5f * glm::pi<float>()))
		{
			if (colorRepCount % 2 == 0)
			{
				for (int i = 0; i < rectCount; ++i)
				{
					colorB[i] = glm::vec3(colorUrd(dre), colorUrd(dre), colorUrd(dre));
				}
			}
			else
			{
				for (int i = 0; i < rectCount; ++i)
				{
					colorA[i] = glm::vec3(colorUrd(dre), colorUrd(dre), colorUrd(dre));
				}
			}
			++colorRepCount;
		}
		float t = glm::sin(colorInterpolationValue) * 0.5f + 0.5f;
		colorInterpolationValue += deltaTime * 2 * glm::pi<float>();

		for (int i = 0; i < rectCount; ++i)
		{
			rects[i].color = colorA[i] * t + (1.f - t) * colorB[i];
		}
	}

}
void DrawScene()
{
	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	// Draw Rects
	for (size_t i = 0; i < rectCount; ++i)
	{
		auto& rect = rects[i];
		auto halfSize = rect.size * 0.5f;
		glColor3f(rect.color.r, rect.color.g, rect.color.b);
		glRectf(rect.pos.x - halfSize.x, rect.pos.y - halfSize.y, rect.pos.x + halfSize.x, rect.pos.y + halfSize.y);
	}
}