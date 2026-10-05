#pragma once
#include <Model.h>

#include <assimp/scene.h>

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


Mesh Model::processMesh(aiMesh* mesh, const aiScene* scene)
{
	std::vector<Vertex> vertices;
	std::vector<unsigned int> indices;

	for (unsigned int i = 0; i < mesh->mNumVertices; ++i)
	{
		Vertex vertex;

		glm::vec3 v3;
		v3.x = mesh->mVertices[i].x;
		v3.y = mesh->mVertices[i].y;
		v3.z = mesh->mVertices[i].z;

		vertex.position = v3;
#if defined(CG_MESH_ENABLE_COLOR)
		if (mesh->HasVertexColors(0))
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
#endif
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

#if !defined(CG_MESH_ENABLE_TEXTURE)
	return Mesh{ vertices, indices };
#else
	return Mesh{ vertices, indices, textures};
#endif

}