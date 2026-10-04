#include <print>
#include <deque>
#include <random>

#include <gl/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <Shader.h>

std::random_device rd{};
std::default_random_engine dre{ rd() };
std::uniform_real_distribution colorRange{ 0.0f, 1.0f };

struct Square
{
	glm::vec2 pos{};
	glm::vec3 color{colorRange(dre), colorRange(dre), colorRange(dre)};
	glm::vec2 target{}; // target Pos
	float t{};
	bool inverted{false};
};

namespace {
	float vertices[]{
		-0.5f, -0.5f, 0.0f,
		-0.5f, 0.5f, 0.0f,
		0.5f, 0.5f, 0.0f,
		0.5f, -0.5f, 0.0f,
	};

	unsigned int indices[]{
		0,1,2,
		2,3,0
	};

	unsigned int VAO, VBO, EBO;
	Square leftSquare{}; 
	Square rightSquare{};

	std::vector<Square> stackedSquares{};
	std::deque<Square> animations{};
}

glm::vec2 screenSize{ 800.0f, 800.0f };
glm::mat4 projection;

int combo{ 0 };
float comboSpeed{ 1.0f };
bool squareCaptured{ false };
float speed{ 100.0f };
float margin{ 0.3f };
float lastTime{ 0.0f };
float frameTime{ 0.0f };

void FrameBufferSizeCallback(GLFWwindow* window, int width, int height);
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

void InitOpenGLObjects();
void Initialize();
void Update();

float smoothstep(float t)
{
	t = glm::clamp(t, 0.0f, 1.0f);
	return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

int main()
{
	if (!glfwInit())
	{
		std::println("[GLFW] Failed to initialize GLFW!");
		return -1;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(static_cast<int>(screenSize.x), static_cast<int>(screenSize.y), "practice-12", nullptr, nullptr);
	if (!window)
	{
		std::println("[GLFW] Failed to create window!");
		glfwTerminate();
		return -1;
	}

	glfwMakeContextCurrent(window);

	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK)
	{
		std::println("[GLEW] Failed to initialize GLEW!");
		glfwDestroyWindow(window);
		glfwTerminate();
		return -1;
	}

	glfwSetFramebufferSizeCallback(window, FrameBufferSizeCallback);
	glfwSetKeyCallback(window, KeyCallback);

	glViewport(0, 0, static_cast<int>(screenSize.x), static_cast<int>(screenSize.y));
	projection = glm::ortho(0.0f, screenSize.x, screenSize.y, 0.0f);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	InitOpenGLObjects();
	Initialize();

	Shader shader{ "shaders\\1_opengl_basics\\orthoStaticProjection.vs", "shaders\\1_opengl_basics\\uniformColor.fs" };
	Shader transparentShader{ "shaders\\1_opengl_basics\\orthoStaticProjection.vs", "shaders\\1_opengl_basics\\transparentColor.fs" };

	while (!glfwWindowShouldClose(window))
	{
		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		Update();
		shader.Bind();
		shader.SetUniform("projection", projection);

		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(leftSquare.pos, 0.0f));
		model = glm::scale(model, glm::vec3(screenSize * 0.1f, 1.0f));
		shader.SetUniform("model", model);
		shader.SetUniform("a_Color", leftSquare.color);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (const void*)0);

		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(rightSquare.pos, 0.0f));
		model = glm::scale(model, glm::vec3(screenSize * 0.1f, 1.0f));
		shader.SetUniform("model", model);
		shader.SetUniform("a_Color", rightSquare.color);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (const void*)0);

		// Draw Both Lines
		glLineWidth(2.0f);
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.15f * screenSize.x, 0.5f * screenSize.y, 0.0f));
		model = glm::scale(model, glm::vec3(0.1f * screenSize.x, 0.8f * screenSize.y, 1.0f));
		shader.SetUniform("model", model);
		shader.SetUniform("a_Color", glm::vec3(0.3f, 0.3f, 0.6f));
		glDrawElements(GL_LINE_LOOP, 6, GL_UNSIGNED_INT, (const void*)0);

		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.3f * screenSize.x, 0.5f * screenSize.y, 0.0f));
		model = glm::scale(model, glm::vec3(0.1f * screenSize.x, 0.8f * screenSize.y, 1.0f));
		shader.SetUniform("model", model);
		glDrawElements(GL_LINE_LOOP, 6, GL_UNSIGNED_INT, (const void*)0);

		transparentShader.Bind();
		transparentShader.SetUniform("projection", projection);
		transparentShader.SetUniform("a_Color", glm::vec4(0.3f, 0.3f, 0.6f, 0.5f));

		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.225f * screenSize.x, 0.5f * screenSize.y, 0.0f));
		model = glm::scale(model, glm::vec3(0.25f * screenSize.x, margin * screenSize.y, 1.0f));

		transparentShader.SetUniform("model", model);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (const void*)0);


		size_t animationsSize{ animations.size() };
		for (int i = 0; i < static_cast<int>(animationsSize); ++i)
		{
			model = glm::mat4(1.0f);
			model = glm::translate(model, glm::vec3(animations[i].pos, 0.0f));
			model = glm::scale(model, glm::vec3( (i + 1) / static_cast<float>(animationsSize) * 0.1f * screenSize, 1.0f));
			transparentShader.SetUniform("model", model);
			transparentShader.SetUniform("a_Color", glm::vec4(animations[i].color, (i + 1) / static_cast<float>(animationsSize)));
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (const void*)0);
		}

		shader.Bind();
		int stackedSize{ static_cast<int>(stackedSquares.size()) };
		for (int i = 0; i < stackedSize; ++i)
		{
			glm::vec2 placePos = (i % 2 == 0) ? glm::vec2(screenSize.x * (0.95f - (i / 20) * 0.2f), screenSize.y * (0.95f - ((i / 2) % 10) * 0.1f)) :
			glm::vec2(screenSize.x * (0.85f - (i / 20) * 0.2f), screenSize.y * (0.95f - ((i / 2) % 10) * 0.1f));
			model = glm::mat4(1.0f);
			model = glm::translate(model, glm::vec3(placePos, 0.0f));
			model = glm::scale(model, glm::vec3(0.1f * screenSize, 1.0f));
			shader.SetUniform("model", model);
			shader.SetUniform("a_Color", stackedSquares[i].color);
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (const void*)0);
		}
		glfwSwapBuffers(window);
		glfwPollEvents();
	}
}

