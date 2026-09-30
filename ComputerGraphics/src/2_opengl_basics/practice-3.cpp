// Standard C++ Libraries
#include <iostream>
#include <print>
#include <array>
#include <filesystem>
#include <fstream>
#include <sstream>

#include <random>

// OpenGL Libraries
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

struct Quad
{
	glm::vec2 pos{};
	glm::vec2 size{};
	// material
	glm::vec3 color{};
};

std::random_device rd;
std::default_random_engine dre{ rd() };
// Data
unsigned int VAO, VBO, EBO;
unsigned int quadShader;
std::array<float, 12> quadVertices{
	// Left-Bottom
	-1.0f, -1.0f, 0.0f,
	1.0f, -1.0f, 0.0f,
	1.0f, 1.0f, 0.0f,
	-1.0f, 1.0f, 0.0f
};

std::array<unsigned int, 6> quadIndices{
	0, 1, 2,
	2, 3, 0,
};

glm::vec2 mousePos{};
bool mouseDragging{ false };

glm::vec2 screenSize{800,600};
glm::mat4 orthographic{};
float aspect{ 800.f / 600.f };

std::array<Quad, 20> quads{};
int quadCounts{};
int holdingQuadIdx{-1};
// Callback Functions
void FrameBufferSizeCallback(GLFWwindow* window, int width, int height);
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void CursorPosCallback(GLFWwindow* window, double xPosIn, double yPosIn);

// Functions
void DrawScene();
void InitVAO();
void InitShaders();
void CleanUp();

int main()
{
	if (!glfwInit())
	{
		std::println(std::cerr, "Failed to initialize GLFW!");
		return -1;
	}
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(static_cast<int>(screenSize.x), static_cast<int>(screenSize.y), "practice 3", nullptr, nullptr);

	if (!window)
	{
		std::println(std::cerr, "[GLFW]: Failed to Create Window!");
		glfwTerminate();
		return -1;
	}
	// Register Callbacks
	glfwSetFramebufferSizeCallback(window, FrameBufferSizeCallback);
	glfwSetKeyCallback(window, KeyCallback);
	glfwSetMouseButtonCallback(window, MouseButtonCallback);
	glfwSetCursorPosCallback(window, CursorPosCallback);
	glfwMakeContextCurrent(window);

	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK)
	{
		std::println(std::cerr, "[GLEW] : Failed to initialize GLEW!");
		return -1;
	}

	glViewport(0, 0, 800, 600);
	orthographic = glm::ortho(0.f,800.f,600.f,0.f);
	InitVAO();
	InitShaders();

	while (!glfwWindowShouldClose(window))
	{
		DrawScene();
		glfwPollEvents();
		glfwSwapBuffers(window);
	}

	CleanUp();
	glfwDestroyWindow(window);
	glfwTerminate();
}

void FrameBufferSizeCallback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
	screenSize.x = static_cast<float>(width);
	screenSize.y = static_cast<float>(height);

	aspect = screenSize.x / screenSize.y;
	orthographic = glm::ortho(0.f, screenSize.x, screenSize.y, 0.f);
}

void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (action == GLFW_PRESS)
	{
		switch (key)
		{
		case GLFW_KEY_A:
		{
			if (quadCounts >= 10) break;
			auto& quad = quads[quadCounts];
			std::uniform_real_distribution sizeDistribution{ 0.05f, 0.1f };
			quad.size.x = screenSize.x * sizeDistribution(dre);
			quad.size.y = screenSize.y * sizeDistribution(dre);
			std::uniform_real_distribution spawnXDistribution{ quad.size.x * 0.5f, screenSize.x - quad.size.x * 0.5f};
			std::uniform_real_distribution spawnYDistribution{ quad.size.y * 0.5f, screenSize.y - quad.size.y * 0.5f };
			quad.pos = glm::vec2(spawnXDistribution(dre), spawnYDistribution(dre));
			std::uniform_real_distribution colorDistribution{ 0.0f, 1.0f };
			quad.color = glm::vec3(colorDistribution(dre), colorDistribution(dre), colorDistribution(dre));
			++quadCounts;
			break;
		}
		case GLFW_KEY_Q:
		{
			glfwSetWindowShouldClose(window, true);
			break;
		}
		}
	}
}

