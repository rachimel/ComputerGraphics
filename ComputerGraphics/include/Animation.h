#pragma once
#include <Alias.h>

#include <cstddef>
#include <functional>
#include <unordered_map>
#include <vector>

// One definition can be evaluated by multiple independent Timelines.
enum class TransformType
{
    Translation,
    Scale,
    Rotation,
};

// Used only to determine the default duration when appended to a Timeline.
// 'speed' is not an independently running playback clock.
struct AnimationConfig
{
    float speed{ 5.0f };
};

struct AnimationAngle
{
    float from{ 0.0f }; // Degrees
    float to{ 0.0f };
};
struct Animation
{
    TransformType transformType{};
    Mat4 transform{ 1.0f };
    AnimationAngle angle{};
    Vec3 axis{ 0.0f, 1.0f, 0.0f };
    Vec3 pivot{ 0.0f };
    float defaultDuration{ 0.2f }; // Seconds
};

struct AnimationID
{
    AnimationID() = default;
    AnimationID(unsigned int version, unsigned int id)
        : version{ version }, id{ id } {}

    unsigned int version{};
    unsigned int id{}; // 0 means invalid

    bool operator==(const AnimationID& rhs) const = default;
};

namespace std
{
    template<> struct hash<AnimationID>
    {
        size_t operator()(const AnimationID& rhs) const noexcept
        {
            const size_t h1 = hash<unsigned int>{}(rhs.version);
            const size_t h2 = hash<unsigned int>{}(rhs.id);
            return h1 ^ (h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2));
        }
    };
}

// Global immutable Animation definition registry.
// Playback state (time, loop, speed, play/pause) lives in Timeline, not here.
class AnimationHandler
{
public:
    static AnimationHandler& Global() noexcept;

    AnimationID SetTranslateAnimation(const Vec3& pos, AnimationConfig config = {});
    AnimationID SetScaleAnimation(const Vec3& scale, AnimationConfig config = {});
    AnimationID SetRotateAnimation(AnimationAngle angle, const Vec3& axis,
        AnimationConfig config = {});
    AnimationID SetRotateAnimation(AnimationAngle angle, const Vec3& axis, const Vec3& pivot,
        AnimationConfig config = {});
    // Single angle: rotate from 0 degrees to angle.
    AnimationID SetRotateAnimation(float angle, const Vec3& axis, AnimationConfig config = {})
    { return SetRotateAnimation(AnimationAngle{ 0.0f, angle }, axis, config); }
    AnimationID SetRotateAnimation(float angle, const Vec3& axis, const Vec3& pivot,
        AnimationConfig config = {})
    { return SetRotateAnimation(AnimationAngle{ 0.0f, angle }, axis, pivot, config); }

    const Animation* Find(AnimationID id) const;
    Mat4 GetTransform(AnimationID id, float t) const;

    size_t Size() const noexcept { return animations.size(); }

    AnimationHandler(const AnimationHandler&) = delete;
    AnimationHandler& operator=(const AnimationHandler&) = delete;
    AnimationHandler(AnimationHandler&&) = delete;
    AnimationHandler& operator=(AnimationHandler&&) = delete;

private:
    AnimationHandler() = default;
    AnimationID Register(Animation animation, AnimationConfig config);

    unsigned int nextID{};
    std::vector<Animation> animations{};
    std::unordered_map<AnimationID, size_t> idToIndex{};
};
