#include <iostream>
#include <print>
#include <random>

#include <GL/glew.h>
#include <glfw/glfw3.h>
#include <glm/glm.hpp>

std::random_device rd;
std::default_random_engine dre{ rd()};
std::uniform_real_distribution<float> urd{ 0.0f, 1.0f };
glm::vec3 clearColor{1.0f};

bool keyboardHitlastTime{ false };
bool timer{ false };

void InputProcess(GLFWwindow* window);
void DrawScene();
void FrameBufferSizeCallback(GLFWwindow* window, int width, int height);

void FrameBufferSizeCallback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

double lastTime{0.0};
double accumulatedTime{ 0.0 };
int main()
{
	if (!glfwInit())
	{
		std::println(std::cerr, "GLFW 초기화 실패!");
		return -1;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(800, 600, "practice-1", nullptr, nullptr);
	if (!window)
	{
		std::println("Failed to create window!");
		glfwTerminate();
		return -1;
	}
	glfwSetFramebufferSizeCallback(window, FrameBufferSizeCallback);
	glfwMakeContextCurrent(window);

	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK)
	{
		std::println("failed to initialize GLEW");
		return -1;
	}
	while (!glfwWindowShouldClose(window))
	{
		InputProcess(window);
		DrawScene();
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwDestroyWindow(window);
	glfwTerminate();
}

void InputProcess(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS)
	{
		if(!keyboardHitlastTime)
		{
			clearColor = glm::vec3(0.0f, 1.0f, 1.0f);
			keyboardHitlastTime = true;
		}
	}
	else if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS)
	{
		if (!keyboardHitlastTime)
		{
			clearColor = glm::vec3(1.0f, 0.0f, 1.0f);
			keyboardHitlastTime = true;
		}
	}
	else if (glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS)
	{
		if (!keyboardHitlastTime)
		{
			clearColor = glm::vec3(1.0f, 1.0f, 0.0f);
			keyboardHitlastTime = true;
		}
	}
	else if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
	{
		if (!keyboardHitlastTime)
		{
			clearColor = glm::vec3(urd(dre), urd(dre), urd(dre));
			keyboardHitlastTime = true;
		}
	}
	else if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS)
	{
		if (!keyboardHitlastTime)
		{
			clearColor = glm::vec3(0.5f, 0.5f, 0.5f);
			keyboardHitlastTime = true;
		}
	}
	else if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS)
	{
		if (!keyboardHitlastTime)
		{
			clearColor = glm::vec3(0.0f, 0.0f, 0.0f);
			keyboardHitlastTime = true;
		}
	}
	else if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS)
	{
		if (!keyboardHitlastTime)
		{
			glfwSetTime(0.0);
			lastTime = 0.0;
			if (!timer) timer = true;
			keyboardHitlastTime = true;
		}
	}
	else if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
	{
		if (!keyboardHitlastTime)
		{
			if (timer) timer = false;
			keyboardHitlastTime = true;
		}
	}
	else if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(window, true);
	}
	else
	{
		keyboardHitlastTime = false;
	}
}

void DrawScene()
{
	if (timer)
	{
		double currentTime = glfwGetTime();
		double deltaTime = currentTime - lastTime;
		accumulatedTime += deltaTime;
		lastTime = currentTime;
		if (accumulatedTime >= 1.0)
		{
			clearColor = glm::vec3(urd(dre), urd(dre), urd(dre));
			accumulatedTime = 0.0;
		}
	}
	glClearColor(clearColor.r, clearColor.g, clearColor.b, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
}