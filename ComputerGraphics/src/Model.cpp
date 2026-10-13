#include <Model.h>

#include <iostream>
#include <print>

#if !defined(STB_IMAGE_IMPLEMENTATION)
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#endif

#include <assimp/Importer.hpp>

#include <assimp/postprocess.h>

#include <gl/glew.h>

unsigned int TextureFromFile(const std::filesystem::path& path)
{
	int width, height, nrChannels;
	unsigned char* texture = stbi_load(path.string().c_str(), &width, &height, &nrChannels, 0);
	if (!texture)
	{
		std::println(std::cerr, "[Error] STB Image : invalid image file : {}", path.string());
		return 0;
	}
	unsigned int textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	GLenum format;
	if (nrChannels == 1)
	{
		format = GL_RED;
	}
	else if (nrChannels == 3)
	{
		format = GL_RGB;
	}
	else if (nrChannels == 4)
	{
		format = GL_RGBA;
	}

	glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, texture);
	glGenerateMipmap(GL_TEXTURE_2D);

	stbi_image_free(texture);
	return textureID;
}

Model::Model(const std::filesystem::path& path)
{
	loadModel(path);
}

void Model::loadModel(const std::filesystem::path& path)
{
	auto readPath = std::filesystem::current_path() / path;
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(readPath.string(), aiProcess_Triangulate);
	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE)
	{
		std::println(std::cerr, "[Error] Assimp : {}", importer.GetErrorString());
		return;
	}
	m_Directory = readPath.parent_path();

	processNode(scene->mRootNode, scene);
}

void Model::processNode(aiNode* node, const aiScene* scene)
{
	for (unsigned int i = 0; i < node->mNumMeshes; ++i)
	{
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		m_Meshes.push_back(processMesh(mesh, scene));
	}

	for (unsigned int i = 0; i < node->mNumChildren; ++i)
	{
		processNode(node->mChildren[i], scene);
	}
}

//
Mesh Model::processMesh(aiMesh* mesh, const aiScene* scene)
{
	VertexTag tag{ VertexTag::None };
	std::vector<Vertex> vertices;
	std::vector<unsigned int> indices;
	std::vector<Texture> textures;

	if (mesh->HasVertexColors(0))
	{
		tag = tag | VertexTag::Color;
	}
	if (mesh->HasNormals())
	{
		tag = tag | VertexTag::Normal;
	}
	if (mesh->HasTextureCoords(0) && (mesh->mMaterialIndex < scene->mNumMaterials))
	{
		tag = tag | VertexTag::Texture;
	}

	for (unsigned int i = 0; i < mesh->mNumVertices; ++i)
	{
		Vertex vertex;

		glm::vec3 v3;
		v3.x = mesh->mVertices[i].x;
		v3.y = mesh->mVertices[i].y;
		v3.z = mesh->mVertices[i].z;

		vertex.pos = v3;
		if (tag & VertexTag::Color)
		{
			v3.r = mesh->mColors[0][i].r;
			v3.g = mesh->mColors[0][i].g;
			v3.b = mesh->mColors[0][i].b;

			vertex.color = v3;
		}
		else
		{
			vertex.color = glm::vec3(1.0f);
		}

		if (tag & VertexTag::Normal)
		{
			v3.x = mesh->mNormals[i].x;
			v3.y = mesh->mNormals[i].y;
			v3.z = mesh->mNormals[i].z;

			vertex.normal = v3;
		}
		else
		{
			vertex.normal = glm::vec3(0.0f);
		}

		if (tag & VertexTag::Texture)
		{
			glm::vec2 v2;
			v2.x = mesh->mTextureCoords[0][i].x;
			v2.y = mesh->mTextureCoords[0][i].y;

			vertex.texCoord = v2;
		}
		else
		{
			vertex.texCoord = glm::vec2(0.0f);
		}
		vertices.push_back(vertex);
	}

	for (unsigned int i = 0; i < mesh->mNumFaces; ++i)
	{
		aiFace face = mesh->mFaces[i];
		for (unsigned int j = 0; j < face.mNumIndices; ++j)
		{
			indices.push_back(face.mIndices[j]);
		}
	}

	if (tag & VertexTag::Texture)
	{
		aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
		std::vector<Texture> diffuseMaps = loadMaterialTexture(material, aiTextureType_DIFFUSE, "diffuseMaps");
		textures.append_range(diffuseMaps);
		std::vector<Texture> specularMaps = loadMaterialTexture(material, aiTextureType_SPECULAR, "specularMaps");
		textures.append_range(specularMaps);
	}
	return Mesh{ tag, vertices, indices, textures };
}

std::vector<Texture> Model::loadMaterialTexture(aiMaterial* material, aiTextureType type, std::string_view typeName)
{
	std::vector<Texture> textures;
	for (unsigned int i = 0; i < material->GetTextureCount(type); ++i)
	{
		aiString str;
		material->GetTexture(type, i, &str);
		auto path = (m_Directory / str.C_Str()).lexically_normal();
		auto it = m_TexturesLoaded.find(path);
		if (it != m_TexturesLoaded.end())
		{
			textures.push_back(it->second);
		}
		else
		{
			Texture texture;
			texture.id = TextureFromFile(path);
			texture.type = typeName;
			textures.push_back(texture);
			m_TexturesLoaded.emplace(path, texture);
		}
	}
	return textures;
}
