#include <iostream>
#include <print>
#include <random>

#include <GL/glew.h>
#include <glfw/glfw3.h>
#include <glm/glm.hpp>

#include <Shader.h>

std::random_device rd;
std::default_random_engine dre{ rd() };
std::uniform_real_distribution<float> urd{ 0.0f, 1.0f };
glm::vec3 clearColor{ 1.0f };


void FrameBufferSizeCallback(GLFWwindow* window, int width, int height);

void FrameBufferSizeCallback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

double lastTime{ 0.0 };
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

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	float vertex[]{
		-3.0f, 3.0f, 0.0f,
		-3.0f,-3.0f, 0.0f,
		3.0f, -3.0f, 0.0f,
		3.0f, 3.0f, 0.0f
	};

	unsigned int indices[]{
		0,1,2,
		2,3,0
	};
	unsigned int VAO, VBO, EBO;
	glCreateVertexArrays(1, &VAO);
	glCreateBuffers(1, &VBO);
	glCreateBuffers(1, &EBO);

	glNamedBufferData(VBO, sizeof(vertex), vertex, GL_STATIC_DRAW);
	glNamedBufferData(EBO, sizeof(indices), indices, GL_STATIC_DRAW);

	glVertexArrayVertexBuffer(VAO, 0, VBO, 0, sizeof(float) * 3);
	glVertexArrayAttribFormat(VAO, 0, 3, GL_FLOAT, GL_FALSE, 0);
	glVertexArrayElementBuffer(VAO, EBO);
	glVertexArrayAttribBinding(VAO, 0, 0);
	glEnableVertexArrayAttrib(VAO, 0);

	Shader shader{ "shaders\\99_labs\\rippleColor.vs", "shaders\\99_labs\\ripple.fs" };
	float t{ 0.0f };
	while (!glfwWindowShouldClose(window))
	{
		float currentTime = static_cast<float>(glfwGetTime());
		float deltaTime = currentTime - lastTime;
		lastTime = currentTime;

		float x = 0.5f * cos(t);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		glBindVertexArray(VAO);
		shader.Bind();
		glPointSize(5.0f);
		t += deltaTime;
		shader.SetUniform("a_Color", glm::vec4(0.8f, 0.4f, 0.8f, 0.8f));
		shader.SetUniform("t", t);
		shader.SetUniform("a_Translation", glm::vec2(-x, 0.0f));
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (const void*)0);

		shader.SetUniform("a_Color", glm::vec4(0.4f, 0.8f, 0.4f, 0.5f));
		shader.SetUniform("a_Translation", glm::vec2(x, 0.0f));
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (const void*)0);
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwDestroyWindow(window);
	glfwTerminate();
}

