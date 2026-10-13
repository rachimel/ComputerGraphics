#pragma once

#include <string>
#include <filesystem>
#include <vector>
#include <glm/glm.hpp>

class Shader;

enum class VertexTag
{
	None = 0,
	Color = 1 << 0,
	Normal = 1 << 1,
	Texture = 1 << 2,
};

constexpr VertexTag operator|(VertexTag a, VertexTag b) 
{
	return static_cast<VertexTag>(std::to_underlying(a) | std::to_underlying(b));
}

constexpr bool operator&(VertexTag a, VertexTag b)
{
	return (std::to_underlying(a) & std::to_underlying(b));
}

struct Vertex
{
	glm::vec3 pos;
	glm::vec3 color;
	glm::vec3 normal;
	glm::vec2 texCoord;
};

struct Texture
{
	unsigned int id;
	std::string type;
};

class Mesh
{
public:
	Mesh(VertexTag tag, std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<Texture> m_Textures);
	~Mesh();

	Mesh(const Mesh&) = delete;
	Mesh& operator= (const Mesh&) = delete;

	Mesh(Mesh&& rhs) noexcept;
	Mesh& operator= (Mesh&& rhs) noexcept;

	void Bind();
	void UpdateVertices();
public:
	VertexTag m_Layout;

	std::vector<Vertex> m_Vertices;
	std::vector<unsigned int> m_Indices;
	std::vector<Texture> m_Textures;
private:
	void CreateVertexObjects();
	unsigned int VAO, VBO, EBO;
};



