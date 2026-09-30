#include <iostream>
#include <print>

#include <random>
#include <optional>
#include <array>

#include <gl/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <Shader.h>

void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void CursorPosCallback(GLFWwindow* window, double xPosIn, double yPosIn);
void FrameBufferSizeCallback(GLFWwindow* window, int width, int height);

void DrawScene();

enum PrimitiveType
{
	PT_POINT,
	PT_LINE,
	PT_TRIANGLE,
	PT_QUAD
};

struct Primitive
{
	glm::vec2 pos{};
	glm::vec2 size{};
	glm::vec3 color{};

	PrimitiveType type{};
	bool selected{ false };
};

bool moveAll{ false };
std::random_device rd;
std::default_random_engine dre{ rd() };
std::uniform_real_distribution colorUrd{ 0.0f, 1.0f };

std::array<Primitive, 50> primitives{};
int primitiveCount{ 0 };

glm::mat4 orthographic;
glm::vec2 screenSize{};
glm::vec2 mousePos{};

std::optional<Shader> shader;
unsigned int VAO{}, VBO{}, EBO{};
int main()
{
	if (!glfwInit())
	{
		std::println(std::cerr, "[GLFW] : Failed to initialize GLFW!");
		return -1;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(800, 800, "practice 7", nullptr, nullptr);
	if (!window)
	{
		std::println(std::cerr,"[GLFW] : Failed to create GLFW window!");
		glfwTerminate();
		return -1;
	}

	glfwMakeContextCurrent(window);
	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK)
	{
		std::println(std::cerr, "[GLEW] : Failed to initialize GLEW!");
		return -1;
	}

	glfwSetKeyCallback(window, KeyCallback);
	glfwSetMouseButtonCallback(window, MouseButtonCallback);
	glfwSetCursorPosCallback(window, CursorPosCallback);
	glfwSetFramebufferSizeCallback(window, FrameBufferSizeCallback);

	screenSize = glm::vec2(800.0f, 800.0f);
	glViewport(0, 0, 800, 800);
	orthographic = glm::ortho(0.0f, 800.0f, 800.0f, 0.0f);

	shader.emplace("shaders\\1_opengl_basics\\orthoStaticProjection.vs", "shaders\\1_opengl_basics\\uniformColor.fs");

	float vertices[] = {
		// Point
		0.0f, 0.0f, 0.0f,
		// Line
		-0.5f, 0.0f, 0.0f,
		0.5f, 0.0f, 0.0f,
		// Triangle
		-0.5f, 0.5f, 0.0f,
		0.5f, 0.5f, 0.0f,
		0.0f, -0.5f, 0.0f,
		// Quad
		-0.5f, 0.5f, 0.0f,
		-0.5f, -0.5f, 0.0f,
		0.5f, -0.5f, 0.0f,
		0.5f, 0.5f, 0.0f,
	};

	unsigned int indices[] = {
		6, 7, 8,
		8, 9, 6
	};

	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, (const void*)0);
	glEnableVertexAttribArray(0);

	while (!glfwWindowShouldClose(window))
	{
		DrawScene();

		glfwSwapBuffers(window);
		glfwPollEvents();
	}
	glfwDestroyWindow(window);
	glfwTerminate();
}

void CreatePrimitive(PrimitiveType primitiveType)
{
	if (primitiveCount >= 50) return;
	std::uniform_real_distribution spawnX{ 10.0f, screenSize.x - 10.0f };
	std::uniform_real_distribution spawnY{ 10.0f, screenSize.y - 10.0f };

	Primitive newPrimitive{};
	newPrimitive.color = glm::vec3(colorUrd(dre), colorUrd(dre), colorUrd(dre));
	newPrimitive.pos = glm::vec2(spawnX(dre), spawnY(dre));
	newPrimitive.size = glm::vec2(40.0f);
	newPrimitive.type = primitiveType;

	primitives[primitiveCount] = newPrimitive;
	primitiveCount++;
}

