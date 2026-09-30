#include <iostream>
#include <print>
#include <array>
#include <random>
#include <queue>

#include <gl/glew.h>
#include <glfw/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <Shader.h>

std::random_device rd;
std::default_random_engine dre{ rd() };
std::uniform_real_distribution colorRange{ 0.0f, 1.0f };

enum PrimitiveType
{
	PT_NONE,
	PT_QUAD,
	PT_TRIANGLE,
};
// [-screenSize / 2, screenSize / 2]
constexpr float lineWidth{ 2.0f };
constexpr float margin{ 2.5f };

struct Primitive
{
	PrimitiveType type{ PT_NONE };
	glm::vec2 pos{};
	glm::vec2 size{};
	float angle{ 0.0f };
	glm::vec3 color{};
	bool collided{false};
};

struct Particle
{
	Primitive geometry{};
	glm::vec2 dir{};
	float speed{};
	float t{};
};

struct Player
{
	Primitive geometry{};
	glm::vec2 dir{};
	float speed{};
	bool isMovingVertical{};
	bool isMoveEnd{ false };
	float nextY{};
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

		// circle
		0.0f, 0.0f, 0.0f
	};

	unsigned int indices[] = {
		// quad
		0, 1, 2,
		2, 3, 0,

		// equilateral triangle
		1,2 ,4,
	};

	constexpr float minPrimitiveGridRatio{ 0.1f };
	constexpr float maxPrimitiveGridRatio{ 0.25f };
}

// Rendering Data
namespace
{
	std::vector<Primitive> primitives;
	std::vector<Particle> particles;
	Player player{};

}

void FrameBufferSizeCallback(GLFWwindow* window, int width, int height);
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

void InitVAO();
void InitPlacements();

void UpdateScene();
bool TestCircleScreenInsersect();
bool TestCirclePrimitiveIntersect(const glm::vec2& gridPos, const Primitive& p);

unsigned int VAO, VBO, EBO;

glm::vec2 screenSize{ 900.0f, 900.0f };
glm::mat4 projection;
glm::mat4 view;
glm::vec2 gridSize{};
glm::vec2 gridUnit{};

float lastTime{};

