#pragma once

#include <vector>
#include <unordered_map>
#include <filesystem>

#include <Mesh.h>

#include <glm/glm.hpp>

#include <assimp/scene.h>

struct aiNode;
struct aiScene;

struct aiMesh;
struct aiMaterial;

class Model
{
public:
	Model(const std::filesystem::path& path);
	Model(Model&&) noexcept = default;
	Model& operator= (Model&&) = default; 
	~Model() = default;

	std::vector<Mesh>& Meshes() { return m_Meshes; }
private:
	// Code Referenced from :
	// LearnOpenGL - Ch.4 Model Loading
	// https://learnopengl.com/Model-Loading/Assimp
	void loadModel(const std::filesystem::path& path);
	void processNode(aiNode* node, const aiScene* scene);
	Mesh processMesh(aiMesh* mesh, const aiScene* scene);
	std::vector<Texture> loadMaterialTexture(aiMaterial* material, aiTextureType type, std::string_view typeName);

	std::vector<Mesh> m_Meshes{};
	std::unordered_map<std::filesystem::path,Texture> m_TexturesLoaded{};
	std::filesystem::path m_Directory{};
};

