#pragma once
#include <Alias.h>
#include <Shader.h>
#include <Object.h>
#include <array>
#include <cstddef>
#include <string>
#include <stdexcept>
#include <optional>
#include <glm/gtc/matrix_transform.hpp>

namespace CG
{
	/*
	* RotateByPivot
	* float angle : (degree)
	*/
	inline Mat4 RotateByPivot(float angle, const Vec3& pivot, const Vec3& axis)
	{
		Mat4 transform = glm::translate(Mat4(1.0f), pivot);
		transform = glm::rotate(transform, glm::radians(angle), axis);
		return glm::translate(transform, -pivot);
	}
	inline Mat4 ScaleByPivot(const Vec3& scale, const Vec3& pivot)
	{
		Mat4 transform = glm::translate(Mat4(1.0f), pivot);
		transform = glm::scale(transform,scale);
		return glm::translate(transform, -pivot);
	}

	// One Object owns the center node and all six independently animated faces.
	enum class CubeFace : std::size_t
	{
		Center = 0, Right, Left, Top, Bottom, Front, Back, Count
	};

	inline constexpr std::size_t CubeFaceCount =
		static_cast<std::size_t>(CubeFace::Count);
	inline constexpr std::size_t CubeSurfaceCount = CubeFaceCount - 1;

	struct AnimatedCube
	{
		Object object;
		SpawnedNode center;
		std::array<SpawnedNode, CubeSurfaceCount> faces;

	private:
		static SpawnedNode CreateCenter(Object& object)
		{
			return object.GetNode(object.AddEmptyNode());
		}

	public:
		// planeFile: a unit XZ plane centered on the origin, with normal +Y.
		// The Object owns all Nodes and their TRS; handles refer to stable IDs.
		explicit AnimatedCube(
			const std::string& planeFile,
			const std::array<Vec4, CubeSurfaceCount>& colors)
			: object{},
			  center(CreateCenter(object)),
			  faces{{
				  object.Spawn({ .parentID = center.nodeID, .file = planeFile,
					  .color = colors[0], .pos = Vec3{ 0.5f, 0.0f, 0.0f },
					  .angle = -90.0f, .axis = Vec3{ 0.0f, 0.0f, 1.0f } }),
				  object.Spawn({ .parentID = center.nodeID, .file = planeFile,
					  .color = colors[1], .pos = Vec3{ -0.5f, 0.0f, 0.0f },
					  .angle = 90.0f, .axis = Vec3{ 0.0f, 0.0f, 1.0f } }),
				  object.Spawn({ .parentID = center.nodeID, .file = planeFile,
					  .color = colors[2], .pos = Vec3{ 0.0f, 0.5f, 0.0f } }),
				  object.Spawn({ .parentID = center.nodeID, .file = planeFile,
					  .color = colors[3], .pos = Vec3{ 0.0f, -0.5f, 0.0f },
					  .angle = 180.0f, .axis = Vec3{ 1.0f, 0.0f, 0.0f } }),
				  object.Spawn({ .parentID = center.nodeID, .file = planeFile,
					  .color = colors[4], .pos = Vec3{ 0.0f, 0.0f, 0.5f },
					  .angle = 90.0f, .axis = Vec3{ 1.0f, 0.0f, 0.0f } }),
				  object.Spawn({ .parentID = center.nodeID, .file = planeFile,
					  .color = colors[5], .pos = Vec3{ 0.0f, 0.0f, -0.5f },
					  .angle = -90.0f, .axis = Vec3{ 1.0f, 0.0f, 0.0f } })
			  }}
		{
		}

		SpawnedNode& Node(CubeFace face)
		{
			if (face == CubeFace::Center)
				return center;
			return faces.at(static_cast<std::size_t>(face) - 1);
		}

		const SpawnedNode& Node(CubeFace face) const
		{
			if (face == CubeFace::Center)
				return center;
			return faces.at(static_cast<std::size_t>(face) - 1);
		}

		SpawnedNode& Face(CubeFace face)
		{
			if (face == CubeFace::Center)
				throw std::invalid_argument("Center is not a cube face");
			return Node(face);
		}

		const SpawnedNode& Face(CubeFace face) const
		{
			if (face == CubeFace::Center)
				throw std::invalid_argument("Center is not a cube face");
			return Node(face);
		}

		unsigned int NodeID(CubeFace face) const
		{
			return Node(face).nodeID;
		}

		void EditPos(const Vec3& pos)
		{
			center.EditPos(pos);
		}

		void EditRotation(float angle, const Vec3& axis)
		{
			center.EditRotation(angle, axis);
		}

		void EditScale(const Vec3& scale)
		{
			center.EditScale(scale);
		}

		void Update(float deltaTime)
		{
			object.Update(deltaTime);
		}

		void Render(Shader& shader, const Mat4& viewMatrix)
		{
			object.Render(shader, viewMatrix);
		}
	};

	// No animation definitions are created; bind them with the existing API.
	// The prvalue return also works when Object is non-copyable/non-movable.
	inline AnimatedCube CreateAnimatedCube(
		const std::string& planeFile = "data/1_opengl_basics/plane.obj",
		std::array<Vec4, CubeSurfaceCount> colors = {
			Vec4{ 1.0f }, Vec4{ 1.0f }, Vec4{ 1.0f },
			Vec4{ 1.0f }, Vec4{ 1.0f }, Vec4{ 1.0f }
		})
	{
		return AnimatedCube{ planeFile, colors };
	}

	/*
	* Shortchut for lighting shader
	* USE THIS ONLY FOR SPEEDRUN PURPOSE
	*/
	void SetLightingParams(
		std::optional<Shader>& lightingShader,
		const Mat4& view,
		const Mat4& projection,

		const Vec3& lightDirection,
		float ambient,
		float specular,
		int shininess
	)
	{
		lightingShader->Bind();
		lightingShader->SetUniform("lightDir", Vec3{ glm::normalize(view * glm::vec4(lightDirection, 0.0f)) });

		lightingShader->SetUniform("projection", projection);
		lightingShader->SetUniform("view", view);

		lightingShader->SetUniform("ambient", ambient);
		lightingShader->SetUniform("specularStrength", specular);
		lightingShader->SetUniform("shininess", shininess);
	}
}