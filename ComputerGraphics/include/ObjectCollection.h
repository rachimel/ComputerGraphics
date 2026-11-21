#pragma once

#include <Object.h>
#include <memory>
#include <vector>
#include <string_view>
#include <optional>
#include <cstddef>
#include <utility>
#include <stdexcept>

struct SpawnedObject
{
	Object& object;
	unsigned int nodeID{};
	NodeTRS trs{}; // Stable IDs; Object owns the underlying TRS.

	void EditPos(const Vec3& pos) const { object.EditPos(nodeID, pos); }
	void EditRotation(float angle, const Vec3& axis) const { object.EditRotation(nodeID, angle, axis); }
	void EditScale(const Vec3& scale) const { object.EditScale(nodeID, scale); }
	void EditTransform(unsigned int transformID, const Mat4& m) const
	{ object.EditTransform(nodeID, transformID, m); }
};

class ObjectCollection
{
public:
	Object& Create()
	{
		objects.push_back(std::make_unique<Object>());
		return *objects.back();
	}

	Object& CreateModel(std::string_view path, std::optional<Vec4> color = std::nullopt)
	{
		Object& object = Create();
		object.AddModel(path, color);
		return object;
	}

	SpawnedObject Spawn(const SpawnConfig& config)
	{
		// Collection::Spawn creates a new Object; Node hierarchy stays inside one Object.
		if (config.parentID != rootID || config.relation != SpawnRelation::ChildOf ||
			config.attachment != TransformAttachment::End || config.parentTransformID)
			throw std::invalid_argument("ObjectCollection: use Object::Spawn for hierarchy");
		Object& object = Create();
		auto node = object.Spawn(config);
		return { object, node.nodeID, node.trs };
	}

	SpawnedObject SpawnOpaque(const SpawnConfigOpaque& config)
	{
		if (config.parentID != rootID || config.relation != SpawnRelation::ChildOf ||
			config.attachment != TransformAttachment::End || config.parentTransformID)
			throw std::invalid_argument("ObjectCollection: use Object::SpawnOpaque for hierarchy");
		Object& object = Create();
		auto node = object.SpawnOpaque(config);
		return { object, node.nodeID, node.trs };
	}

	template<class Fn>
	void ForEach(Fn&& fn)
	{
		for (auto& object : objects) fn(*object);
	}

	Object& operator[](size_t index) { return *objects.at(index); }
	const Object& operator[](size_t index) const { return *objects.at(index); }

	void Update(float dt)
	{
		for (auto& object : objects) object->Update(dt);
	}

	void Render(Shader& shader, const Mat4& view)
	{
		for (auto& object : objects) object->Render(shader, view);
	}

	void Clear() { objects.clear(); }
	size_t Size() const { return objects.size(); }

private:
	std::vector<std::unique_ptr<Object>> objects{};
};