int FindHoldingQuad()
{
	for (int i = quadCounts - 1; i >= 0; --i)
	{
		if (mousePos.x >= quads[i].pos.x - quads[i].size.x && mousePos.x <= quads[i].pos.x + quads[i].size.x &&
			mousePos.y >= quads[i].pos.y - quads[i].size.y && mousePos.y <= quads[i].pos.y + quads[i].size.y)
		{
			return i; 
		}
	}
	return -1;
}

void MergeWithHoldingQuad()
{
	for (int i = quadCounts - 1; i >= 0; --i)
	{
		if (i == holdingQuadIdx)
			continue;
		auto holdingQuad{ quads[holdingQuadIdx] };
		auto quad{ quads[i] };

		float AminX = holdingQuad.pos.x - holdingQuad.size.x;
		float AmaxX = holdingQuad.pos.x + holdingQuad.size.x;
		float AminY = holdingQuad.pos.y - holdingQuad.size.y;
		float AmaxY = holdingQuad.pos.y + holdingQuad.size.y;
		float BminX = quad.pos.x - quad.size.x;
		float BmaxX = quad.pos.x + quad.size.x;
		float BminY = quad.pos.y - quad.size.y;
		float BmaxY = quad.pos.y + quad.size.y;

		if (!(AmaxX >= BminX && AminX <= BmaxX))
		{
			continue;
		}
		if (!(AmaxY >= BminY && AminY <= BmaxY))
		{
			continue;
		}

		// calculate max size
		float newMinX = std::min(AminX, BminX);
		float newMaxX = std::max(AmaxX, BmaxX);
		float newMinY = std::min(AminY, BminY);
		float newMaxY = std::max(AmaxY, BmaxY);

		Quad newQuad{};
		newQuad.pos = glm::vec2((newMinX + newMaxX) * 0.5f, (newMinY + newMaxY) * 0.5f);
		newQuad.size = glm::vec2(newMaxX - newMinX, newMaxY - newMinY) * 0.5f;
		std::uniform_real_distribution colorDistribution{ 0.0f, 1.0f };
		newQuad.color = glm::vec3(colorDistribution(dre), colorDistribution(dre), colorDistribution(dre)); 
		int deleteIdx = std::max(holdingQuadIdx, i);
		for (int j = deleteIdx; j < quadCounts - 1 ; ++j)
		{
			quads[j] = quads[j + 1];
		}
		deleteIdx = std::min(holdingQuadIdx, i);
		for (int j = deleteIdx; j < quadCounts - 1; ++j)
		{
			quads[j] = quads[j + 1];
		}
		quadCounts--;
		quads[quadCounts - 1] = newQuad;
		break;
	}
}

void SplitHoldingQuad()
{
	std::uniform_real_distribution sizeDistribution{ 0.4f, 0.7f };
	std::uniform_real_distribution colorDistribution{ 0.0f, 1.0f };
	auto holdingQuad = quads[holdingQuadIdx];
	Quad splitQuadA{}, splitQuadB{};
	splitQuadA.size = glm::vec2(holdingQuad.size.x * sizeDistribution(dre), holdingQuad.size.y * sizeDistribution(dre));
	splitQuadA.pos = holdingQuad.pos - glm::vec2(splitQuadA.size.x, 0.0f);
	splitQuadA.color = glm::vec3(colorDistribution(dre), colorDistribution(dre), colorDistribution(dre));
	splitQuadB.size = holdingQuad.size- splitQuadA.size;
	splitQuadB.pos = holdingQuad.pos + glm::vec2(splitQuadB.size.x, 0.0f);
	splitQuadB.color = glm::vec3(colorDistribution(dre), colorDistribution(dre), colorDistribution(dre));

	for (int i = holdingQuadIdx; i < quadCounts - 1; ++i)
	{
		quads[i] = quads[i + 1];
	}
	quads[quadCounts - 1] = splitQuadA;
	quads[quadCounts++] = splitQuadB;
	holdingQuadIdx = -1;
}