bool TestScreenPrimitiveIntersection(const Primitive& primitive, const glm::vec2& nextPos)
{
	auto halfSize = primitive.size * 0.5f;
	switch (primitive.type)
	{
	case PT_POINT:
		return (nextPos.x - 5.0f <= 0.0f || nextPos.x + 5.0f >= screenSize.x
			|| nextPos.y - 5.0f <= 0.0f || nextPos.y + 5.0f >= screenSize.y);
	case PT_LINE:
		return (nextPos.x - halfSize.x <= 0.0f || nextPos.x + halfSize.x >= screenSize.x
			|| nextPos.y - 2.5f <= 0.0f || nextPos.y + 2.5f >= screenSize.y);
	case PT_TRIANGLE:
	{
		glm::vec2 A = nextPos + glm::vec2(-0.5f, 0.5f) * primitive.size;
		glm::vec2 B = nextPos + glm::vec2(0.5f, 0.5f) * primitive.size;
		glm::vec2 C = nextPos + glm::vec2(0.0f, -0.5f) * primitive.size;

		auto outside = [&](const glm::vec2& p)
			{
				return p.x <= 0.0f || p.x >= screenSize.x
					|| p.y <= 0.0f || p.y >= screenSize.y;
			};

		return outside(A) || outside(B) || outside(C);
	}
	case PT_QUAD:
		return (nextPos.x - halfSize.x <= 0.0f || nextPos.x + halfSize.x >= screenSize.x
			|| nextPos.y - halfSize.y <= 0.0f || nextPos.y + halfSize.y >= screenSize.y);
	}
}
void MovePrimitive(Primitive& primitive, const glm::vec2& direction)
{
	auto nextPos = primitive.pos + direction;
	if (TestScreenPrimitiveIntersection(primitive, nextPos))
	{
		return;
	}
	primitive.pos = nextPos;
}

void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (action == GLFW_PRESS)
	{
		if (moveAll)
		{
			for (auto& primitive : primitives)
			{
				primitive.selected = false;
			}
			moveAll = false;
		}
		switch (key)
		{
		case GLFW_KEY_W:
			for (auto& primitive : primitives)
			{
				if (primitive.selected)
					MovePrimitive(primitive, glm::vec2(0.0f, -5.0f));
			}
			break;
		case GLFW_KEY_A:
			for (auto& primitive : primitives)
			{
				if (primitive.selected)
					MovePrimitive(primitive, glm::vec2(-5.0f, 0.0f));
			}
			break;
		case GLFW_KEY_S:
			for (auto& primitive : primitives)
			{
				if (primitive.selected)
					MovePrimitive(primitive, glm::vec2(0.0f, 5.0f));
			}
			break;
		case GLFW_KEY_D:
			for (auto& primitive : primitives)
			{
				if (primitive.selected)
					MovePrimitive(primitive, glm::vec2(5.0f, 0.0f));
			}
			break;
		case GLFW_KEY_I:
			for (auto& primitive : primitives)
			{
				if (primitive.selected)
					MovePrimitive(primitive, glm::vec2(-5.0f, -5.0f));
			}
			break;
		case GLFW_KEY_J:
			for (auto& primitive : primitives)
			{
				if (primitive.selected)
					MovePrimitive(primitive, glm::vec2(5.0f, -5.0f));
			}
			break;
		case GLFW_KEY_K:
			for (auto& primitive : primitives)
			{
				if (primitive.selected)
					MovePrimitive(primitive, glm::vec2(-5.0f, 5.0f));
			}
			break;
		case GLFW_KEY_L:
			for (auto& primitive : primitives)
			{
				if (primitive.selected)
					MovePrimitive(primitive, glm::vec2(5.0f, 5.0f));
			}
			break;
		case GLFW_KEY_1:
			moveAll = true;
			for (auto& primitive : primitives)
			{
				primitive.selected = true;
				MovePrimitive(primitive, glm::vec2(-5.0f, 0.0f));
			}
			break;
		case GLFW_KEY_2:
			moveAll = true;
			for (auto& primitive : primitives)
			{
				primitive.selected = true;
				MovePrimitive(primitive, glm::vec2(5.0f, 0.0f));
			}
			break;
		case GLFW_KEY_3:
			moveAll = true;
			for (auto& primitive : primitives)
			{
				primitive.selected = true;
				MovePrimitive(primitive, glm::vec2(0.0f, -5.0f));
			}
			break;
		case GLFW_KEY_4:
			moveAll = true;
			for (auto& primitive : primitives)
			{
				primitive.selected = true;
				MovePrimitive(primitive, glm::vec2(0.0f, 5.0f));
			}
			break;
		case GLFW_KEY_P:
			CreatePrimitive(PT_POINT);
			break;
		case GLFW_KEY_E:
			CreatePrimitive(PT_LINE);
			break;
		case GLFW_KEY_T:
			CreatePrimitive(PT_TRIANGLE);
			break;
		case GLFW_KEY_R:
			CreatePrimitive(PT_QUAD);
			break;
		case GLFW_KEY_C:
			primitiveCount = 0;
			break;
		case GLFW_KEY_Q:
			glfwSetWindowShouldClose(window, true);
			break;
		}
	}
}