int main()
{
	glm::ivec2 inputGridSize{};
	std::println("Input grid size [10, 30] :");
	while (true)
	{
		std::cin >> inputGridSize.x >> inputGridSize.y;
		if (std::cin.fail())
		{
			std::cin.clear();
			std::cin.ignore(std::numeric_limits<std::streamsize>::max(),'\n');
			continue;
		}
		if (inputGridSize.x < 10 || inputGridSize.x > 30 || inputGridSize.y < 10 || inputGridSize.y > 30)
		{
			std::println("[Error] : The grid size must be in range [10, 30]!");
			continue;
		}
		break;
	}

	if (!glfwInit())
	{
		std::println(std::cerr, "[GLFW] : Failed to initialize GLFW!");
		return -1;
	}


	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(static_cast<int>(screenSize.x), static_cast<int>(screenSize.y), "practice-11", nullptr, nullptr);
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

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	gridSize = glm::vec2(static_cast<float>(inputGridSize.x), static_cast<float>(inputGridSize.y));
	gridUnit = glm::vec2(screenSize.x / gridSize.x, screenSize.y / gridSize.y);

	glViewport(0, 0, static_cast<int>(screenSize.x), static_cast<int>(screenSize.y));
	projection = glm::ortho(0.0f, screenSize.x, screenSize.y, 0.0f);
	// register callbacks
	glfwSetFramebufferSizeCallback(window, FrameBufferSizeCallback);
	glfwSetKeyCallback(window, KeyCallback);

	InitVAO();
	InitPlacements();

	Shader shader{ "shaders\\1_opengl_basics\\orthoProjection.vs", "shaders\\1_opengl_basics\\uniformColor.fs" };
	Shader circleShader{ "shaders\\1_opengl_basics\\orthoProjection.vs", "shaders\\1_opengl_basics\\circle.fs" };
	Shader transparentShader{ "shaders\\1_opengl_basics\\orthoProjection.vs", "shaders\\1_opengl_basics\\transparentColor.fs" };

	while (!glfwWindowShouldClose(window))
	{
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		UpdateScene();
		glBindVertexArray(VAO);
		shader.Bind();
		shader.SetUniform("projection", projection);
		view = glm::mat4(1.0f);
		shader.SetUniform("view", view);
		// Rendering
		for (int y = 0; y < inputGridSize.y; ++y)
		{
			for (int x = 0; x < inputGridSize.x; ++x)
			{
				glm::mat4 model{ glm::mat4(1.0f) };
				glLineWidth(2.5f);
				glm::vec2 gridPos{ gridUnit.x * x + gridUnit.x * 0.5f, gridUnit.y * y + gridUnit.y * 0.5f };
				model = glm::translate(model, glm::vec3(gridPos, 0.0f));
				model = glm::scale(model, glm::vec3(gridUnit, 1.0f));
				shader.SetUniform("model", model);
				shader.SetUniform("a_Color", glm::vec3(0.3f));
				glDrawElements(GL_LINE_LOOP, 6, GL_UNSIGNED_INT, (const void*)0);

				auto& primitive = primitives[y * gridSize.x + x];
				glLineWidth(1.0f);
				model = glm::mat4(1.0f);
				model = glm::translate(model, glm::vec3(gridPos, 0.0f));
				model = glm::rotate(model, glm::radians(primitive.angle), glm::vec3(0.0f, 0.0f, 1.0f));
				model = glm::scale(model, glm::vec3(primitive.size, 1.0f));
				shader.SetUniform("model", model);
				shader.SetUniform("a_Color", primitive.color);
				switch (primitives[y * gridSize.x + x].type)
				{
				case PT_QUAD:
					glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (const void*)0);
					break;
				case PT_TRIANGLE:
					glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, (const void*)(6 * sizeof(unsigned int)));
					break;
				}
			}
		}
		// Player 
		{
			glm::mat4 model = glm::mat4(1.0f);
			model = glm::translate(model, glm::vec3(player.geometry.pos, 0.0f));
			model = glm::rotate(model, glm::radians(player.geometry.angle), glm::vec3(0.0f, 0.0f, 1.0f));
			model = glm::scale(model, glm::vec3(player.geometry.size, 1.0f));
			shader.SetUniform("model", model);
			shader.SetUniform("a_Color", glm::vec3(player.geometry.color));

			switch (player.geometry.type)
			{
			case PT_QUAD:
				glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (const void*)0);
				break;
			case PT_TRIANGLE:
				glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, (const void*)(6 * sizeof(unsigned int)));
				break;
			}
		}
		// Particle
		{
			transparentShader.Bind();
			transparentShader.SetUniform("projection", projection);
			transparentShader.SetUniform("view", view);

			for(auto& particle : particles)
			{
				glLineWidth(6.5f - (particle.t * 5.5f));
				glm::mat4 model = glm::mat4(1.0f);
				model = glm::translate(model, glm::vec3(particle.geometry.pos, 0.0f));
				model = glm::rotate(model, glm::radians(particle.geometry.angle), glm::vec3(0.0f, 0.0f, 1.0f));
				model = glm::scale(model, glm::vec3(particle.geometry.size, 1.0f));
				transparentShader.SetUniform("model", model);
				transparentShader.SetUniform("a_Color", glm::vec4(particle.geometry.color, 1.0f - particle.t));

				switch (particle.geometry.type)
				{
				case PT_QUAD:
					glDrawElements(GL_LINE_LOOP, 6, GL_UNSIGNED_INT, (const void*)0);
					break;
				case PT_TRIANGLE:
					glDrawElements(GL_LINE_LOOP, 3, GL_UNSIGNED_INT, (const void*)(6 * sizeof(unsigned int)));
					break;
				}
			}
		}

		glfwSwapBuffers(window);
		glfwPollEvents();
	}
}

