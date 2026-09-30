#include <iostream>
#include <print>
#include <array>
#include <random>

#include <gl/glew.h>
#include <glfw/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <Shader.h>

std::random_device rd;
std::default_random_engine dre{ rd() };

enum PrimitiveType
{
	PT_QUAD,
	PT_RIGHTTRIANGLE,
	PT_TRIANGLE,
};
// [-screenSize / 2, screenSize / 2]
constexpr float lineWidth{ 2.0f };
constexpr float margin{ 2.5f };
struct Primitive
{
	glm::vec2 pos{};
	glm::vec3 color{};
	glm::vec2 scale{1.0f, 1.0f};
	float angle{0.0f};
	PrimitiveType type{};

	bool isFilled{ true };

	void Render()
	{
		GLenum drawingMode = (isFilled) ? GL_TRIANGLES : GL_LINE_LOOP ;
		glLineWidth(lineWidth);
		switch (type)
		{
		case PT_QUAD:
			glDrawElements(drawingMode, 6, GL_UNSIGNED_INT, (const void*)0);
			break;
		case PT_RIGHTTRIANGLE:
			glDrawElements(drawingMode, 3, GL_UNSIGNED_INT, (const void*)0);
			break;
		case PT_TRIANGLE:
			glDrawElements(drawingMode, 3, GL_UNSIGNED_INT, (const void*)(sizeof(unsigned int) * 3 * 2));
			break;
		}
	}

	glm::mat4 GetLocalModelMatrix() const
	{
		return
			glm::scale(glm::rotate(glm::translate(glm::identity<glm::mat4>(), glm::vec3(pos, 0.0f)), glm::radians(angle), glm::vec3(0.0f, 0.0f, 1.0f)),
				glm::vec3(scale, 1.0f));
	}
};

// [0, screenSize]
struct Shape
{
	glm::vec2 pos{};
	glm::vec2 size{};

	std::vector<Primitive> m_Primitives;

	glm::mat4 GetWorldModelMatrix() const
	{
		return glm::scale(glm::translate(glm::identity<glm::mat4>(), glm::vec3(pos, 0.0f)), glm::vec3(size, 1.0f));
	}
}; 
namespace
{
	float vertices[] = {
		-0.5f, -0.5f, 0.0f,
		-0.5f, 0.5f, 0.0f,
		0.5f, 0.5f, 0.0f,
		0.5f, -0.5f, 0.0f,
		// equilateral triangle's height must be sqrt(3) * 0.5f.
		0.0f, 0.5f - static_cast<float>(std::sqrt(3)) * 0.5f, 0.0f,

		// line
		-0.5f, 0.0f, 0.0f,
		0.5f, 0.0f, 0.0f
	};

	unsigned int indices[] = {
		// right triangles
		0, 1, 2,
		2, 3, 0,

		// equilateral triangle
		1,2 ,4,

		// Line
		5, 6
	};
}

// Rendering Data
namespace
{
	// Frames
	Shape m_RectShape{};
	Shape m_TriangleShape{};
	Shape m_Rect{};

	// Custom Shapes
	Shape m_CustomShape{};
	Shape m_CustomShape2{};

	std::vector<Shape> primitiveShapes{};
	std::vector<Shape> shapes{};
}

void FrameBufferSizeCallback(GLFWwindow* window, int width, int height);
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void CursorPosCallback(GLFWwindow* window, double xPosIn, double yPosIn);

void InitVAO();
void InitPlacements();

void DrawScene(Shader& shader);

bool MousePrimitiveTest(const glm::mat4& shapeWorldTransform, const Primitive& a);
bool IsPrimitiveIdentical(const Primitive& a, const Primitive& b);
bool PrimitivePrimitiveTest(const glm::mat4& aWorldTransform, const Primitive& a, const glm::mat4& bWorldTransform, const Primitive& b);
unsigned int VAO, VBO, EBO;
glm::vec2 screenSize{ 800.0f, 800.0f };
glm::mat4 projection;