void InitOpenGLObjects()
{
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_TRUE, sizeof(float) * 3, (const void*)0);
	glEnableVertexAttribArray(0);
}

void FrameBufferSizeCallback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
	screenSize = glm::vec2(static_cast<int>(screenSize.x), static_cast<int>(screenSize.y));
	projection = glm::ortho(0.0f, screenSize.x, screenSize.y, 0.0f);
}

void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (action == GLFW_PRESS)
	{
		switch (key)
		{
		case GLFW_KEY_ENTER:
		{
			if (squareCaptured) return;
			if (
				(leftSquare.pos.y - screenSize.y * 0.05f > screenSize.y * 0.5f - margin * screenSize.y * 0.5f &&
					leftSquare.pos.y + screenSize.y * 0.05f < screenSize.y * 0.5f + margin * screenSize.y * 0.5f) &&
				(rightSquare.pos.y - screenSize.y * 0.05f > screenSize.y * 0.5f - margin * screenSize.y * 0.5f &&
					rightSquare.pos.y + screenSize.y * 0.05f < screenSize.y * 0.5f + margin * screenSize.y * 0.5f)
				)
			{
				std::println("Hit");
				squareCaptured = true;

				size_t size{ stackedSquares.size() };
				if (size >= 60) { stackedSquares.erase(stackedSquares.begin(), stackedSquares.begin() + 20); size -= 20; }
				glm::vec2 halfSize{ screenSize * 0.05f };
				leftSquare.target = glm::vec2(screenSize.x * (0.95f - (size / 20) * 0.2f), screenSize.y * (0.95f - ((size / 2) % 10) * 0.1f));
				rightSquare.target = glm::vec2(screenSize.x * (0.85f - (size / 20) * 0.2f), screenSize.y * (0.95f - ((size / 2) % 10) * 0.1f));
				++combo;
			}
			else
			{
				combo = 0;
				margin = 0.3f;
			}
			break;
		}
		case GLFW_KEY_R:
			Initialize();
			break;
		case GLFW_KEY_Q:
			glfwSetWindowShouldClose(window, true);
			break;
		}
	}
}