bool TestCirclePrimitiveIntersect(const glm::vec2& gridPos, const Primitive& p)
{
	std::vector<glm::vec2> primitiveVertices{};
	switch (p.type)
	{
	case PT_QUAD:
		primitiveVertices = { glm::vec2(-0.5f, -0.5f), glm::vec2(-0.5f, 0.5f), glm::vec2(0.5f, 0.5f), glm::vec2(0.5f, -0.5f) };
		break;
	case PT_TRIANGLE:
		primitiveVertices = { glm::vec2(-0.5f, 0.5f), glm::vec2(0.5f, 0.5f), glm::vec2(0.0f, 0.5f - static_cast<float>(std::sqrt(3)) * 0.5f)};
		break;
	}

	glm::mat4 pModel = glm::mat4(1.0f);
	pModel = glm::translate(pModel, glm::vec3(gridPos + p.pos, 0.0f));
	pModel = glm::rotate(pModel, glm::radians(p.angle), glm::vec3(0.0f, 0.0f, 1.0f));
	pModel = glm::scale(pModel, glm::vec3(p.size, 1.0f));

	glm::mat4 cModel = glm::mat4(1.0f);
	cModel = glm::translate(cModel, glm::vec3(player.geometry.pos, 0.0f));
	cModel = glm::rotate(cModel, glm::radians(player.geometry.angle), glm::vec3(0.0f, 0.0f, 1.0f));
	cModel = glm::scale(cModel, glm::vec3(player.geometry.size, 1.0f));

	glm::mat4 PtoC = glm::inverse(cModel) * pModel;

	for (auto& vertex : primitiveVertices)
		vertex = glm::vec2(PtoC * glm::vec4(vertex, 0.0f, 1.0f));
	
	size_t count{ primitiveVertices.size() };
	bool inside{ true };
	float referenceSign{ 0.0f };

	for (size_t i = 0; i < count; ++i)
	{
		glm::vec2 A = primitiveVertices[i];
		glm::vec2 B = primitiveVertices[(i + 1) % count];

		glm::vec2 AB = B - A;
		glm::vec2 A0 = -A;
		
		float t = glm::dot(A0, AB) / glm::dot(A0, A0);
		float cross = AB.x * A0.y - AB.y * A0.x;

		if (std::abs(cross) <= 1e-6)
			continue;

		if (referenceSign == 0.0f)
		{
			referenceSign = cross;
		}
		else if (cross * referenceSign < 0.0f)
		{
			inside = false;
			break;
		}
		t = glm::clamp(t, 0.0f, 1.0f);

		glm::vec2 closest = A + t * AB;

		if (glm::dot(closest, closest) <= 0.25f)
			return true;
	}

	if (inside)
	{
		return true;
	}
	return false;
}

void UpdateScene()
{
	float currentTime = static_cast<float>(glfwGetTime());
	float deltaTime = currentTime - lastTime;
	lastTime = currentTime;

	if (!player.isMoveEnd)
	{
		glm::vec2 playerPos;
		if (player.isMovingVertical)
		{
			playerPos = player.geometry.pos + glm::vec2(0.0f, 1.0f) * player.speed * deltaTime;
			if (playerPos.y >= player.nextY)
			{
				playerPos.y = player.nextY;
				player.dir = player.dir * -1.0f;
				player.isMovingVertical = false;
			}
		}
		else
		{
			playerPos = player.geometry.pos + (player.dir) * player.speed * deltaTime;
			if (playerPos.x < gridUnit.x * 0.5f || playerPos.x > gridUnit.x * (gridSize.x - 1) + gridUnit.x * 0.5f)
			{
				playerPos.x = std::clamp(playerPos.x, gridUnit.x * 0.5f, gridUnit.x * (gridSize.x - 1) + gridUnit.x * 0.5f);
				player.nextY += gridUnit.y;
				if (player.nextY > gridUnit.y * (gridSize.y - 1) + gridUnit.y * 0.5f)
				{
					player.geometry.pos.x = gridUnit.x * 0.5f;
					player.isMoveEnd = true;
				}
				player.isMovingVertical = true;
			}
		}
		player.geometry.pos = playerPos;
	}

	glm::ivec2 gridIntSize{ static_cast<int>(gridSize.x), static_cast<int>(gridSize.y) };
	for (int y = 0; y < gridIntSize.y; ++y)
	{
		for (int x = 0; x < gridIntSize.x; ++x)
		{
			auto& primitive = primitives[y * gridIntSize.x + x];
			if (primitive.collided || primitive.type == PT_NONE) continue;
			glm::vec2 gridPos{ gridUnit.x * x + gridUnit.x * 0.5f, gridUnit.y * y + gridUnit.y * 0.5f };

			if (TestCirclePrimitiveIntersect(gridPos,primitives[y * gridIntSize.x + x]))
			{
				primitive.collided = true;
				std::println("Collided");

				Primitive oldPrimitive{ primitive };
				primitive.type = player.geometry.type;
				primitive.angle = player.geometry.angle;
				primitive.color = player.geometry.color;

				player.geometry.type = oldPrimitive.type;
				player.geometry.angle = oldPrimitive.angle;
				player.geometry.color = oldPrimitive.color;

				// create particles
				Particle newParticle;
				newParticle.t = 0.0f;
				newParticle.dir = glm::vec2(0.0f, 0.0f);
				newParticle.speed = 5.f;

				newParticle.geometry.type = primitives[y * gridIntSize.x + x].type;
				newParticle.geometry.pos = gridPos;
				newParticle.geometry.angle = primitives[y * gridIntSize.x + x].angle;
				newParticle.geometry.color = primitives[y * gridIntSize.x + x].color;
				particles.push_back(newParticle);
			}
		}
	}

	for (auto& particle : particles)
	{
		particle.t = std::clamp(particle.t + deltaTime, 0.0f, 1.0f);
		particle.geometry.size = particle.t * (gridUnit * 4.0f) ;
	}

	std::erase_if(particles, [](const auto& particle) {
		return particle.t >= 1.0f;
		});
}
void FrameBufferSizeCallback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
	screenSize = glm::vec2(static_cast<float>(width), static_cast<float>(height));
	projection = glm::ortho(0.0f, screenSize.x, screenSize.y, 0.0f);
	gridUnit = glm::vec2(screenSize.x / gridSize.x, screenSize.y / gridSize.y);
}