// mouse
bool leftMouseButtonCliked{ false };
glm::vec2 mousePos{};
int pickedIndex{ -1 };

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

	GLFWwindow* window = glfwCreateWindow(static_cast<int>(screenSize.x), static_cast<int>(screenSize.y), "practice-10", nullptr, nullptr);
	if (!window)
	{
		std::print(std::cerr, "[GLFW] : Failed to create window!");
		glfwTerminate();
		return -1;
	}

	glfwMakeContextCurrent(window);
	
	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK)
	{
		std::print(std::cerr, "[GLEW] : Failed to initialize GLEW!");
		glfwDestroyWindow(window);
		glfwTerminate();

		return -1;
	}

	glViewport(0, 0, static_cast<int>(screenSize.x), static_cast<int>(screenSize.y));
	projection = glm::ortho(0.0f, screenSize.x, screenSize.y, 0.0f);
	// register callbacks
	glfwSetFramebufferSizeCallback(window, FrameBufferSizeCallback);
	glfwSetCursorPosCallback(window, CursorPosCallback);
	glfwSetKeyCallback(window, KeyCallback);
	glfwSetMouseButtonCallback(window, MouseButtonCallback);

	InitVAO();
	InitPlacements();

	Shader shader{ "shaders\\1_opengl_basics\\orthoStaticProjection.vs", "shaders\\1_opengl_basics\\uniformColor.fs" };
	while (!glfwWindowShouldClose(window))
	{
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
		case GLFW_KEY_R:
			InitPlacements();
			break;
		case GLFW_KEY_Q:
			glfwSetWindowShouldClose(window, true);
			break;
		}
	}
}

void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	if(action == GLFW_PRESS)
	{
		if (button == GLFW_MOUSE_BUTTON_LEFT)
		{
			leftMouseButtonCliked = true;
			size_t primitiveSize{ primitiveShapes.size() };
			pickedIndex = -1;
			for (int i = primitiveSize - 1; i >= 0; --i)
			{
				if (MousePrimitiveTest(primitiveShapes[i].GetWorldModelMatrix(), primitiveShapes[i].m_Primitives.front()))
				{
					pickedIndex = i;
					std::println("Picked Primitive #{}", i);
					break;
				}
			}
		}
	}
	else if (action == GLFW_RELEASE && button == GLFW_MOUSE_BUTTON_LEFT)
	{
		if(pickedIndex >= 0)
		{
			for (auto& shape : shapes)
			{
				for (auto& primitive : shape.m_Primitives)
				{
					if (primitive.isFilled) continue;
					auto& shapePrimitive = primitiveShapes[pickedIndex].m_Primitives.front();
					if (PrimitivePrimitiveTest(primitiveShapes[pickedIndex].GetWorldModelMatrix(), shapePrimitive,
						shape.GetWorldModelMatrix(), primitive))
					{
						std::println("SAT Detected");
						Primitive testPrimitive{ primitive };
						testPrimitive.scale = shape.size * testPrimitive.scale;
						Primitive testShapePrimitive{ shapePrimitive };
						testShapePrimitive.scale = primitiveShapes[pickedIndex].size * shapePrimitive.scale;
						if (IsPrimitiveIdentical(testShapePrimitive, testPrimitive))
						{
							primitive.color = shapePrimitive.color;
							primitive.isFilled = true;
							std::swap(primitiveShapes.back(), primitiveShapes[pickedIndex]);
							primitiveShapes.pop_back();
							pickedIndex = -1;
							return;
						}
					}
				}
			}
		}
		pickedIndex = -1;
	}

}

void CursorPosCallback(GLFWwindow* window, double xPosIn, double yPosIn)
{
	mousePos = glm::vec2(static_cast<float>(xPosIn), static_cast<float>(yPosIn));
	if (pickedIndex >= 0)
	{
		primitiveShapes[pickedIndex].pos = mousePos;
	}
}

