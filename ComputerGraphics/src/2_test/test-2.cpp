// without framework for preparing the test

#include <iostream>
#include <print>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <gl/glew.h>
#include <GLFW/glfw3.h>

using Vec2 = glm::vec2;
using Vec3 = glm::vec3;
using Vec4 = glm::vec4;
using Mat3 = glm::mat3;
using Mat4 = glm::mat4;

namespace
{
	const char* vertexShaderSource = {R"(
#version 450 core

layout (location = 0) in vec3 a_Pos;
layout (location = 1) in vec3 a_Normal;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;
uniform mat3 normalMatrix;

out vec3 Normal;
out vec3 FragPos;

void main()
{
	vec4 viewPos = view * model * vec4(a_Pos, 1.0);
	gl_Position = projection * viewPos;
	Normal = normalMatrix * a_Normal;
	FragPos = vec3(viewPos);
}
)"};
	const char* fragmentShaderSource = { R"(
#version 450 core

// needs to be normalized when uploading to the shader!
uniform vec3 lightPos;

uniform vec3 lightColor;
uniform vec3 objectColor;

uniform float ambientStrength;
uniform float specularStrength;
uniform int shininess;
uniform bool enableLight;

in vec3 FragPos;
in vec3 Normal;

out vec4 FragColor;

void main()
{
    if(enableLight)
    {
        vec3 lightDir = normalize(FragPos - lightPos);
        vec3 normal = normalize(Normal);
        // ambient
        vec3 ambientColor = objectColor * ambientStrength;
        // diffuse
        float diffuse = max(dot(-lightDir, normal), 0.0);
        vec3 diffuseColor = objectColor * lightColor * diffuse;

        vec3 viewDir = normalize(-FragPos);
        vec3 specularDir = reflect(lightDir, normal);
        float specular = pow(max(dot(specularDir, viewDir), 0.0), shininess);

        vec3 specularColor = lightColor * specular * specularStrength;
        FragColor = vec4(ambientColor + diffuseColor + specularColor, 1.0);
    }
    else
    {
        FragColor = vec4(0.0, 0.0, 0.0, 1.0);
    }

}
)"};

	float cubeVertices[] = {
		// positions           // normals  
		-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
		 0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
		 0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
		 0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
		-0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
		-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,

		-0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
		 0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
		 0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
		 0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
		-0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
		-0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,

		-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
		-0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
		-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
		-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
		-0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
		-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,

		 0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
		 0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
		 0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
		 0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
		 0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
		 0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,

		-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
		 0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
		 0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
		 0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
		-0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
		-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,

		-0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
		 0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
		 0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
		 0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
		-0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
		-0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f
	};
	glm::vec2 screenSize{ 1280,960 };
	unsigned int VAO, VBO;

	unsigned int vertexShader, fragmentShader, shader;
}

void BindShader()
{
	glUseProgram(shader);
}
void SetUniform(std::string_view location, const bool b)
{
	glUniform1i(glGetUniformLocation(shader, location.data()), b);
}

void SetUniform(std::string_view location, const int i)
{
	glUniform1i(glGetUniformLocation(shader, location.data()), i);
}

void SetUniform(std::string_view location, const float f)
{
	glUniform1f(glGetUniformLocation(shader, location.data()), f);
}

void SetUniform(std::string_view location, const Vec3& v)
{
	glUniform3f(glGetUniformLocation(shader, location.data()), v.x, v.y, v.z);
}

void SetUniform(std::string_view location, const Vec4& v)
{
	glUniform4f(glGetUniformLocation(shader, location.data()), v.x, v.y, v.z, v.w);
}

void SetUniform(std::string_view location, const Mat3& m)
{
	glUniformMatrix3fv(glGetUniformLocation(shader, location.data()), 1, GL_FALSE, glm::value_ptr(m));
}

void SetUniform(std::string_view location, const Mat4& m)
{
	glUniformMatrix4fv(glGetUniformLocation(shader, location.data()), 1, GL_FALSE, glm::value_ptr(m));
}

void FrameBufferSizeCallback(GLFWwindow* window, int width, int height)
{
	screenSize = { static_cast<float>(width), static_cast<float>(height) };
	glViewport(0, 0, width, height);
}

void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
void CharCallback(GLFWwindow* window, unsigned int codepoint);
void PollInputs(GLFWwindow* window);

void Init();
void Reset();
void Update();
void Render();



int main()
{
	if (!glfwInit())
	{
		std::println(std::cerr, "[GLFW] Failed to initialize GLFW!");
		return -1;
	}
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(static_cast<int>(screenSize.x),
		static_cast<int>(screenSize.y), "IHComputerGraphics", nullptr, nullptr);

	if (!window)
	{
		std::println(std::cerr, "[GLFW] Failed to create window!");
		return -2;
	}
	glfwSetCharCallback(window, CharCallback);
	glfwSetKeyCallback(window, KeyCallback);
	glfwSetFramebufferSizeCallback(window, FrameBufferSizeCallback);

	glfwMakeContextCurrent(window);

	glewExperimental = GL_TRUE;

	if (glewInit() != GLEW_OK)
	{
		std::println(std::cerr, "[GLEW] Failed to initialize GLEW!");
		return -3;
	}

	glEnable(GL_DEPTH_TEST);
	Init();
	Reset();

	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
		// poll Inputs
		PollInputs(window);
		// Update
		Update();
		// Render
		Render();
		glfwSwapBuffers(window);
	}
}

