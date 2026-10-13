#include <Mesh.h>

#include <iostream>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <gl/glew.h>
#include <Shader.h>

Mesh::Mesh(VertexTag tag, std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<Texture> textures)
	:
	m_Layout{tag},
	m_Vertices{std::move(vertices)},
	m_Indices{std::move(indices)},
	m_Textures{std::move(textures)}
{
	CreateVertexObjects();
}

Mesh::~Mesh()
{
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &EBO);
	glDeleteVertexArrays(1, &VAO);
}

Mesh::Mesh(Mesh&& rhs) noexcept
	: VAO{rhs.VAO}, VBO{rhs.VBO}, EBO{rhs.EBO},
	m_Vertices{std::move(rhs.m_Vertices)}, m_Indices{std::move(rhs.m_Indices)}
{
	rhs.VAO = 0;
	rhs.VBO = 0;
	rhs.EBO = 0;
}

Mesh& Mesh::operator=(Mesh&& rhs) noexcept
{
	if (this == &rhs)
	{
		return *this;
	}
	VAO = rhs.VAO;
	VBO = rhs.VBO;
	EBO = rhs.EBO;

	m_Vertices = std::move(rhs.m_Vertices);
	m_Indices = std::move(rhs.m_Indices);

	return *this;
}

void Mesh::CreateVertexObjects()
{
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * m_Vertices.size(), m_Vertices.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * m_Indices.size(), m_Indices.data(), GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<const void*>(offsetof(Vertex, pos)));

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<const void*>(offsetof(Vertex, color)));

	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<const void*>(offsetof(Vertex, normal)));

	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<const void*>(offsetof(Vertex, texCoord)));

	glBindVertexArray(0);
}

void Mesh::Bind()
{
	glBindVertexArray(VAO);
}

void Mesh::UpdateVertices()
{
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * m_Vertices.size(), m_Vertices.data(), GL_STATIC_DRAW);
}