void InitVAO()
{
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, (const void*)0);

	glBindVertexArray(0);
}

void InitPlacements()
{
	shapes.clear();
	primitiveShapes.clear();

	std::uniform_real_distribution colorRange{ 0.0f, 1.0f };
	std::uniform_real_distribution sizeRange{ 80.0f, 100.0f };

	// Shape 1
	{
		Shape shape{};
		float size{ sizeRange(dre) };
		shape.size = glm::vec2(size);

		Primitive primitive{};
		primitive.type = PT_QUAD;
		primitive.color = glm::vec3(colorRange(dre), colorRange(dre), colorRange(dre));
		primitive.angle = 0.0f;
		primitive.isFilled = false;
		primitive.pos = glm::vec2(-0.3f, -0.3f);
		primitive.scale = glm::vec2(0.5f);
		shape.m_Primitives.push_back(primitive);

		primitive.color = glm::vec3(colorRange(dre), colorRange(dre), colorRange(dre));
		primitive.pos = glm::vec2(0.3f, -0.3f);
		shape.m_Primitives.push_back(primitive);

		primitive.color = glm::vec3(colorRange(dre), colorRange(dre), colorRange(dre));
		primitive.pos = glm::vec2(-0.3f, 0.3f);
		shape.m_Primitives.push_back(primitive);
		primitive.color = glm::vec3(colorRange(dre), colorRange(dre), colorRange(dre));
		primitive.pos = glm::vec2(0.3f, 0.3f);
		shape.m_Primitives.push_back(primitive);

		shapes.push_back(shape);
	}

	// Shape 2
	{
		Shape shape{};
		float size{ sizeRange(dre) };
		shape.size = glm::vec2(size);

		Primitive primitive{};
		primitive.type = PT_TRIANGLE;
		primitive.color = glm::vec3(colorRange(dre), colorRange(dre), colorRange(dre));
		primitive.angle = 180.0f;
		primitive.isFilled = false;
		primitive.scale = glm::vec2(0.5f);
		primitive.pos = glm::vec2(0.0f, (0.5f - static_cast<float>(std::sqrt(3)) * 0.5f) * 0.5f);
		shape.m_Primitives.push_back(primitive);
		
		primitive.color = glm::vec3(colorRange(dre), colorRange(dre), colorRange(dre));
		primitive.angle = 90.0f;
		primitive.isFilled = false;
		primitive.scale = glm::vec2(0.5f);
		primitive.pos = glm::vec2(0.5f - static_cast<float>(std::sqrt(3)) * 0.5f, 0.0f);
		shape.m_Primitives.push_back(primitive);

		primitive.color = glm::vec3(colorRange(dre), colorRange(dre), colorRange(dre));
		primitive.angle = 0.0f;
		primitive.isFilled = false;
		primitive.scale = glm::vec2(0.5f);
		primitive.pos = glm::vec2(0.0f, (static_cast<float>(std::sqrt(3)) * 0.5f - 0.5f) * 0.5f);
		shape.m_Primitives.push_back(primitive);

		primitive.color = glm::vec3(colorRange(dre), colorRange(dre), colorRange(dre));
		primitive.angle = 270.0f;
		primitive.isFilled = false;
		primitive.scale = glm::vec2(0.5f);
		primitive.pos = glm::vec2(static_cast<float>(std::sqrt(3)) * 0.5f - 0.5f, 0.0f);
		shape.m_Primitives.push_back(primitive);

		shapes.push_back(shape);
	}

	// Shape 3
	{
		Shape shape{};
		float size{ sizeRange(dre) };
		shape.size = glm::vec2(size);

		Primitive primitive{};
		primitive.type = PT_RIGHTTRIANGLE;
		primitive.color = glm::vec3(colorRange(dre), colorRange(dre), colorRange(dre));
		primitive.angle = 0.0f;
		primitive.isFilled = false;
		primitive.scale = glm::vec2(0.5f, 1.0f);
		primitive.pos = glm::vec2(0.0f, 0.0f);
		shape.m_Primitives.push_back(primitive);

		primitive.color = glm::vec3(colorRange(dre), colorRange(dre), colorRange(dre));
		primitive.angle = 180.0f;
		primitive.isFilled = false;
		primitive.scale = glm::vec2(0.5f, 1.0f);
		primitive.pos = glm::vec2(0.0f, 0.0f);
		shape.m_Primitives.push_back(primitive);

		shapes.push_back(shape);
	}

	// Custom Shape 1
	{
		Shape shape{};
		float size{ sizeRange(dre) };
		shape.size = glm::vec2(size);

		Primitive primitive{};
		primitive.type = PT_QUAD;
		primitive.color = glm::vec3(colorRange(dre), colorRange(dre), colorRange(dre));
		primitive.angle = 0.0f;
		primitive.isFilled = false;
		primitive.scale = glm::vec2(0.5f);
		primitive.pos = glm::vec2(0.0f, 0.0f);
		shape.m_Primitives.push_back(primitive);

		primitive.type = PT_TRIANGLE;
		primitive.color = glm::vec3(colorRange(dre), colorRange(dre), colorRange(dre));
		primitive.angle = 0.0f;
		primitive.isFilled = false;
		primitive.scale = glm::vec2(0.5f);
		primitive.pos = glm::vec2(0.0f, -0.51f);
		shape.m_Primitives.push_back(primitive);

		primitive.color = glm::vec3(colorRange(dre), colorRange(dre), colorRange(dre));
		primitive.angle = 90.0f;
		primitive.pos = glm::vec2(0.51f, 0.0f);
		shape.m_Primitives.push_back(primitive);

		primitive.color = glm::vec3(colorRange(dre), colorRange(dre), colorRange(dre));
		primitive.angle = 180.0f;
		primitive.pos = glm::vec2(0.0f, 0.51f);
		shape.m_Primitives.push_back(primitive);

		primitive.color = glm::vec3(colorRange(dre), colorRange(dre), colorRange(dre));
		primitive.angle = 270.0f;
		primitive.pos = glm::vec2(-0.51f, 0.0f);
		shape.m_Primitives.push_back(primitive);
		shapes.push_back(shape);
	}

	// Custom Shape 2
	{
		Shape shape{};
		float size{ sizeRange(dre) };

		shape.size = glm::vec2(size);

		Primitive primitive{};
		primitive.type = PT_RIGHTTRIANGLE;
		primitive.color = glm::vec3(colorRange(dre), colorRange(dre), colorRange(dre));
		primitive.angle = 0.0f;
		primitive.isFilled = false;
		primitive.scale = glm::vec2(1.0f, 1.0f);
		primitive.pos = glm::vec2(0.0f, 0.0f);
		shape.m_Primitives.push_back(primitive);

		primitive.color = glm::vec3(colorRange(dre), colorRange(dre), colorRange(dre));
		primitive.angle = 180.0f;
		primitive.isFilled = false;
		primitive.scale = glm::vec2(0.5f, 0.5f);
		primitive.pos = glm::vec2(-0.25f, -0.25f);
		shape.m_Primitives.push_back(primitive);

		primitive.color = glm::vec3(colorRange(dre), colorRange(dre), colorRange(dre));
		primitive.angle = 180.0f;
		primitive.isFilled = false;
		primitive.scale = glm::vec2(0.5f, 0.5f);
		primitive.pos = glm::vec2(0.25f, 0.25f);
		shape.m_Primitives.push_back(primitive);

		primitive.color = glm::vec3(colorRange(dre), colorRange(dre), colorRange(dre));
		primitive.angle = 0.0f;
		primitive.isFilled = false;
		primitive.scale = glm::vec2(0.5f, 0.5f);
		primitive.pos = glm::vec2(0.25f, -0.25f);
		shape.m_Primitives.push_back(primitive);

		primitive.color = glm::vec3(colorRange(dre), colorRange(dre), colorRange(dre));
		primitive.angle = 180.0f;
		primitive.isFilled = false;
		primitive.scale = glm::vec2(0.5f, 0.5f);
		primitive.pos = glm::vec2(0.25f, -0.25f);
		shape.m_Primitives.push_back(primitive);

		shapes.push_back(shape);
	}

	std::vector<glm::vec2> acceptedPositons{};
	std::vector<glm::vec2> acceptedPrimitivePositons{};
	for (auto& shape : shapes)
	{
		// dart throwing
		std::uniform_real_distribution dartXRange{ screenSize.x * 0.5f + shape.size.x * 0.5f, screenSize.x - shape.size.x * 0.5f};
		std::uniform_real_distribution dartYRange{ shape.size.y * 0.5f, screenSize.y - shape.size.y * 0.5f };

		float candidateRadius{ glm::length(shape.size) * 0.5f };
		while (true)
		{
			glm::vec2 dartPosition = glm::vec2(dartXRange(dre), dartYRange(dre));
			bool accepted{ true };
			size_t positionsSize{ acceptedPositons.size() };
			for (int i = 0; i < positionsSize; ++i)
			{
				float rejectRadius{ glm::length(shapes[i].size) * 0.5f};

				glm::vec2 delta = dartPosition - acceptedPositons[i];

				float minDistance = candidateRadius + rejectRadius + margin;

				if (glm::dot(delta, delta) < minDistance * minDistance)
				{
					accepted = false;
					break;
				}
			}

			if (accepted)
			{
				shape.pos = dartPosition;
				acceptedPositons.push_back(shape.pos);
				break;
			}
		}
		for (const auto& primitive : shape.m_Primitives)
		{
			Primitive primitiveBuffer{ primitive };
			Shape primitiveShape{};
			primitiveBuffer.pos = glm::vec2(0.0f);
			primitiveBuffer.scale = glm::vec2(1.0f);
			primitiveBuffer.isFilled = true;
			primitiveShape.size = shape.size * primitive.scale;
			primitiveShape.m_Primitives.push_back(primitiveBuffer);

			// dart throwing
			std::uniform_real_distribution dartXRange{ primitiveShape.size.x * 0.5f, screenSize.x * 0.5f - primitiveShape.size.x * 0.5f };
			std::uniform_real_distribution dartYRange{ primitiveShape.size.y * 0.5f, screenSize.y - primitiveShape.size.y * 0.5f };

			float candidateRadius{ glm::length(primitiveShape.size) * 0.5f };
			while (true)
			{
				glm::vec2 dartPosition = glm::vec2(dartXRange(dre), dartYRange(dre));
				bool accepted{ true };
				size_t positionsSize{ acceptedPrimitivePositons.size() };
				for (int i = 0; i < positionsSize; ++i)
				{
					float rejectRadius{ glm::length(primitiveShapes[i].size) * 0.5f}; 

					glm::vec2 delta = dartPosition - acceptedPrimitivePositons[i];

					float minDistance = candidateRadius + rejectRadius + margin;

					if (glm::dot(delta, delta) < minDistance * minDistance)
					{
						accepted = false;
						break;
					}
				}

				if (accepted)
				{
					primitiveShape.pos = dartPosition;
					acceptedPrimitivePositons.push_back(primitiveShape.pos);
					break;
				}
			}

			primitiveShapes.push_back(primitiveShape);
		}
	}
}