float kross2d(const glm::vec2& a, const glm::vec2& b)
{
	return a.x * b.y - a.y * b.x;
}
bool TestPointPrimitiveIntersection(const glm::vec2& point, const Primitive& primitive)
{
	auto halfSize = primitive.size * 0.5f;
	switch (primitive.type)
	{
	case PT_POINT:
		return (point.x >= primitive.pos.x - 5.0f && point.x <= primitive.pos.x + 5.0f 
			&& point.y >= primitive.pos.y - 5.0f && point.y <= primitive.pos.y + 5.0f);
	case PT_LINE:
		return (point.x >= primitive.pos.x - halfSize.x && point.x <= primitive.pos.x + halfSize.x
			&& point.y >= primitive.pos.y - 2.5f && point.y <= primitive.pos.y + 2.5f);
	case PT_TRIANGLE:
	{
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(primitive.pos, 0.0f));
		model = glm::scale(model, glm::vec3(primitive.size, 1.0f));
		glm::vec4 localPoint4 = glm::inverse(model) * glm::vec4(mousePos, 0.0f, 1.0f);
		glm::vec2 P = glm::vec2(localPoint4);

		glm::vec2 A(-0.5f, 0.5f);
		glm::vec2 B(0.5f, 0.5f);
		glm::vec2 C(0.0f, -0.5f);

		float c1 = kross2d(B - A, P - A);
		float c2 = kross2d(C - B, P - B);
		float c3 = kross2d(A - C, P - C);

		bool hasNegative{ c1 < 0 && c2 < 0 && c3 < 0 };
		bool hasPositive{ c1 > 0 && c2 > 0 && c3 > 0 };
		return hasNegative || hasPositive;
	}
	case PT_QUAD:
		return (point.x >= primitive.pos.x - halfSize.x && point.x <= primitive.pos.x + halfSize.x
			&& point.y >= primitive.pos.y - halfSize.y && point.y <= primitive.pos.y + halfSize.y);
	}
	return false;
}

void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
	{
		if (moveAll) moveAll = false;
		for (auto& primitive : primitives)
			primitive.selected = false;
		for (int i = primitiveCount - 1; i >= 0; --i)
		{
			if (TestPointPrimitiveIntersection(mousePos, primitives[i]))
			{
				std::println("[Intersection] : Point to Primitive");
				primitives[i].selected = true;
				break;
			}
		}
	}
}

void CursorPosCallback(GLFWwindow* window, double xPosIn, double yPosIn)
{
	mousePos = glm::vec2(static_cast<float>(xPosIn), static_cast<float>(yPosIn));
}
void FrameBufferSizeCallback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
	screenSize = glm::vec2(static_cast<float>(width), static_cast<float>(height));
	orthographic = glm::ortho(0.0f, screenSize.x, screenSize.y, 0.0f);
}

void DrawPrimtive(const Primitive& primitive, float scale = 1.0f)
{
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(primitive.pos, 0.0f));
	model = glm::scale(model, glm::vec3(primitive.size * scale, 1.0f));
	shader->SetUniform("model", model);
	shader->SetUniform("a_Color", primitive.color);

	switch (primitive.type)
	{
	case PT_POINT:
		glPointSize(10.f * scale);
		glDrawArrays(GL_POINTS, 0, 1);
		break;
	case PT_LINE:
		glLineWidth(5.f * scale);
		glDrawArrays(GL_LINES, 1, 2);
		break;
	case PT_TRIANGLE:
		glDrawArrays(GL_TRIANGLES, 3, 3);
		break;
	case PT_QUAD:
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (const void*)(0));
		break;
	}
}
void DrawScene()
{
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	glBindVertexArray(VAO);
	shader->Bind();
	shader->SetUniform("projection", orthographic);
	for (int i = 0; i < primitiveCount; ++i)
	{
		if (primitives[i].selected)
		{
			auto oldColor = primitives[i].color;
			primitives[i].color = glm::vec3(0.0f, 0.0f, 0.0f);
			DrawPrimtive(primitives[i], 1.2f);
			primitives[i].color = oldColor;
		}
		DrawPrimtive(primitives[i]);
	}
	glBindVertexArray(0);
}