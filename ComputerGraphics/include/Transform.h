#include <Alias.h>
#include <Animation.h>

struct Transform
{
	Transform() = default;
	Transform(unsigned int id, const Mat4& m, bool passToChild)
		: id{ id }, m{ m }, passToChild{passToChild} {
	}
	Mat4 m{ 1.0f };
	unsigned int id{};
	bool passToChild{};
	std::vector<AnimationID> registeredAnimations{};
};