void DrawScene(Shader& shader)
{
	glBindVertexArray(VAO);
	shader.Bind();
	shader.SetUniform("projection", projection);

	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(screenSize.x * 0.5f, screenSize.y * 0.5f, 0.0f));
	model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	model = glm::scale(model, glm::vec3(screenSize.y, 1.0f, 1.0f));

	shader.SetUniform("model", model);
	shader.SetUniform("a_Color", glm::vec3(0.5f, 0.5f, 1.0f));
	glLineWidth(lineWidth);
	glDrawElements(GL_LINES, 2, GL_UNSIGNED_INT, (const void*)(sizeof(unsigned int) * 3 * 3));

	for (auto& shape : shapes)
	{
		for (auto& primitive : shape.m_Primitives)
		{
			model = shape.GetWorldModelMatrix() * primitive.GetLocalModelMatrix();

			shader.SetUniform("model", model);
			if(primitive.isFilled)
				shader.SetUniform("a_Color", primitive.color);
			else
				shader.SetUniform("a_Color", glm::vec3(0.4f, 0.6f, 0.8f));

			primitive.Render();
		}
	}

	for (auto& primitiveShape : primitiveShapes)
	{
		for (auto& primitive : primitiveShape.m_Primitives)
		{
			model = primitiveShape.GetWorldModelMatrix() * primitive.GetLocalModelMatrix();

			shader.SetUniform("model", model);
			if (primitive.isFilled)
				shader.SetUniform("a_Color", primitive.color);
			else
				shader.SetUniform("a_Color", glm::vec3(0.4f, 0.6f, 0.8f));

			primitive.Render();
		}
	}
}

