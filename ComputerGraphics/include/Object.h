#pragma once
#include <utility>
#include <Transform.h>
#include <Model.h>
#include <string>
#include <numeric>
#include <Shader.h>
#include <Timeline.h>
#include <memory>
#include <optional>
#include <string_view>
#include <limits>

constexpr unsigned int rootID = std::numeric_limits<unsigned int>::max();

// Polygon rasterization mode for a single Node; mesh topology remains GL_TRIANGLES.
enum class DrawingMode
{
    Inherit,  // No OpenGL state override (legacy behavior).
    Solid,    // GL_FILL.
    Wireframe, // GL_LINE, for triangle edges.,
	Outline,  // GL_LINE_LOOP
};


// ChildOf appends a child. ParentOf inserts a new parent above parentID.
enum class SpawnRelation { ChildOf, ParentOf };

// Attach a child to a specific point in its parent's Transform list.
enum class TransformAttachment { End, Before, After };

struct SpawnConfig
{
	unsigned int parentID{ rootID };
	std::string file{};
	std::optional<Color4f> color;
	Vec3 pos{ 0.0f };
	float angle{ 0.0f };
	Vec3 axis{0.0f, 1.0f, 0.0f};
	Vec3 scale{ 1.0f };
	DrawingMode drawingMode{ DrawingMode::Inherit };
	SpawnRelation relation{ SpawnRelation::ChildOf };
	TransformAttachment attachment{ TransformAttachment::End };
	std::optional<unsigned int> parentTransformID{};
};

struct SpawnConfigOpaque
{
	unsigned int parentID{ rootID };
	std::string file{};
	std::optional<Color3f> color;
	Vec3 pos{ 0.0f };
	float angle{ 0.0f };
	Vec3 axis{ 0.0f, 1.0f, 0.0f };
	Vec3 scale{ 1.0f };
	DrawingMode drawingMode{ DrawingMode::Inherit };
	SpawnRelation relation{ SpawnRelation::ChildOf };
	TransformAttachment attachment{ TransformAttachment::End };
	std::optional<unsigned int> parentTransformID{};
};

struct NodeTRS
{
	unsigned int t{};
	unsigned int r{};
	unsigned int s{};
};

class Object;

// Node handle. The Object owns the TRS; trs keeps the stable Transform IDs.
struct SpawnedNode
{
	unsigned int nodeID{};
	NodeTRS trs{};
	Object* owner{}; // Non-owning; invalidate when Object is destroyed.

	NodeTRS TRS() const;
	void EditPos(const Vec3& pos) const;
	void EditRotation(float angle, const Vec3& axis) const;
	void EditScale(const Vec3& scale) const;
	void EditTransform(unsigned int transformID, const Mat4& m) const;
	unsigned int InsertBefore(unsigned int anchorID, const Mat4& m, bool passToChild = true) const;
	unsigned int InsertAfter(unsigned int anchorID, const Mat4& m, bool passToChild = true) const;
	void SetParent(unsigned int parentID) const;
	void SetParentAtTransform(unsigned int parentID, unsigned int transformID,
		TransformAttachment attachment) const;
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
		unsigned int parentID{ rootID };
		std::optional<NodeTRS> trs{};
		TransformAttachment attachment{ TransformAttachment::End };
		std::optional<unsigned int> parentTransformID{};

		std::vector<Transform> transforms{};
		std::optional<Vec4> uniformColor{};
		DrawingMode drawingMode{ DrawingMode::Inherit };
	};

public:
	Object() = default;
	~Object() = default;
	Object(const Object&) = delete;
	Object& operator=(const Object&) = delete;
	Object(Object&&) = delete; // No implicit moves while registered Timeline pointers exist.
	Object& operator=(Object&&) = delete;

	unsigned int AddEmptyNode();
	unsigned int AddModel(unsigned int parentID, std::string_view fileName, std::optional<Vec4> uniformColor = std::nullopt);
	unsigned int AddModel(std::string_view fileName, std::optional<Vec4> uniformColor = std::nullopt);

	/* Object::Render()
	* The Shader must be bind before calling this method.
	*/
	void Render(Shader& shader, const Mat4& view);
	void Update(float dt);

	// transformation
	void SetColor(unsigned int nodeID, const Vec3& color);
	void SetColor(unsigned int nodeID, const Vec4& color);
	void SetDrawingMode(unsigned int nodeID, DrawingMode mode);
	void SetDrawingMode(DrawingMode mode); // All current Nodes in this Object.
	DrawingMode GetDrawingMode(unsigned int nodeID) const;

	// returns transformation ID
	NodeTRS SetTRS(unsigned int nodeID, const Vec3& pos,
		float angle, const Vec3& axis, const Vec3& scale);
	NodeTRS SetTRS(unsigned int nodeID, const Vec3& pos,
		const Vec3& scale);
	NodeTRS GetTRS(unsigned int nodeID) const;
	SpawnedNode GetNode(unsigned int nodeID);

	SpawnedNode Spawn(const SpawnConfig& config);
	SpawnedNode SpawnOpaque(const SpawnConfigOpaque& config);

	unsigned int SetTransform(unsigned int nodeID, const Mat4& transform, bool passToChild);
    unsigned int InsertTransformBefore(unsigned int nodeID, unsigned int anchorID,
        const Mat4& transform, bool passToChild = true);
    unsigned int InsertTransformAfter(unsigned int nodeID, unsigned int anchorID,
        const Mat4& transform, bool passToChild = true);
    void SetTransformPropagation(unsigned int nodeID, unsigned int transformID, bool passToChild);
    bool GetTransformPropagation(unsigned int nodeID, unsigned int transformID) const;
    void SetParent(unsigned int nodeID, unsigned int parentID);
    void SetParentAtTransform(unsigned int nodeID, unsigned int parentID,
        unsigned int transformID, TransformAttachment attachment);

	void EditPos(unsigned int nodeID, const Vec3& pos);
	void EditRotation(unsigned int nodeID, float angle, const Vec3& axis);
	void EditScale(unsigned int nodeID, const Vec3& scale);
	// Explicit TRS handles remain supported for old call sites.
	void EditPos(unsigned int nodeID, NodeTRS trs, const Vec3& pos);
	void EditRotation(unsigned int nodeID, NodeTRS trs, float angle, const Vec3& axis);
	void EditScale(unsigned int nodeID, NodeTRS trs, const Vec3& scale);
	void EditTransform(unsigned int nodeID, unsigned int transformID, const Mat4& m);

	// The Object owns only Timeline playback states, not Animation definitions.
	Timeline& CreateTimeline();
	// A Timeline binds its targets automatically in Append().

private:
	friend class Timeline;
	bool CanAttachTimeline(unsigned int nodeID, unsigned int transformID, const Timeline& timeline) const noexcept;
	void AttachTimeline(unsigned int nodeID, unsigned int transformID, const Timeline& timeline) noexcept;
	void DetachTimeline(unsigned int nodeID, unsigned int transformID, const Timeline& timeline) noexcept;

	unsigned int SetPos(unsigned int nodeID, const Vec3& pos);
	unsigned int SetRotation(unsigned int nodeID, float angle, const Vec3& axis);
	unsigned int SetScale(unsigned int nodeID, const Vec3& scale);
	void ValidateParent(unsigned int nodeID, unsigned int parentID) const;
	void ValidateAttachment(unsigned int parentID,
		std::optional<unsigned int> transformID, TransformAttachment attachment) const;
private:
	std::vector<Node> nodes{};
	std::vector<std::unique_ptr<Timeline>> timelines{};
};

