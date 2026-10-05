#pragma once

#include <vector>
#include <glm/glm.hpp>
class Shader;

struct Vertex
{
	glm::vec3 position;
#if defined(CG_MESH_ENABLE_COLOR)
	glm::vec3 color;
#endif
#if defined(CG_MESH_ENABLE_NORMAL)
	glm::vec3 normal;
#endif
#if defined(CG_MESH_ENABLE_TEXTURE)
	glm::vec2 texCoord;
#endif
};

class Mesh
{
public:
	Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices);
	~Mesh();

	Mesh(const Mesh&) = delete;
	Mesh& operator= (const Mesh&) = delete;

	Mesh(Mesh&& rhs);
	Mesh& operator= (Mesh&& rhs) noexcept;

	void Bind();
public:
	std::vector<Vertex> m_Vertices;
	std::vector<unsigned int> m_Indices;
private:
	unsigned int VAO, VBO, EBO;
};



