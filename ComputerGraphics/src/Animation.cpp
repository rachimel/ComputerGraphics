#include "Animation.h"

#include <utility>
#include <algorithm>
#include <stdexcept>

#include <glm/gtc/matrix_transform.hpp>

AnimationID AnimationHandler::SetScaleAnimation(const Vec3& scale, PlaybackType playbackType)
{
	return SetAnimation(glm::scale(Mat4{ 1.0f }, scale), playbackType, TransformType::Scale);
}

AnimationID AnimationHandler::SetRotateAnimation(float angle, const Vec3& axis, PlaybackType playbackType)
{
	AnimationID id = SetAnimation(Mat4{1.0f}, playbackType, TransformType::Rotation);
	auto* a = Find(id);
	a->axis = axis;
	a->angle = angle;
	return id;
}

AnimationID AnimationHandler::SetTranslateAnimation(const Vec3& pos, PlaybackType playbackType)
{
	return SetAnimation(glm::translate(Mat4{ 1.0f }, pos), playbackType, TransformType::Translation);
}
AnimationID AnimationHandler::SetAnimation(const Mat4& mat, PlaybackType playbackType, TransformType transformType)
{
	Animation a{};
	a.transform = mat;
	a.playbackType = playbackType;
	a.transformType = transformType;
	animations.push_back(a);

	AnimationID id{1, ++nextID};
	idToIndex.emplace(id, animations.size() - 1);
	return id;
}

void AnimationHandler::DisableAnimation(AnimationID id)
{
	auto* a = Find(id);
	if (!a) return;
	a->enabled = false;
}

void AnimationHandler::DisableAnimation(Animation& animation)
{
	if (!animation.enabled) return;
	animation.enabled = false;
}

void AnimationHandler::SetSpeed(AnimationID id, float speed)
{
	auto* a = Find(id);
	if (!a) return;
	a->speed = speed;
}

void AnimationHandler::SetLoop(AnimationID id, bool loop)
{
	auto* a = Find(id);
	if (!a) return;
	a->loop = loop;
}

Mat4 AnimationHandler::GetTransform(AnimationID id)
{
	Mat4 m{ 1.0f };
	auto* a = Find(id);
	if (!a) return m;
	
	float t = a->t;
	switch (a->transformType)
	{
	case TransformType::Translation:
	case TransformType::Scale:
		return (1.0f - t) * m + t * a->transform;
	case TransformType::Rotation:
		return glm::rotate(a->transform, glm::radians(t * *(a->angle)), *a->axis);
	default:
		return m;
	}
}

Animation* AnimationHandler::Find(AnimationID id)
{
	auto it = idToIndex.find(id);
	if (it == idToIndex.end()) return nullptr;
	return &(animations[it->second]);
}

void AnimationHandler::Update(float deltaTime)
{
	for (auto& animation : animations)
	{
		if (!animation.enabled) continue;
		animation.t = glm::clamp(animation.t + (animation.sign * animation.speed * deltaTime), 0.0f, 1.0f);
		if (animation.t >= 1.0f || animation.t <= 0.0f)
		{
			if (animation.playbackType == PlaybackType::PingPong || animation.playbackType == PlaybackType::Toggle)
				animation.sign *= -1.0f;
			if (!animation.loop || animation.playbackType == PlaybackType::Toggle)
				DisableAnimation(animation);
			if (animation.loop && animation.playbackType == PlaybackType::Default)
				animation.t = 0.0f;
		}
	}
}
