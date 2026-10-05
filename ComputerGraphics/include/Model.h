#pragma once

#include <vector>
#include <filesystem>

#include <Mesh.h>

#include <glm/glm.hpp>

struct aiNode;
struct aiScene;

struct aiMesh;
#if defined(CG_MESH_ENABLE_TEXTURE)
struct aiMaterial;
#endif

class Model
{
public:
	Model(const std::filesystem::path& path);
	~Model() = default;

	std::vector<Mesh>& Meshes() { return m_Meshes; }
private:
	void loadModel(const std::filesystem::path& path);
	void processNode(aiNode* node, const aiScene* scene);
	Mesh processMesh(aiMesh* mesh, const aiScene* scene);

	std::vector<Mesh> m_Meshes;
	std::filesystem::path m_Directory;
};

