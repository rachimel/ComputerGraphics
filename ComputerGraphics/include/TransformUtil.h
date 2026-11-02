#pragma once
#include <Alias.h>
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

	inline Mat4 Translate(const Vec3& offset)
	{
		return glm::translate(Mat4(1.0f), offset);
	}

	inline Mat4 Translate(float x = 0.0f, float y = 0.0f, float z = 0.0f)
	{
		return glm::translate(Mat4(1.0f), Vec3(x, y, z));
	}
}