#pragma once
#include <Alias.h>
#include <vector>
#include <unordered_map>
#include <optional>

enum class PlaybackType
{
	Default,
	PingPong,
	Toggle,
};

enum class TransformType
{
	Translation,
	Scale,
	Rotation,
};

struct Animation
{
	Animation() = default;
	~Animation() = default;

	PlaybackType playbackType{};
	TransformType transformType{};

	bool enabled{ true };
	bool loop{ false };

	Mat4 transform{ 1.0f };
	std::optional<float> angle{};
	std::optional<Vec3> axis{};

	float sign{ 1.0f };
	float t{ 0.0f };
	float speed{5.0f};
};

struct AnimationID
{
	AnimationID() = default;
	AnimationID(unsigned int version, unsigned int id)
		: version{ version }, id{ id } {}
	unsigned int version{};
	unsigned int id{};

	bool operator== (const AnimationID& rhs) const
	{
		return id == rhs.id && version == rhs.version;
	}
};

namespace std
{
	template<>
	struct hash<AnimationID>
	{
		size_t operator()(const AnimationID& rhs) const {
			size_t h1 = hash<unsigned int>{}(rhs.version);
			size_t h2 = hash<unsigned int>{}(rhs.id);
			return h1 ^ (h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2));
		}
	};
}

struct AnimationHandler
{
	inline static unsigned int nextID{ 0 };
	AnimationID SetScaleAnimation(const Vec3& scale, 
		PlaybackType playbackType);
	AnimationID SetRotateAnimation(float angle,
		const Vec3& axis, PlaybackType playbackType);
	AnimationID SetTranslateAnimation(const Vec3& pos,
		PlaybackType playbackType);
	AnimationID SetAnimation(const Mat4& mat, 
		PlaybackType playbackType, TransformType transformType);
	void DisableAnimation(AnimationID animationID);
	void DisableAnimation(Animation& animation);

	void SetSpeed(AnimationID id, float speed);
	void SetLoop(AnimationID id, bool loop);

	Mat4 GetTransform(AnimationID id);
	Animation* Find(AnimationID id);
	void Update(float deltaTime);

	// TODO : Make delete and
	// free list to manage animations
	std::vector<Animation> animations{};
	std::unordered_map<AnimationID, size_t> idToIndex{};
};