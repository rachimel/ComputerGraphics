// Standard C++ Libraries
#include <iostream>
#include <print>
#include <array>

#include <random>
// OpenGL Libraries
#include <GL/glew.h>
#include <glfw/glfw3.h>
#include <glm/glm.hpp>

std::random_device rd;
std::default_random_engine dre{ rd() };
std::uniform_real_distribution<float> colorUrd{0.0f, 1.0f};
void FrameBufferSizeCallback(GLFWwindow* window, int width, int height);
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void CursorPosCallback(GLFWwindow* window, double xPosIn, double yPosIn);
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

void DrawScene();

struct Rect
{
	float size;
	glm::vec3 color;
};

glm::vec2 screenSize{ 800, 600 };
glm::vec2 mousePos;

std::array<size_t, 4> rectCounts{};
std::array<std::array<Rect, 5>, 4> rects{};
std::array<glm::vec3, 4> quadrantColors{
	glm::vec3(0.24f, 0.23f, 0.54f),
	glm::vec3(0.74f, 0.45f, 0.21f),
	glm::vec3(0.13f, 0.65f, 0.23f),
	glm::vec3(0.96f, 0.97f, 0.95f)
};
std::array<glm::vec2, 4> quadrantPositions{
	glm::vec2(-1.0f, 0.0f),
	glm::vec2(0.0f, 0.0f),
	glm::vec2(-1.0f, -1.0f),
	glm::vec2(0.0f, -1.0f)
};

int selectedQuadrant = -1;
int selectedRect = -1;

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

void SelectRect()
{
	for (int i = 0; i < 4; ++i)
	{
		const auto quadrantCenter = quadrantPositions[i] + glm::vec2(0.5f, 0.5f);
		for (int j = static_cast<int>(rectCounts[i]) - 1; j >= 0; --j)
		{
			auto& rect = rects[i][j];
			float halfSize = rect.size * 0.5f;

			if (mousePos.x >= quadrantCenter.x - halfSize && mousePos.x <= quadrantCenter.x + halfSize &&
				mousePos.y >= quadrantCenter.y - halfSize && mousePos.y <= quadrantCenter.y + halfSize)
			{
				selectedQuadrant = i;
				selectedRect = j;
				std::println("Mouse Hit! : Quadrant {}, Rect Idx : {}", selectedQuadrant + 1, selectedRect);
				return;
			}
		}
	}

	selectedQuadrant = -1;
	selectedRect = -1;
}

void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
	{
		SelectRect();
	}
}

void CursorPosCallback(GLFWwindow* window, double xPosIn, double yPosIn)
{
	mousePos.x = static_cast<float>(xPosIn) / screenSize.x * 2.0f - 1.0f;
	mousePos.y = 1.0f - static_cast<float>(yPosIn) / screenSize.y * 2.0f;
}

void InsertRect(int idx)
{
	if (rectCounts[idx] >= 5) return;
	auto& rect = rects[idx][rectCounts[idx]];
	rect.size = (1.0f - 0.1f * (rectCounts[idx] + 1));
	rect.color = rect.size * glm::vec3(quadrantColors[idx]);
	++rectCounts[idx];
}

void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (action == GLFW_PRESS)
	{
		switch (key)
		{
		case GLFW_KEY_1:
			InsertRect(0);
			break;
		case GLFW_KEY_2:
			InsertRect(1);
			break;
		case GLFW_KEY_3:
			InsertRect(2);
			break;
		case GLFW_KEY_4:
			InsertRect(3);
			break;
		case GLFW_KEY_EQUAL:
			if (mods & GLFW_MOD_SHIFT) 
			{
				if (selectedQuadrant != -1 && selectedRect != -1)
				{
					if(rects[selectedQuadrant][selectedRect].size <= 1.0f)
					{
						rects[selectedQuadrant][selectedRect].size += 0.1f;
					}
				}
			}
			break;
		case GLFW_KEY_MINUS:
			if (selectedQuadrant != -1 && selectedRect != -1)
			{
				if(rects[selectedQuadrant][selectedRect].size > 0.2f)
				{
					rects[selectedQuadrant][selectedRect].size -= 0.1f;
				}
			}
			break;
		case GLFW_KEY_C:
			if (selectedQuadrant != -1 && selectedRect != -1)
			{
				rects[selectedQuadrant][selectedRect].color = glm::vec3(colorUrd(dre), colorUrd(dre), colorUrd(dre));
			}
			break;
		case GLFW_KEY_R:
			selectedQuadrant = -1;
			selectedRect = -1;
			for (auto& quadrant : rects)
			{
				quadrant = std::array<Rect, 5>();
			}
			rectCounts = std::array<size_t, 4>();
			for (int i = 0; i < 4; ++i)
			{
				quadrantColors[i] = glm::vec3(colorUrd(dre), colorUrd(dre), colorUrd(dre));
			}
			break;
		case GLFW_KEY_Q:
			glfwSetWindowShouldClose(window, true);
			break;
		default:
			break;
		}
	}
}

void DrawScene()
{
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	// Draw Quadrants
	for (int i = 0; i < 4; ++i)
	{
		glColor3f(quadrantColors[i].r, quadrantColors[i].g, quadrantColors[i].b);
		auto& originPos = quadrantPositions[i];
		glRectf(originPos.x, originPos.y ,originPos.x + 1.0f, originPos.y + 1.0f);
	}

	// Draw Rects
	for (int i = 0; i < 4; ++i)
	{
		const auto& quadrantCenter = quadrantPositions[i] + glm::vec2(0.5f, 0.5f);
		for (size_t j = 0; j < rectCounts[i]; ++j)
		{
			auto& rect = rects[i][j];
			float halfSize = rect.size * 0.5f;
			if (i == selectedQuadrant && j == selectedRect)
			{
				// draw selected
				glColor3f(1.0f, 0.0f, 0.0f);
				glRectf(quadrantCenter.x - halfSize - 0.01f, quadrantCenter.y - halfSize - 0.01f, quadrantCenter.x + halfSize + 0.01f, quadrantCenter.y + halfSize + 0.01f);
			}
			glColor3f(rect.color.r, rect.color.g, rect.color.b);
			glRectf(quadrantCenter.x - halfSize, quadrantCenter.y - halfSize, quadrantCenter.x + halfSize, quadrantCenter.y + halfSize);
		}
	}
}