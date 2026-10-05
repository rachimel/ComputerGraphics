#include <Mesh.h>

#include <iostream>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <gl/glew.h>
#include <Shader.h>

Mesh::Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices)
	: 	
	m_Vertices{std::move(vertices)},
	m_Indices{std::move(indices)}
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
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<const void*>(offsetof(Vertex, position)));

#if defined(CG_MESH_ENABLE_COLOR)
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<const void*>(offsetof(Vertex, color)));
#endif
#if defined(CG_MESH_ENABLE_NORMAL)
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<const void*>(offsetof(Vertex, normal)));
#endif
#if defined(CG_MESH_ENABLE_TEXTURE)
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<const void*>(offsetof(Vertex, texCoord)));
#endif
	glBindVertexArray(0);
}

Mesh::~Mesh()
{
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &EBO);
	glDeleteVertexArrays(1, &VAO);
}

Mesh::Mesh(Mesh&& rhs)
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

void Mesh::Bind()
{
	glBindVertexArray(VAO);
}
