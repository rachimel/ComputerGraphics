#pragma once
#include <Transform.h>
#include <Animation.h>
#include <Model.h>
#include <string>
#include <numeric>
#include <Shader.h>

constexpr unsigned int rootID = std::numeric_limits<unsigned int>::max();

struct ObjectTRS
{
	unsigned int t{};
	unsigned int r{};
	unsigned int s{};
};

class Object
{
	struct Node
	{
		unsigned int nextTransformID{ 0 };
		Node() = default;
		Node(Node&&) noexcept = default;
		Node& operator= (Node&&) noexcept = default;

		Model model{};
		// transformation in global reference frame.
		Mat4 parentWorld{1.0f};
		unsigned int parentID{};

		std::vector<Transform> transforms{};
		std::optional<Vec4> uniformColor{};
	};
public:
	Object() = default;
	~Object() = default;

	void AddModel(unsigned int parentID, std::string_view fileName, std::optional<Vec4> uniformColor = std::nullopt);
	void AddModel(std::string_view fileName, std::optional<Vec4> uniformColor = std::nullopt);
	/* Object::Render()
	* The Shader must be bind before calling this method.
	*/
	void Render(Shader& shader, const Mat4& view);
	void Update(float dt);

	// transformation
	void SetColor(unsigned int nodeID, const Vec3& color);
	void SetColor(unsigned int nodeID, const Vec4& color);

	// returns transformation ID
	ObjectTRS SetTRS(unsigned int nodeID, const Vec3& pos,
		float angle, const Vec3& axis, const Vec3& scale);
	ObjectTRS SetTRS(unsigned int nodeID, const Vec3& pos,
		const Vec3& scale);

	unsigned int SetPos(unsigned int nodeID, const Vec3& pos);
	unsigned int SetRotation(unsigned int nodeID, float angle, const Vec3& axis);
	unsigned int SetScale(unsigned int nodeID, const Vec3& scale);

	unsigned int SetTransform(unsigned int nodeID, const Mat4& transform, bool passToChild);
	void EditTransform(unsigned int nodeID, unsigned int transformID, const Mat4& m);

	void EnableAnimation(AnimationID id);
	void DisableAnimation(AnimationID id);

	AnimationID SetScaleAnimation(const Vec3& scale,
		PlaybackType playbackType, AnimationConfig config = {});
	AnimationID SetRotateAnimation(float angle,
		const Vec3& axis, const Vec3& pivot, PlaybackType playbackType,
		AnimationConfig config);

	AnimationID SetRotateAnimation(float angle,
		const Vec3& axis, PlaybackType playbackType, AnimationConfig config = {});
	AnimationID SetTranslateAnimation(const Vec3& pos,
		PlaybackType playbackType, AnimationConfig config = {});

	bool isAnimationEnded(AnimationID id);
	void SetAnimationT(AnimationID id, float t);

	AnimationID BindAnimation(unsigned int nodeID, unsigned int transformID, AnimationID id);
	void UnbindAnimation(unsigned int nodeID, unsigned int transformID, AnimationID id);

	AnimationHandler animationHandler{};
private:
	std::vector<Node> nodes{};
};