void PlaceSquares()
{
	std::uniform_int_distribution directionDecision{ 0,1 };
	int decision{ directionDecision(dre) };

	leftSquare.inverted = (decision == 0) ? false : true;
	leftSquare.pos.x = screenSize.x * 0.15f;
	leftSquare.pos.y = (leftSquare.inverted) ? screenSize.y * 0.85f : screenSize.y * 0.15f;
	leftSquare.color = glm::vec3(colorRange(dre), colorRange(dre), colorRange(dre));

	rightSquare.inverted = (decision == 0) ? true : false;
	rightSquare.pos.x = screenSize.x * 0.3f;
	rightSquare.pos.y = (rightSquare.inverted) ? screenSize.y * 0.85f : screenSize.y * 0.15f;
	rightSquare.color = glm::vec3(colorRange(dre), colorRange(dre), colorRange(dre));

	leftSquare.t = 0.0f;
	rightSquare.t = 0.0f;
}
void Initialize()
{
	margin = 0.3f;
	combo = 0;
	squareCaptured = false;
	stackedSquares.clear();
	animations.clear();
	std::uniform_real_distribution<float> speedRange{ 150.0f, 300.0f };
	speed = speedRange(dre);
	PlaceSquares();
}
void Update()
{
	float currentTime = static_cast<float>(glfwGetTime());
	float deltaTime = currentTime - lastTime;
	lastTime = currentTime;

	frameTime += deltaTime;
	float targetSpeed = 0.5f + 0.5f * std::sqrt(static_cast<float>(combo));
	if(!squareCaptured)
	{
		float leftSquareDirection{ (leftSquare.inverted) ? -1.0f : 1.0f };
		float rightSquareDirection{ (rightSquare.inverted) ? -1.0f : 1.0f };

		leftSquare.pos += glm::vec2(0.0f, 1.0f) * leftSquareDirection * deltaTime * speed * targetSpeed;
		rightSquare.pos += glm::vec2(0.0f, 1.0f) * rightSquareDirection * deltaTime * speed * targetSpeed;

		if (leftSquare.pos.y <= screenSize.y * 0.15f)
		{
			leftSquare.pos.y = screenSize.y * 0.15f;
			leftSquare.inverted = false;
		}
		else if (leftSquare.pos.y >= screenSize.y * 0.85f)
		{
			leftSquare.pos.y = screenSize.y * 0.85f;
			leftSquare.inverted = true;
		}

		if (rightSquare.pos.y <= screenSize.y * 0.15f)
		{
			rightSquare.pos.y = screenSize.y * 0.15f;
			rightSquare.inverted = false;
		}
		else if (rightSquare.pos.y >= screenSize.y * 0.85f)
		{
			rightSquare.pos.y = screenSize.y * 0.85f;
			rightSquare.inverted = true;
		}
	}
	else
	{
		leftSquare.t = glm::clamp(leftSquare.t + deltaTime * targetSpeed, 0.0f, 1.0f);
		rightSquare.t = glm::clamp(rightSquare.t + deltaTime * targetSpeed, 0.0f, 1.0f);

		float leftEasingT{ smoothstep(leftSquare.t) };
		float rightEasingT{ smoothstep(rightSquare.t) };

		Square leftSquareAnimation{ leftSquare };
		leftSquareAnimation.pos = (1.0f - leftEasingT) * leftSquare.pos + (leftEasingT) * leftSquare.target;
		Square rightSquareAnimation{ rightSquare };
		rightSquareAnimation.pos = (1.0f - rightEasingT) * rightSquare.pos + (rightEasingT)*rightSquare.target;

		if (leftSquare.t >= 1.0f && rightSquare.t >= 1.0f)
		{
			animations.clear();
			stackedSquares.push_back(leftSquareAnimation);
			stackedSquares.push_back(rightSquareAnimation);
			squareCaptured = false;
			margin = glm::clamp(margin - 0.005f, 0.1f, 0.3f);
			PlaceSquares();
			return;
		}
		animations.push_back(leftSquareAnimation);
		animations.push_back(rightSquareAnimation);
		
	}

	if (animations.size() > 100)
	{
		animations.pop_front();
		animations.pop_front();
	}
}