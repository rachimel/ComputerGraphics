#include <Shader.h>
#include <print>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <gl/glew.h>
#include <glm/gtc/type_ptr.hpp>

#include <limits>

float e = std::numeric_limits<float>::epsilon();

Shader::Shader(std::string_view vertexShaderSource, std::string_view fragmentShaderSource)
{
	unsigned int vertexShader, fragmentShader;
	auto vertexShaderSourcePath{ std::filesystem::current_path() / vertexShaderSource };
	std::ifstream in{vertexShaderSourcePath.string()};
	if (!in)
	{
		std::println("[Vertex Shader] Failed to find vertex shader source path : {}", vertexShaderSource.data());
		return;
	}
	vertexShader = glCreateShader(GL_VERTEX_SHADER);
	std::stringstream ss;
	ss << in.rdbuf();
	std::string sourceBuffer{ ss.str() };
	const GLchar* source{ sourceBuffer.c_str() };
	int success;
	char infoLog[512];

	glShaderSource(vertexShader, 1, &source, nullptr);
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

	auto fragmentShaderSourcePath{ std::filesystem::current_path() / fragmentShaderSource };
	in.open(fragmentShaderSourcePath.string());
	if (!in)
	{
		std::println("[Fragment Shader] Failed to find fragment shader source path : {}", fragmentShaderSource.data());
		return;
	}

	ss << in.rdbuf();
	fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	sourceBuffer = ss.str();
	source = sourceBuffer.c_str();
	glShaderSource(fragmentShader, 1, &source, nullptr);
	glCompileShader(fragmentShader);
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);

	if (!success)
	{
		glGetShaderInfoLog(fragmentShader, sizeof(infoLog), nullptr, infoLog);
		std::println("[Fragment Shader] : Failed to Compile Fragment Shader!");
		std::println("[Info Log]");
		std::println("{}", infoLog);
	}

	m_ID = glCreateProgram();
	glAttachShader(m_ID, vertexShader);
	glAttachShader(m_ID, fragmentShader);

	glLinkProgram(m_ID);
	glGetProgramiv(m_ID, GL_LINK_STATUS, &success);

	if (!success)
	{
		glGetProgramInfoLog(m_ID, sizeof(infoLog), nullptr, infoLog);
		std::println("[Shader Program] : Failed to Link Shader Program!");
		std::println("[Info Log]");
		std::println("{}", infoLog);
	}

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);
}

Shader::~Shader()
{
	glDeleteProgram(m_ID);
}

void Shader::Bind()
{
	glUseProgram(m_ID);
}

void Shader::SetUniform(std::string_view location, bool data)
{
	glUniform1i(GetUniformLocation(location), data);
}

void Shader::SetUniform(std::string_view location, int data)
{
	glUniform1i(GetUniformLocation(location), data);
}

void Shader::SetUniform(std::string_view location, float data)
{
	glUniform1f(GetUniformLocation(location), data);
}

void Shader::SetUniform(std::string_view location, const glm::vec2& data)
{
	glUniform2f(GetUniformLocation(location), data.x, data.y);
}

void Shader::SetUniform(std::string_view location, const glm::vec3& data)
{
	glUniform3f(GetUniformLocation(location), data.x, data.y, data.z);
}

void Shader::SetUniform(std::string_view location, const glm::vec4& data)
{
	glUniform4f(GetUniformLocation(location), data.x, data.y, data.z, data.w);
}

void Shader::SetUniform(std::string_view location, const glm::mat2& data)
{
	glUniformMatrix2fv(GetUniformLocation(location), 1, GL_FALSE ,glm::value_ptr(data));
}

void Shader::SetUniform(std::string_view location, const glm::mat3& data)
{
	glUniformMatrix3fv(GetUniformLocation(location), 1, GL_FALSE, glm::value_ptr(data));
}

void Shader::SetUniform(std::string_view location, const glm::mat4& data)
{
	glUniformMatrix4fv(GetUniformLocation(location), 1, GL_FALSE, glm::value_ptr(data));
}

int Shader::GetUniformLocation(std::string_view location)
{
	return glGetUniformLocation(m_ID, location.data());
}