float DotPerp(const glm::vec2& a, const glm::vec2& b)
{
	return a.x * b.y - a.y * b.x;
}

bool MousePrimitiveTest(const glm::mat4& shapeWorldTransform ,const Primitive& a)
{
	glm::mat4 model = shapeWorldTransform * a.GetLocalModelMatrix();
	glm::vec2 P = glm::vec2(glm::inverse(model) * glm::vec4(mousePos, 0.0f, 1.0f));

	switch (a.type)
	{
	case PT_QUAD:
	{
		glm::vec2 A = glm::vec2(-0.5f, -0.5f);
		glm::vec2 B = glm::vec2(-0.5f, 0.5f);
		glm::vec2 C = glm::vec2(0.5f, 0.5f);
		glm::vec2 D = glm::vec2(0.5f, -0.5f);

		float c1 = DotPerp(B - A, P - A);
		float c2 = DotPerp(C - B, P - B);
		float c3 = DotPerp(D - C, P - C);
		float c4 = DotPerp(A - D, P - D);

		return (c1 >= 0 && c2 >= 0 && c3 >= 0 && c4  >= 0) || (c1 <= 0 && c2 <= 0 && c3 <= 0 && c4 <= 0);
	}
	case PT_TRIANGLE:
	{
		glm::vec2 A = glm::vec2(-0.5f, 0.5f);
		glm::vec2 B = glm::vec2(0.5f, 0.5f);
		glm::vec2 C = glm::vec2(0.0f, -0.5f);

		float c1 = DotPerp(B - A, P - A);
		float c2 = DotPerp(C - B, P - B);
		float c3 = DotPerp(A - C, P - C);

		return (c1 >= 0 && c2 >= 0 && c3 >= 0) || (c1 <= 0 && c2 <= 0 && c3 <= 0);
	}
	case PT_RIGHTTRIANGLE:
	{
		glm::vec2 A = glm::vec2(-0.5f, 0.5f);
		glm::vec2 B = glm::vec2(0.5f, 0.5f);
		glm::vec2 C = glm::vec2(-0.5f, -0.5f);

		float c1 = DotPerp(B - A, P - A);
		float c2 = DotPerp(C - B, P - B);
		float c3 = DotPerp(A - C, P - C);

		return (c1 >= 0 && c2 >= 0 && c3 >= 0) || (c1 <= 0 && c2 <= 0 && c3 <= 0);
	}
	}
	return false;
}

