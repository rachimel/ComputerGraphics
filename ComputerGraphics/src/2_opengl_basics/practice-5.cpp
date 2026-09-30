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
std::uniform_real_distribution<float> colorUrd{ 0.0f, 1.0f };

bool eraseRect{ false };

float rectSize;
std::uniform_real_distribution rectSpawnSize{ 0.05f, 0.1f };

void FrameBufferSizeCallback(GLFWwindow* window, int width, int height);
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void CursorPosCallback(GLFWwindow* window, double xPosIn, double yPosIn);
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

void InitRects();
void DrawScene();

struct Rect
{
	glm::vec2 pos{};
	glm::vec2 size{};
	glm::vec3 color{};

	bool visible{ true };
};

glm::vec2 screenSize{ 800, 800 };
glm::vec2 mousePos;

size_t rectCount{};
int maxRects{};

std::array<Rect, 50> rects{};

Rect eraser{};
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

	GLFWwindow* window = glfwCreateWindow(800, 600, "practice 5", nullptr, nullptr);
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
	eraser.size -= glm::vec2(rectSize) * 0.1f;
	++rectCount;
}

void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT)
	{
		if (action == GLFW_PRESS)
		{
			eraser.color = glm::vec3(0.f);
			eraseRect = true;
		}
		else if (action == GLFW_RELEASE)
		{
			eraseRect = false;
			for (int i = 0; i < rectCount; ++i)
			{
				rects[i].visible = true;
			}
			eraser.visible = false;
			eraser.size = glm::vec2(rectSize) * 2.f;
		}
	}
	else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
	{
		InsertRect();
	}
}

void CursorPosCallback(GLFWwindow* window, double xPosIn, double yPosIn)
{
	mousePos.x = static_cast<float>(xPosIn) / screenSize.x * 2.0f - 1.0f;
	mousePos.y = 1.0f - static_cast<float>(yPosIn) / screenSize.y * 2.0f;

	if (eraseRect)
	{
		eraser.visible = true;
		eraser.pos = mousePos;

		for (int i = 0; i < rectCount; ++i)
		{
			auto& rect = rects[i];
			if (!rect.visible) continue;

			// assume that all rects are square.
			float halfSize = rect.size.x * 0.5f;
			float eraserHalfSize = eraser.size.x * 0.5f;
			bool xCollide = (rect.pos.x + halfSize > eraser.pos.x - eraserHalfSize && eraser.pos.x + eraserHalfSize > rect.pos.x - halfSize);
			bool yCollide = (rect.pos.y + halfSize > eraser.pos.y - eraserHalfSize && eraser.pos.y + eraserHalfSize > rect.pos.y - halfSize);
			if (xCollide && yCollide)
			{
				rect.visible = false;
				eraser.color = rect.color;
				eraser.size += glm::vec2(1.0f) * (1.0f / (float)rectCount);
			}
		}
	}
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

void InitRects()
{
	std::uniform_real_distribution rectSpawnSize{ 0.05f, 0.1f };
	std::uniform_int_distribution rectSpawnCount{ 20,40 };
	rectSize =  rectSpawnSize(dre) ;
	std::uniform_real_distribution rectSpawnPos{ -1.f + rectSize, 1.f - rectSize };
	rectCount = rectSpawnCount(dre);
	maxRects = rectCount + 10;

	for (int i = 0; i < rectCount; ++i)
	{
		auto& rect = rects[i];
		rect.color = glm::vec3(colorUrd(dre), colorUrd(dre), colorUrd(dre));
		rect.size = glm::vec2(rectSize , rectSize);
		rect.pos = glm::vec2(rectSpawnPos(dre), rectSpawnPos(dre));
	}

	eraser.visible = false;
	eraser.size = glm::vec2(rectSize) * 2.f;
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

	if(eraser.visible)
	{
		auto halfSize = eraser.size * 0.5f;
		glColor3f(eraser.color.r, eraser.color.g, eraser.color.b);
		glRectf(eraser.pos.x - halfSize.x, eraser.pos.y - halfSize.y, eraser.pos.x + halfSize.x, eraser.pos.y + halfSize.y);
	}
}