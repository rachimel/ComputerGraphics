// Standard C++ Libraries
#include <iostream>
#include <print>
#include <array>
#include <queue>

#include <random>
// OpenGL Libraries
#include <GL/glew.h>
#include <glfw/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

constexpr float epsilon{ 0.001f };
std::random_device rd;
std::default_random_engine dre{ rd() };
std::uniform_real_distribution<float> colorUrd{ 0.0f, 1.0f };

float rectSize;
std::uniform_real_distribution rectSpawnSize{ 0.05f, 0.1f };


std::array<glm::vec2, 8> dirs{
		glm::vec2(-1.f, 0.f),
		glm::vec2(1.f, 0.f),
		glm::vec2(0.f, -1.f),
		glm::vec2(0.f, 1.f),
		glm::normalize(glm::vec2(1.f, 1.f)),
		glm::normalize(glm::vec2(-1.f, -1.f)),
		glm::normalize(glm::vec2(-1.f, 1.f)),
		glm::normalize(glm::vec2(1.f, -1.f)),
};

void FrameBufferSizeCallback(GLFWwindow* window, int width, int height);
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void CursorPosCallback(GLFWwindow* window, double xPosIn, double yPosIn);
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

void InitRects();
void UpdateEffects();
void DrawScene();

enum
{
	SPLIT_NORMAL,
	SPLIT_DIAG,
	SPLIT_ALONG,
	SPLIT_OCT
};
struct Rect
{
	glm::vec2 pos{};
	glm::vec2 size{};
	glm::vec3 color{};
	glm::vec2 dir{};

	bool visible{ true };
	float t{};
	float speed{0.5f};
};

glm::vec2 screenSize{ 800, 800 };
glm::vec2 mousePos;

size_t rectCount{};
int maxRects{};

std::array<Rect, 10> rects{};
std::vector<Rect> effects{};

double lastTime{};

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

	GLFWwindow* window = glfwCreateWindow(800, 600, "practice 2", nullptr, nullptr);
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

	InitRects();

	while (!glfwWindowShouldClose(window))
	{
		UpdateEffects();
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
	if (rectCount >= maxRects) return;
	auto& rect = rects[rectCount];
	rect.pos = mousePos;
	rect.size = glm::vec2(rectSize);
	rect.color = glm::vec3(colorUrd(dre), colorUrd(dre), colorUrd(dre));
	++rectCount;
}

void SplitRect(Rect& rect)
{
	auto halfSize = rect.size * 0.5f;
	std::vector<Rect> splitedRects{};

	auto splitedRectSize = halfSize * 0.5f;

	for(int i = 0; i < 4; ++i)
	{
		Rect splitedRect;
		splitedRect.color = rect.color;
		splitedRect.pos = rect.pos;
		splitedRect.size = halfSize;
		splitedRects.push_back(splitedRect);
	}

	rect.visible = false;

	std::uniform_int_distribution splitModeDist{ static_cast<int>(SPLIT_NORMAL), static_cast<int>(SPLIT_OCT) };
	int splitMode{ splitModeDist(dre) };

	switch (splitMode)
	{
	case SPLIT_NORMAL:
		for (int i = 0; i < 4; ++i)
		{
			splitedRects[i].dir = dirs[i];
		}
		break;
	case SPLIT_DIAG:
		for (int i = 0; i < 4; ++i)
		{
			splitedRects[i].dir = dirs[4 + i];
		}
		break;
	case SPLIT_ALONG:
	{
		std::uniform_int_distribution dirSel{ 0,7 };
		auto dirAlong{ dirs[dirSel(dre)] };
		splitedRects[0].pos = rect.pos - glm::vec2(splitedRectSize.x, -splitedRectSize.y) * 0.5f;
		splitedRects[1].pos = rect.pos - glm::vec2(-splitedRectSize.x, -splitedRectSize.y) * 0.5f;
		splitedRects[2].pos = rect.pos - glm::vec2(splitedRectSize.x, splitedRectSize.y) * 0.5f;
		splitedRects[3].pos = rect.pos - glm::vec2(-splitedRectSize.x, splitedRectSize.y) * 0.5f;

		for (int i = 0; i < 4; ++i)
		{
			splitedRects[i].dir = dirAlong;
		}
		break;
	}
	case SPLIT_OCT:
	{
		for (int i = 0; i < 4; ++i)
		{
			Rect splitedRect;
			splitedRect.color = rect.color;
			splitedRect.pos = rect.pos;
			splitedRect.size = halfSize;
			splitedRects.push_back(splitedRect);
		}

		for (int i = 0; i < 8; ++i)
		{
			splitedRects[i].dir = dirs[i];
		}
		break;
	}
	}

	effects.append_range(splitedRects);
}

void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT)
	{
		if (action == GLFW_PRESS)
		{
			for (int i = rectCount - 1; i >= 0; --i)
			{
				auto& rect = rects[i];
				if (!rect.visible) continue; 
				auto halfSize = rect.size * 0.5f;
				if (mousePos.x >= rect.pos.x - halfSize.x && mousePos.x <= rect.pos.x + halfSize.x &&
					mousePos.y >= rect.pos.y - halfSize.y && mousePos.y <= rect.pos.y + halfSize.y)
				{
					std::println("Split Rect! [Idx : {}]", i);
					SplitRect(rect);
					break;
				}
			}
		}
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
		case GLFW_KEY_R:
			InitRects();
			break;
		case GLFW_KEY_Q:
			glfwSetWindowShouldClose(window, true);
			break;
		default:
			break;
		}
	}
}

void UpdateEffects()
{
	double currentTime{ glfwGetTime() };
	float deltaTime = static_cast<float>(currentTime - lastTime);
	lastTime = currentTime;

	for (auto& effect : effects)
	{
		effect.t += deltaTime;
		effect.pos += effect.dir * effect.speed * deltaTime;
	}
	std::erase_if(effects, [](const Rect& e) {return e.t > 1.0f;});
}

void InitRects()
{
	std::uniform_real_distribution rectSpawnSize{ 0.05f, 0.1f };
	std::uniform_int_distribution rectSpawnCount{ 5,10 };
	rectSize =  rectSpawnSize(dre) * 4.f;
	std::uniform_real_distribution rectSpawnPos{ -1.f + rectSize, 1.f - rectSize };
	rectCount = rectSpawnCount(dre);

	for (int i = 0; i < rectCount; ++i)
	{
		auto& rect = rects[i];
		rect.color = glm::vec3(colorUrd(dre), colorUrd(dre), colorUrd(dre));
		rect.size = glm::vec2(rectSize , rectSize);
		rect.pos = glm::vec2(rectSpawnPos(dre), rectSpawnPos(dre));
		rect.visible = true;
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
		if (!rect.visible) continue;
		auto halfSize = rect.size * 0.5f;
		glColor3f(rect.color.r, rect.color.g, rect.color.b);
		glRectf(rect.pos.x - halfSize.x, rect.pos.y - halfSize.y, rect.pos.x + halfSize.x, rect.pos.y + halfSize.y);
	}

	for (const auto& effect : effects)
	{
		if (!effect.visible) continue;

		auto halfSize = (effect.t * glm::vec2(0.01f) + (1.f - effect.t) * effect.size) * 0.5f;

		auto renderColor = effect.t * glm::vec3(0.f) + (1.f - effect.t) * effect.color;
		glColor3f(renderColor.r, renderColor.g, renderColor.b);
		glRectf(effect.pos.x - halfSize.x, effect.pos.y - halfSize.y, effect.pos.x + halfSize.x, effect.pos.y + halfSize.y);
	}
}