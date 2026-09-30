#pragma once
#include <string>
#include <glm/glm.hpp>
class Shader
{
public:
	Shader(std::string_view vertexShaderSource, std::string_view fragmentShaderSource);
	~Shader();

	void Bind();
	void SetUniform(std::string_view location, bool  data);
	void SetUniform(std::string_view location, int   data);
	void SetUniform(std::string_view location, float data);
	void SetUniform(std::string_view location, const glm::vec2& data);
	void SetUniform(std::string_view location, const glm::vec3& data);
	void SetUniform(std::string_view location, const glm::vec4& data);
	void SetUniform(std::string_view location, const glm::mat2& data);
	void SetUniform(std::string_view location, const glm::mat3& data);
	void SetUniform(std::string_view location, const glm::mat4& data);
private:
	int GetUniformLocation(std::string_view location);
private:
	unsigned int m_ID{ 0 };
};