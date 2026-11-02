#include "Object.h"

#include <glm/gtc/matrix_transform.hpp>
#include <gl/glew.h>

void Object::AddModel(unsigned int parentID, std::string_view fileName,
	std::optional<Vec4> uniformColor)
{
	nodes.emplace_back();
	auto& node = nodes.back();
	node.model = Model{ fileName };
	node.parentID = parentID;
	node.uniformColor = uniformColor;
}
void Object::AddModel(std::string_view fileName, std::optional<Vec4> uniformColor)
{
	AddModel(rootID, fileName, uniformColor);
}


void Object::Render(Shader& shader, const Mat4& view)
{
	shader.Bind();
	std::unordered_map<unsigned int, Mat4> parentWorlds{};

	for (size_t i = 0; i < nodes.size(); ++i)
	{
		auto& node = nodes[i];
		auto it = parentWorlds.find(node.parentID);
		Mat4 inherited = (it == parentWorlds.end()) ?
			Mat4{ 1.0f } : it->second;

		Mat4 local{ inherited };
		Mat4 parent{ inherited };

		for (const auto& transform : node.transforms)
		{
			local *= transform.m;
			if (transform.passToChild)
				parent *= transform.m;

			for (const auto& animation : transform.registeredAnimations)
			{
				Mat4 m = animationHandler.GetTransform(animation);
				local *= m;
				if (transform.passToChild)
					parent *= m;
			}
		}

		parentWorlds.emplace(static_cast<unsigned int>(i), parent);

		// Draw
		shader.SetUniform("model", local);
		shader.SetUniform("normalMatrix", glm::inverse(glm::transpose
		(Mat3{ view * local })));
		if (node.uniformColor)
		{
			// TODO we need to support multiple color types.
			shader.SetUniform("objectColor", Vec3{ *node.uniformColor });
		}
		auto& modelMeshes = node.model.Meshes();
		for (auto& modelMesh : modelMeshes)
		{
			modelMesh.Bind();
			// Render
			glDrawElements(GL_TRIANGLES,
				static_cast<GLsizei>(modelMesh.m_Indices.size()),
				GL_UNSIGNED_INT, (const void*)0);
		}
	}
}

void Object::Update(float dt)
{
	animationHandler.Update(dt);
}

void Object::SetColor(unsigned int nodeID, const Vec3& color)
{
	nodes[nodeID].uniformColor = Vec4(color, 1.0f);
}

void Object::SetColor(unsigned int nodeID, const Vec4& color)
{
	nodes[nodeID].uniformColor = color;
}


unsigned int Object::SetPos(unsigned int nodeID, const Vec3& pos)
{
	return SetTransform(nodeID, glm::translate(Mat4{1.0f}, pos), true);
}

unsigned int Object::SetRotation(unsigned int nodeID, float angle, const Vec3& axis)
{
	return SetTransform(
		nodeID, glm::rotate(Mat4{ 1.0f }, glm::radians(angle), axis),
		true
	);
}

unsigned int Object::SetScale(unsigned int nodeID, const Vec3& scale)
{
	return SetTransform(nodeID,
		glm::scale(Mat4{ 1.0f }, scale),false
	);
}

unsigned int Object::SetTransform(unsigned int nodeID, const Mat4& transform, bool passToChild)
{
	unsigned int id = nodes[nodeID].nextTransformID++;
	nodes[nodeID].transforms.emplace_back(id, transform, passToChild);
	return id;
}

// Naive Implementation
// O(N)
void Object::BindAnimation(unsigned int nodeID, unsigned int transformID, AnimationID id)
{
	// 1. early O(1)
	if (nodeID >= nodes.size()) return;
	// 2. validate animation id
	auto* a = animationHandler.Find(id);
	if (!a) return;
	// 3. validate transform id
	auto& transforms = nodes[nodeID].transforms;
	auto it = std::find_if(transforms.begin(), transforms.end(), [&](const auto& t) {return t.id == transformID;});
	if (it == transforms.end()) return;

	// 4. link animation id to transform
	it->registeredAnimations.push_back(id);
}

void Object::UnbindAnimation(unsigned int nodeID, unsigned int transformID, AnimationID id)
{
	if (nodeID >= nodes.size()) return;

	auto& transforms = nodes[nodeID].transforms;
	auto it = std::find_if(transforms.begin(), transforms.end(), [&](const auto& t) {return t.id == transformID;});
	if (it == transforms.end()) return;

	std::erase(it->registeredAnimations, id);
}
