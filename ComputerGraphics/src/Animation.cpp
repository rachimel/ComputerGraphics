#include "Animation.h"

#include <utility>
#include <algorithm>
#include <stdexcept>

#include <glm/gtc/matrix_transform.hpp>

AnimationID AnimationHandler::SetAnimation(const Mat4& mat, PlaybackType playbackType, TransformType transformType,
	AnimationConfig config)
{
	Animation a{};
	a.transform = mat;
	a.playbackType = playbackType;
	a.transformType = transformType;
	a.speed = config.speed;
	a.loop = config.loop;
	animations.push_back(a);

	AnimationID id{1, ++nextID};
	idToIndex.emplace(id, animations.size() - 1);
	return id;
}

bool AnimationHandler::isEnded(AnimationID id)
{
	auto* a = Find(id);
	if (!a) return false;
	return !a->loop && (a->t >= 1.0f);
}

void AnimationHandler::EnableAnimation(AnimationID id)
{
	auto* a = Find(id);
	if (!a) return;
	a->enabled = true;
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

void AnimationHandler::SetT(AnimationID id, float t)
{
	auto* a = Find(id);
	if (!a) return;
	a->t = glm::clamp(t, 0.0f, 1.0f);
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
	{
		Mat4 m = glm::translate(a->transform, a->pivot);
		m = glm::rotate(m, glm::radians(t * *(a->angle)), *a->axis);
		return glm::translate(m, -(a->pivot));
	}
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

			if (animation.loop && animation.playbackType == PlaybackType::Default)
				animation.t = 0.0f;
		}
	}
}
