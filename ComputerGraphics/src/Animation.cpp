#include "Animation.h"

#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

#include <glm/gtc/matrix_transform.hpp>

AnimationHandler& AnimationHandler::Global() noexcept
{
    static AnimationHandler handler{};
    return handler;
}

AnimationID AnimationHandler::Register(Animation animation, AnimationConfig config)
{
    if (!(config.speed > 0.0f) || !std::isfinite(config.speed))
        throw std::invalid_argument("Animation: speed must be finite and > 0");
    if (nextID == std::numeric_limits<unsigned int>::max())
        throw std::overflow_error("Animation: ID space exhausted");

    animation.defaultDuration = 1.0f / config.speed;
    if (!(animation.defaultDuration > 0.0f) || !std::isfinite(animation.defaultDuration))
        throw std::invalid_argument("Animation: resulting duration must be finite and > 0");

    const AnimationID id{ 1, nextID + 1 };
    animations.push_back(std::move(animation));
    try
    {
        idToIndex.emplace(id, animations.size() - 1);
    }
    catch (...)
    {
        animations.pop_back();
        throw;
    }
    ++nextID;
    return id;
}

AnimationID AnimationHandler::SetTranslateAnimation(const Vec3& pos, AnimationConfig config)
{
    if (!std::isfinite(pos.x) || !std::isfinite(pos.y) || !std::isfinite(pos.z))
        throw std::invalid_argument("Animation: translation must be finite");
    Animation animation{};
    animation.transformType = TransformType::Translation;
    animation.transform = glm::translate(Mat4{ 1.0f }, pos);
    return Register(std::move(animation), config);
}

AnimationID AnimationHandler::SetScaleAnimation(const Vec3& scale, AnimationConfig config)
{
    if (!std::isfinite(scale.x) || !std::isfinite(scale.y) || !std::isfinite(scale.z))
        throw std::invalid_argument("Animation: scale must be finite");
    Animation animation{};
    animation.transformType = TransformType::Scale;
    animation.transform = glm::scale(Mat4{ 1.0f }, scale);
    return Register(std::move(animation), config);
}

AnimationID AnimationHandler::SetRotateAnimation(AnimationAngle angle, const Vec3& axis, AnimationConfig config)
{
    return SetRotateAnimation(angle, axis, Vec3{ 0.0f }, config);
}

AnimationID AnimationHandler::SetRotateAnimation(AnimationAngle angle, const Vec3& axis,
    const Vec3& pivot, AnimationConfig config)
{
    if (!std::isfinite(angle.from) || !std::isfinite(angle.to) ||
        !std::isfinite(axis.x) || !std::isfinite(axis.y) || !std::isfinite(axis.z) ||
        !std::isfinite(pivot.x) || !std::isfinite(pivot.y) || !std::isfinite(pivot.z) ||
        !(glm::dot(axis, axis) > 0.0f) ||
        !std::isfinite(glm::dot(axis, axis)))
        throw std::invalid_argument("Animation: invalid rotation axis or angle");

    Animation animation{};
    animation.transformType = TransformType::Rotation;
    animation.angle = angle;
    animation.axis = glm::normalize(axis);
    animation.pivot = pivot;
    return Register(std::move(animation), config);
}

const Animation* AnimationHandler::Find(AnimationID id) const
{
    const auto it = idToIndex.find(id);
    return it == idToIndex.end() ? nullptr : &animations[it->second];
}

Mat4 AnimationHandler::GetTransform(AnimationID id, float t) const
{
    const Animation* animation = Find(id);
    if (!animation) throw std::out_of_range("Animation: invalid AnimationID");
    if (!std::isfinite(t)) throw std::invalid_argument("Animation: t must be finite");
    t = glm::clamp(t, 0.0f, 1.0f);

    const Mat4 identity{ 1.0f };
    switch (animation->transformType)
    {
    case TransformType::Translation:
    case TransformType::Scale:
        return (1.0f - t) * identity + t * animation->transform;
    case TransformType::Rotation:
    {
        Mat4 m = glm::translate(identity, animation->pivot);
        m = glm::rotate(m, glm::radians(glm::mix(animation->angle.from, animation->angle.to, t)), animation->axis);
        return glm::translate(m, -animation->pivot);
    }
    default:
        return identity;
    }
}