void Init()
{
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 6, (const void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 6, (const void*)(sizeof(float) * 3));

	glBindVertexArray(0);
	vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
	glCompileShader(vertexShader);

	int ok;
	GLchar errorLog[500]{};
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &ok);
	if (!ok)
	{
		glGetShaderInfoLog(vertexShader, sizeof(errorLog), nullptr, errorLog);
		std::println("[Error] Failed to compile vertex shader!");
		std::println("[ErrorLog] \n {}", errorLog);

	}

	fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
	glCompileShader(fragmentShader);
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &ok);
	if (!ok)
	{
		glGetShaderInfoLog(fragmentShader, sizeof(errorLog), nullptr, errorLog);
		std::println("[Error] Failed to compile fragment shader!");
		std::println("[ErrorLog] \n {}", errorLog);

	}

	shader = glCreateProgram();
	glAttachShader(shader, vertexShader);
	glAttachShader(shader, fragmentShader);

	glLinkProgram(shader);
	glGetProgramiv(shader, GL_LINK_STATUS, &ok);

	if (!ok)
	{
		glGetProgramInfoLog(shader, sizeof(errorLog), nullptr, errorLog);
		std::println("[Error] Failed to link shader!");
		std::println("[ErrorLog] \n {}", errorLog);
	}

	glViewport(0, 0, static_cast<int>(screenSize.x), static_cast<int>(screenSize.y));
}

namespace
{
	Vec3 cameraPos{ 0.0f };
	float cameraYaw{ 0.0f };
	float cameraPitch{ 0.0f };

	float fovy{ 90.0f };
	Mat4 projection{};
	Mat4 view{};

	float lastTime{};
	float deltaTime{};

	Vec3 lightPos{ 0.0f, 10.0f, 0.0f };
	Vec3 lightColor{ 1.0, 0.0, 0.0f };

	std::vector<Vec3> modelColors{};
	std::vector<Mat4> models{};
	std::vector<Mat4> modelTransforms{};
}

#pragma region 카메라
void FocusAt(const Vec3& v)
{
	Vec3 d = glm::normalize(v - cameraPos);
	cameraYaw = glm::degrees(glm::atan(d.z, d.x));
	cameraPitch = glm::degrees(glm::atan(d.y, glm::length(Vec2(d.x, d.z))));
}
#pragma endregion

void Reset()
{
	cameraPos = Vec3{ 0.0f, 1.5f, 3.0f };
	FocusAt(Vec3{ 0.0f });
	models = std::vector<Mat4>();

	Mat4 model = Mat4{ 1.0f };
	model = glm::translate(model, Vec3{ 0.0f, -0.05f, 0.0f });
	model = glm::scale(model, Vec3{ 100.0f, 0.1f, 100.0f });
	models.push_back(model);

	model = Mat4{ 1.0f };
	model = glm::translate(model, Vec3{ 0.0f, 0.5f, 0.0f });
	models.push_back(model);


	modelColors.push_back(Vec3{ 0.5f });
	modelColors.push_back(Vec3{ 1.0f, 0.0f, 0.0f });
}

void Update()
{
	modelTransforms = std::vector<Mat4>(models.size());
	float curtime = static_cast<float>(glfwGetTime());
	deltaTime = curtime - lastTime;
	lastTime = curtime;

	Vec3 lookDir{};
	lookDir.x = glm::cos(glm::radians(cameraYaw))* glm::cos(glm::radians(cameraPitch));
	lookDir.y = glm::sin(glm::radians(cameraPitch));
	lookDir.z = glm::sin(glm::radians(cameraYaw)) * glm::cos(glm::radians(cameraPitch));

	Vec3 front = glm::normalize(lookDir);
	view = glm::lookAt(cameraPos, cameraPos + front, Vec3{ 0.0f, 1.0f, 0.0f });
	projection = glm::perspective(glm::radians(fovy), screenSize.x / screenSize.y, 0.1f, 1000.f);

	//
	modelTransforms[0] = models[0];
	modelTransforms[1] = models[1];
}

void Render()
{
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Bind Shader
	BindShader();
	SetUniform("view", view);
	SetUniform("projection", projection);

	SetUniform("lightPos", Vec3{ view * Vec4{ lightPos, 1.0f } });
	SetUniform("lightColor", lightColor);
	SetUniform("ambientStrength", 0.2f);
	SetUniform("specularStrength", 0.4f);
	SetUniform("shininess", 4);
	SetUniform("enableLight", true);

	glBindVertexArray(VAO);
	size_t modelSize{ modelTransforms.size() };
	for (int i = 0; i < modelSize; ++i)
	{
		Mat3 normal = Mat3{ glm::inverse(glm::transpose(view * modelTransforms[i]))};
		SetUniform("model", modelTransforms[i]);
		SetUniform("normalMatrix", normal);
		SetUniform("objectColor", modelColors[i]);
		glDrawArrays(GL_TRIANGLES, 0, 36);
	}

}
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if(action == GLFW_PRESS)
	{
		switch (key)
		{
		case GLFW_KEY_Q:
			glfwSetWindowShouldClose(window, true);
			break;

		}
	}
}

void CharCallback(GLFWwindow* window, unsigned int codepoint)
{

}

void PollInputs(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_KP_8))
	{
		cameraPos.z -= 1.0f * deltaTime;
	}
	else if (glfwGetKey(window, GLFW_KEY_KP_2))
	{
		cameraPos.z += 1.0f * deltaTime;
	}

	if (glfwGetKey(window, GLFW_KEY_KP_4))
	{
		cameraPos.x -= 1.0f * deltaTime;
	}
	else if (glfwGetKey(window, GLFW_KEY_KP_6))
	{
		cameraPos.x += 1.0f * deltaTime;
	}

	if (glfwGetKey(window, GLFW_KEY_KP_9))
	{
		cameraPos.y += 1.0f * deltaTime;
	}
	else if(glfwGetKey(window, GLFW_KEY_KP_3))
	{
		cameraPos.y -= 1.0f * deltaTime;
	}
}