void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
	{
		// find which rect the mouse is dragging.
		mouseDragging = true;
		holdingQuadIdx = FindHoldingQuad();
		if (holdingQuadIdx >= 0)
		{
			std::println("Found Quad! Index : {}", holdingQuadIdx);
		}
	}
	else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
	{
		mouseDragging = false;
		if(holdingQuadIdx >= 0)
		{
			MergeWithHoldingQuad();
		}
	}
	else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
	{
		holdingQuadIdx = FindHoldingQuad();
		if (quadCounts < 20 && holdingQuadIdx >= 0)
		{
			std::println("Split Quad! Index : {}", holdingQuadIdx);
			SplitHoldingQuad();
		}
	}
}

void CursorPosCallback(GLFWwindow* window, double xPosIn, double yPosIn)
{
	mousePos = glm::vec2(xPosIn, yPosIn);
	if (mouseDragging && holdingQuadIdx != -1)
	{
		quads[holdingQuadIdx].pos = mousePos;
	}
}
void DrawScene()
{
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glUseProgram(quadShader);
	glUniformMatrix4fv(glGetUniformLocation(quadShader, "projection"), 1, GL_FALSE, glm::value_ptr(orthographic));
	glBindVertexArray(VAO);
	for (int i = 0; i < quadCounts; ++i)
	{
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(quads[i].pos, 0.0f));
		model = glm::scale(model, glm::vec3(quads[i].size, 1.0f));
		glUniformMatrix4fv(glGetUniformLocation(quadShader, "model"), 1, GL_FALSE, glm::value_ptr(model));
		glUniform3fv(glGetUniformLocation(quadShader, "quadColor"), 1, glm::value_ptr(quads[i].color));
		glDrawElements(GL_TRIANGLES, 6 , GL_UNSIGNED_INT, (void*)0);
	}
	glBindVertexArray(0);
	glUseProgram(0);
}

void InitVAO()
{
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * quadVertices.size(), quadVertices.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * quadIndices.size(), quadIndices.data(), GL_STATIC_DRAW);
	
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, (void*)0);
	glEnableVertexAttribArray(0);
}

void InitShaders()
{
	unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
	auto vertexShaderPath = std::filesystem::current_path() / "shaders\\1_opengl_basics\\quadShader.vs";
	auto fragmentShaderPath = std::filesystem::current_path() / "shaders\\1_opengl_basics\\quadShader.fs";
	std::ifstream in{vertexShaderPath.string()};
	if (!in)
	{
		std::println("Failed to open path : {}", vertexShaderPath.string());
		return;
	}
	int success;
	char infoLog[512];
	std::stringstream ss;
	ss << in.rdbuf();
	std::string vertexSourceString{ ss.str() };
	const GLchar* vertexSource{ vertexSourceString.c_str() };
	glShaderSource(vertexShader, 1, &vertexSource, nullptr);
	glCompileShader(vertexShader);
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(vertexShader, sizeof(infoLog), nullptr, infoLog);
		std::println("[Vertex Shader] : Failed to Compile Vertex Shader!");
		std::println("[Info Log]");
		std::println("{}", infoLog);
	}

	ss.str("");
	ss.clear();
	in.close();

	unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	in.clear();
	in.open(fragmentShaderPath.string());
	if (!in)
	{
		std::println("Failed to open path : {}", fragmentShaderPath.string());
		return;
	}
	ss << in.rdbuf();
	std::string fragmentSourceString{ ss.str() };
	const GLchar* fragmentSource{ fragmentSourceString.c_str() };
	glShaderSource(fragmentShader, 1, &fragmentSource, nullptr);
	glCompileShader(fragmentShader);
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(fragmentShader, sizeof(infoLog), nullptr, infoLog);
		std::println("[Fragment Shader] : Failed to Compile Fragment Shader!");
		std::println("[Info Log]");
		std::println("{}", infoLog);
	}

	quadShader = glCreateProgram();
	glAttachShader(quadShader, vertexShader);
	glAttachShader(quadShader, fragmentShader);
	glLinkProgram(quadShader);
	glGetProgramiv(quadShader, GL_LINK_STATUS, &success);
	if (!success)
	{
		glGetProgramInfoLog(quadShader, sizeof(infoLog), nullptr, infoLog);
		std::println("[Shader Program] : Failed to Link Shader Program!");
		std::println("[Info Log]");
		std::println("{}", infoLog);
	}

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);
}

void CleanUp()
{
	// Clean-up Shader
	glDeleteProgram(quadShader);
	// Clean-up VAO
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &EBO);
	glDeleteVertexArrays(1, &VAO);
}