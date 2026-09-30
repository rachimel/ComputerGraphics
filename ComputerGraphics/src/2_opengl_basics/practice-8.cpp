#include <print>
#include <array>
#include <random>

#include <gl/glew.h>
#include <GLFW/glfw3.h>
#include <Shader.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define EPSILON 0.0001f

float vertices[]{
	// Triangle
	-0.5f,  0.5f, 0.0f,
	 0.5f,  0.5f, 0.0f,
	 0.0f, -0.5f, 0.0f,
	// Horizontial Line
	-0.5f,  0.0f, 0.0f,
	 0.5f,  0.0f, 0.0f,
};


struct Triangle
{
	glm::vec2 pos{};
	glm::vec2 size{};
	float scaleFactor{ 1.0f };
	glm::vec3 color{};
};
// 
std::random_device rd;
std::default_random_engine dre{ rd() };
std::uniform_real_distribution widthRange{ 40.0f, 50.0f };
std::uniform_real_distribution colorRange{ 0.0f, 1.0f };

unsigned int VAO, VBO;
glm::vec2 screenSize{800.0f, 800.0f};
glm::mat4 projection{};

GLenum drawingMode{ GL_TRIANGLES };
std::array<Triangle, 4> triangles{};
constexpr float maxSize{ 2.0f };
constexpr float minSize{ 0.5f };

glm::vec2 mousePos{};
bool rightMousePressed{ false };
int selectedTriangleIndex{ -1 };

void FrameBufferSizeCallback(GLFWwindow* window, int width, int height);
void KeyCallback(GLFWwindow*, int, int, int, int);
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void CursorPosCallback(GLFWwindow*, double, double);

void PollInputs(GLFWwindow* window);

void InitVAO();
void InitTriangles();
void PlaceTriangle(int index, const glm::vec2& xRange, const glm::vec2& yRange);
void DrawScene(Shader& shader);

// utils
bool IntersectTrianglePoint(const Triangle& triangle, const glm::vec2& point);
float PerpDot(const glm::vec2& a, const glm::vec2& b);

int main()
{
	if (!glfwInit())
	{
		std::println("[GLFW] : Failed to initialize GLFW!");
		return -1;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(static_cast<int>(screenSize.x), static_cast<int>(screenSize.y), "practice 8 ", nullptr, nullptr);
	if (!window)
	{
		std::println("[GLFW] : Failed to create window!");
		glfwTerminate();
		return -1;
	}

	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, FrameBufferSizeCallback);
	glfwSetKeyCallback(window, KeyCallback);
	glfwSetMouseButtonCallback(window, MouseButtonCallback);
	glfwSetCursorPosCallback(window, CursorPosCallback);

	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK)
	{
		std::println("[GLEW] : Failed to initialize GLEW!");
		return -1;
	}

	glViewport(0, 0, static_cast<int>(screenSize.x), static_cast<int>(screenSize.y));
	projection = glm::ortho(0.0f, screenSize.x, screenSize.y, 0.0f);

	InitVAO();
	InitTriangles();

	Shader shader{ "shaders\\1_opengl_basics\\orthoStaticProjection.vs", "shaders\\1_opengl_basics\\uniformColor.fs" };

	while (!glfwWindowShouldClose(window))
	{
		shader.Bind();
		DrawScene(shader);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}
}

void FrameBufferSizeCallback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
	screenSize = glm::vec2(static_cast<float>(width), static_cast<float>(height));
	projection = glm::ortho(0.0f, screenSize.x, screenSize.y, 0.0f);
}

void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (action == GLFW_PRESS)
	{
		switch (key)
		{
		case GLFW_KEY_A:
			if (drawingMode == GL_LINE_LOOP) drawingMode = GL_TRIANGLES;
			break;
		case GLFW_KEY_B:
			if (drawingMode == GL_TRIANGLES) drawingMode = GL_LINE_LOOP;
			break;
		case GLFW_KEY_C:
			InitTriangles();
			break;
		case GLFW_KEY_Q:
			glfwSetWindowShouldClose(window, true);
			break;
		}
	}
}

void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	if (action == GLFW_PRESS)
	{
		switch (button)
		{
		case GLFW_MOUSE_BUTTON_LEFT:
		{
			float midX{ screenSize.x * 0.5f };
			float midY{ screenSize.y * 0.5f };
			if (mousePos.x < midX && mousePos.y < midY)
				PlaceTriangle(0, glm::vec2(0.0f, midX), glm::vec2(0.0f, midY));
			else if(mousePos.x > midX && mousePos.y < midY)
				PlaceTriangle(1, glm::vec2(midX, screenSize.x), glm::vec2(0.0f, midY));
			else if(mousePos.x < midX && mousePos.y > midY)
				PlaceTriangle(2, glm::vec2(0.0f, midX), glm::vec2(midY, screenSize.y));
			else if (mousePos.x > midX && mousePos.y > midY)
				PlaceTriangle(3, glm::vec2(midX, screenSize.x), glm::vec2(midY, screenSize.y));
			break;
		}
		case GLFW_MOUSE_BUTTON_RIGHT:
		{
			rightMousePressed = true;
			selectedTriangleIndex = -1;
			for (int i = 0; i < 4; ++i)
			{
				if (IntersectTrianglePoint(triangles[i], mousePos))
				{
					selectedTriangleIndex = i;
					break;
				}
			}
			break;
		}
		}
	}

	else if (action == GLFW_RELEASE && button == GLFW_MOUSE_BUTTON_RIGHT)
	{
		rightMousePressed = false;
	}
}
void CursorPosCallback(GLFWwindow* window, double xPosIn, double yPosIn)
{
	mousePos = glm::vec2(static_cast<float>(xPosIn), static_cast<float>(yPosIn));
	if (rightMousePressed && selectedTriangleIndex >= 0 && selectedTriangleIndex < 4)
	{
		auto& triangle = triangles[selectedTriangleIndex];
		glm::vec2 v{glm::abs(mousePos - triangle.pos)};

		float scaleX = v.x / (triangle.size.x * 0.5f);
		float scaleY = v.y / (triangle.size.y * 0.5f);
		float targetScale = std::max(scaleX, scaleY);

		targetScale = 1.0f + (targetScale - 1.0f) * 0.4f;
		triangle.scaleFactor = std::clamp(targetScale, minSize, maxSize);

	}
}