bool IsPrimitiveIdentical(const Primitive& a, const Primitive& b)
{
	return a.angle == b.angle &&
		std::abs(a.scale.x - b.scale.x) <= 0.1F  && std::abs(a.scale.y - b.scale.y) <= 0.1F 
		&& a.type == b.type;
}

bool PrimitivePrimitiveTest(const glm::mat4& aWorldTransform, const Primitive& a, const glm::mat4& bWorldTransform, const Primitive& b)
{
	std::vector<glm::vec2> aVertices;
	std::vector<glm::vec2> bVertices;

	switch (a.type)
	{
	case PT_QUAD:
		aVertices = { glm::vec2(-0.5f, -0.5f),glm::vec2(-0.5f, 0.5f), glm::vec2(0.5f, 0.5f),  glm::vec2(0.5f, -0.5f) };
		break;
	case PT_TRIANGLE:
		aVertices = { glm::vec2(-0.5f, 0.5f), glm::vec2(0.5f, 0.5f), glm::vec2(0.0f, -0.5f) };
		break;
	case PT_RIGHTTRIANGLE:
		aVertices = { glm::vec2(-0.5f, 0.5f) , glm::vec2(0.5f, 0.5f),glm::vec2(-0.5f, -0.5f) };
		break;
	}

	switch (b.type)
	{
	case PT_QUAD:
		bVertices = { glm::vec2(-0.5f, -0.5f),glm::vec2(-0.5f, 0.5f), glm::vec2(0.5f, 0.5f),  glm::vec2(0.5f, -0.5f) };
		break	;
	case PT_TRIANGLE:
		bVertices = { glm::vec2(-0.5f, 0.5f), glm::vec2(0.5f, 0.5f), glm::vec2(0.0f, -0.5f) };
		break;
	case PT_RIGHTTRIANGLE:
		bVertices = { glm::vec2(-0.5f, 0.5f) , glm::vec2(0.5f, 0.5f),glm::vec2(-0.5f, -0.5f) };
		break;
	}
	
	glm::mat4 aTransform{ aWorldTransform * a.GetLocalModelMatrix() };
	glm::mat4 bTransform{ bWorldTransform * b.GetLocalModelMatrix() };
	glm::mat4 BToA{ glm::inverse(aTransform) * bTransform };

	for (auto& bVertex : bVertices)
	{
		bVertex = glm::vec2(BToA * glm::vec4(bVertex, 0.0f, 1.0f));
	}
	
	for (size_t i = 0; i < aVertices.size(); ++i)
	{
		glm::vec2 edge = aVertices[(i + 1) % aVertices.size()] - aVertices[i];
		glm::vec2 axis = glm::normalize(glm::vec2(-edge.y, edge.x));

		float minA = std::numeric_limits<float>::max();
		float maxA = std::numeric_limits<float>::lowest();
		float minB = std::numeric_limits<float>::max();
		float maxB = std::numeric_limits<float>::lowest();
		for (const auto& vertex : aVertices)
		{
			float projection = glm::dot(vertex, axis);
			minA = std::min(minA, projection);
			maxA = std::max(maxA, projection);
		}

		for (const auto& vertex : bVertices)
		{
			float projection = glm::dot(vertex, axis);
			minB = std::min(minB, projection);
			maxB = std::max(maxB, projection);
		}

		if (maxA < minB || maxB < minA)
			return false;
	}

	for (size_t i = 0; i < bVertices.size(); ++i)
	{
		glm::vec2 edge = bVertices[(i + 1) % bVertices.size()] - bVertices[i];
		glm::vec2 axis = glm::normalize(glm::vec2(-edge.y, edge.x));

		float minA = std::numeric_limits<float>::max();
		float maxA = std::numeric_limits<float>::lowest();
		float minB = std::numeric_limits<float>::max();
		float maxB = std::numeric_limits<float>::lowest();
		for (const auto& vertex : aVertices)
		{
			float projection = glm::dot(vertex, axis);
			minA = std::min(minA, projection);
			maxA = std::max(maxA, projection);
		}

		for (const auto& vertex : bVertices)
		{
			float projection = glm::dot(vertex, axis);
			minB = std::min(minB, projection);
			maxB = std::max(maxB, projection);
		}

		if (maxA < minB || maxB < minA)
			return false;
	}

	return true;
}