void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (action == GLFW_PRESS)
	{
		switch (key)
		{
		case GLFW_KEY_S:
			player.isMoveEnd = !player.isMoveEnd;
			break;
		case GLFW_KEY_MINUS:
			player.speed = std::clamp(player.speed - 50.0f, 0.0f, 1000.0f);
			break;
		case GLFW_KEY_EQUAL:
			if (mods & GLFW_MOD_SHIFT)
			{
				player.speed = std::clamp(player.speed + 50.0f, 0.0f, 1000.0f);
			}
			break;
		case GLFW_KEY_R:
			InitPlacements();
			break;
		case GLFW_KEY_Q:
			glfwSetWindowShouldClose(window, true);
			break;
		}
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
	primitives.clear();
	particles.clear();
	// reset player
	player.dir = glm::vec2(1.0f, 0.0f);
	player.speed = 200.0f;
	player.isMovingVertical = false;
	player.nextY = gridUnit.y * 0.5f;
	player.isMoveEnd = false;

	std::uniform_int_distribution type{ 0,1 };
	player.geometry.type = (type(dre)) ? PT_QUAD : PT_TRIANGLE;
	player.geometry.pos = glm::vec2(gridUnit * 0.5f);
	player.geometry.color = glm::vec3(1.0f);
	player.geometry.size = glm::vec2(std::min(gridUnit.x, gridUnit.y)) * 0.75f;
	// reset Screen Effect

	glm::ivec2 gridIntSize{ static_cast<int>(gridSize.x), static_cast<int>(gridSize.y) };
	const int gridCount{ gridIntSize.x * gridIntSize.y };
	int remainingGrids{ gridCount - 1};
	std::uniform_int_distribution<int> primitiveGridCountRange{ static_cast<int>(minPrimitiveGridRatio * gridCount), static_cast<int>(maxPrimitiveGridRatio* gridCount) };
	int spawnPrimitiveCount = primitiveGridCountRange(dre);
	{
		Primitive primitive{};
		primitive.type = PT_NONE;
		primitives.push_back(primitive);
	}
	remainingGrids -= spawnPrimitiveCount;
	for (int i = 0; i < spawnPrimitiveCount; ++i)
	{
		Primitive primitive{};
		primitive.type = PT_QUAD;
		primitive.size = glm::vec2(std::min(gridUnit.x, gridUnit.y) * 0.75f);
		primitive.angle = 0.0f;
		primitive.color = glm::vec3(colorRange(dre), colorRange(dre), colorRange(dre));
		primitive.pos = glm::vec2(0.0f);

		primitives.push_back(primitive);
	}
	spawnPrimitiveCount = primitiveGridCountRange(dre);
	remainingGrids -= spawnPrimitiveCount;
	for (int i = 0; i < spawnPrimitiveCount; ++i)
	{
		Primitive primitive{};
		primitive.type = PT_TRIANGLE;
		primitive.size = glm::vec2(std::min(gridUnit.x, gridUnit.y)) * 0.75f;
		primitive.angle = 0.0f;
		primitive.color = glm::vec3(colorRange(dre), colorRange(dre), colorRange(dre));
		primitive.pos = glm::vec2(0.0f);

		primitives.push_back(primitive);
	}
	spawnPrimitiveCount = primitiveGridCountRange(dre);
	remainingGrids -= spawnPrimitiveCount;
	for (int i = 0; i < spawnPrimitiveCount; ++i)
	{
		Primitive primitive{};
		primitive.type = PT_TRIANGLE;
		primitive.size = glm::vec2(std::min(gridUnit.x, gridUnit.y)) * 0.75f;
		primitive.angle = 180.0f;
		primitive.color = glm::vec3(colorRange(dre), colorRange(dre), colorRange(dre));
		primitive.pos = glm::vec2(0.0f);

		primitives.push_back(primitive);
	}

	for (int i = 0; i < remainingGrids; ++i)
	{
		Primitive primitive{};
		primitive.type = PT_NONE;
		primitives.push_back(primitive);
	}

	std::shuffle(primitives.begin() + 1, primitives.end(), dre);

}