void InitVAO()
{
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, (const void*)0);
	glEnableVertexAttribArray(0);

	glBindVertexArray(0);
}

void InitTriangles()
{
	for (int i = 0; i < 4; ++i)
	{
		bool right = (i % 2) == 1; 
		bool bottom = (i / 2) == 1;

		float minX = right ? screenSize.x * 0.5f	: 0.0f;
		float maxX = right ? screenSize.x			: screenSize.x * 0.5f;
		float minY = bottom ? screenSize.y * 0.5f	: 0.0f;
		float maxY = bottom ? screenSize.y			: screenSize.y * 0.5f;

		float width{ widthRange(dre) };
		std::uniform_real_distribution heightRange{ width * 1.5f, width * 2.0f };
		float height{ heightRange(dre) };

		std::uniform_real_distribution xRange{ minX + width  * 0.5f, maxX - width * 0.5f };
		std::uniform_real_distribution yRange{ minY + height * 0.5f, maxY - height * 0.5f };

		triangles[i].pos = glm::vec2(xRange(dre), yRange(dre));
		triangles[i].size = glm::vec2(width, height);
		triangles[i].color = glm::vec3(colorRange(dre), colorRange(dre), colorRange(dre));
		triangles[i].scaleFactor = 1.0f;
	}
}

void PlaceTriangle(int index, const glm::vec2& xRange, const glm::vec2& yRange)
{
	float width{ widthRange(dre) };
	std::uniform_real_distribution heightRange{ width * 1.5f, width * 2.0f };
	float height{ heightRange(dre) };

	glm::vec2 placePos{ mousePos };

	placePos.x = glm::clamp(
		mousePos.x,
		xRange.x + width * 0.5f + EPSILON,
		xRange.y - width * 0.5f - EPSILON
	);

	placePos.y = glm::clamp(
		mousePos.y,
		yRange.x + height * 0.5f + EPSILON,
		yRange.y - height * 0.5f - EPSILON
	);

	triangles[index].pos = placePos;
	triangles[index].size = glm::vec2(width, height);
	triangles[index].color = glm::vec3(colorRange(dre), colorRange(dre), colorRange(dre));
	triangles[index].scaleFactor = 1.0f;
}

void DrawScene(Shader& shader)
{
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	glBindVertexArray(VAO);
	shader.SetUniform("projection", projection);

	glLineWidth(2.5f);
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(screenSize.x * 0.5f, screenSize.y * 0.5f, 0.0f));
	model = glm::scale(model, glm::vec3(screenSize.x, 1.0f, 1.0f));

	shader.SetUniform("model", model);
	shader.SetUniform("a_Color", glm::vec3(0.0f, 0.0f, 0.0f));
	glDrawArrays(GL_LINES, 3, 2);

	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(screenSize.x * 0.5f, screenSize.y * 0.5f, 0.0f));
	model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	model = glm::scale(model, glm::vec3(screenSize.x, 1.0f, 1.0f));

	shader.SetUniform("model", model);
	shader.SetUniform("a_Color", glm::vec3(0.0f, 0.0f, 0.0f));
	glDrawArrays(GL_LINES, 3, 2);

	for (const auto& triangle : triangles)
	{
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(triangle.pos, 0.0f));
		model = glm::scale(model, glm::vec3(triangle.size, 1.0f) * triangle.scaleFactor);

		shader.SetUniform("model", model);
		shader.SetUniform("a_Color", triangle.color);

		glDrawArrays(drawingMode, 0, 3);
	}
}

bool IntersectTrianglePoint(const Triangle& triangle, const glm::vec2& point)
{
	glm::mat4 model = glm::identity<glm::mat4>();
	model = glm::translate(model, glm::vec3(triangle.pos, 0.0f));
	model = glm::scale(model, glm::vec3(triangle.size, 1.0f) * triangle.scaleFactor);

	glm::vec4 P4 = glm::inverse(model) * glm::vec4(point, 0.0f, 1.0f);
	glm::vec2 P = glm::vec2(P4);

	glm::vec2 A{ -0.5f, 0.5f };
	glm::vec2 B{ 0.5f, 0.5f };
	glm::vec2 C{ 0.0f, -0.5f };

	float c1{ PerpDot(B - A, P - A) };
	float c2{ PerpDot(C - B, P - B) };
	float c3{ PerpDot(A - C, P - C) };

	return (c1 >= 0 && c2 >= 0 && c3 >= 0) ||
		(c1 < 0 && c2 < 0 && c3 < 0);
}

float PerpDot(const glm::vec2& a, const glm::vec2& b)
{
	return a.x * b.y - a.y * b.x;
}