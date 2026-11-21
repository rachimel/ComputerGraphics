#include "Object.h"

#include <glm/gtc/matrix_transform.hpp>
#include <gl/glew.h>
#include <algorithm>
#include <stdexcept>
#include <unordered_map>
#include <iterator>
#include <limits>
#include <cstdint>

namespace
{
// Only query/change polygon state for explicit Solid/Wireframe overrides.
// Inherit-only Objects keep the legacy fast path and never query GL state.
class ScopedPolygonMode
{
public:
    explicit ScopedPolygonMode(bool enabled) : enabled{ enabled }
    {
        if (enabled) glGetIntegerv(GL_POLYGON_MODE, previous);
    }

    ScopedPolygonMode(const ScopedPolygonMode&) = delete;
    ScopedPolygonMode& operator=(const ScopedPolygonMode&) = delete;

    void Set(DrawingMode mode)
    {
        if (!enabled) return;
        if (mode == DrawingMode::Inherit)
        {
            Restore();
            return;
        }
        const GLenum next = mode == DrawingMode::Wireframe ? GL_LINE : GL_FILL;
        if (next != current)
        {
            glPolygonMode(GL_FRONT_AND_BACK, next);
            current = next;
        }
    }

    ~ScopedPolygonMode() { Restore(); }
private:
    void Restore()
    {
        if (!enabled || current == 0) return;
        if (previous[0] == previous[1])
            glPolygonMode(GL_FRONT_AND_BACK, static_cast<GLenum>(previous[0]));
        else
        {
            glPolygonMode(GL_FRONT, static_cast<GLenum>(previous[0]));
            glPolygonMode(GL_BACK, static_cast<GLenum>(previous[1]));
        }
        current = 0;
    }

    bool enabled{};
    GLint previous[2]{ GL_FILL, GL_FILL };
    GLenum current{};
};
}


unsigned int Object::AddEmptyNode()
{
    nodes.emplace_back();
    const auto id = static_cast<unsigned int>(nodes.size() - 1);
    // Every Node starts with an editable identity TRS.
    SetTRS(id, Vec3{ 0.0f }, 0.0f, Vec3{ 0.0f, 1.0f, 0.0f }, Vec3{ 1.0f });
    return id;
}

unsigned int Object::AddModel(unsigned int parentID, std::string_view fileName,
    std::optional<Vec4> uniformColor)
{
    if (parentID != rootID && parentID >= nodes.size())
        throw std::out_of_range("Object: invalid parent Node");
    const auto id = AddEmptyNode();
    auto& node = nodes[id];
    node.model = Model{ fileName };
    node.parentID = parentID;
    node.uniformColor = uniformColor;
    return id;
}

unsigned int Object::AddModel(std::string_view fileName, std::optional<Vec4> uniformColor)
{
    return AddModel(rootID, fileName, uniformColor);
}

void Object::Render(Shader& shader, const Mat4& view)
{
    shader.Bind();
    const bool hasExplicitMode = std::any_of(nodes.begin(), nodes.end(),
        [](const Node& n) { return n.drawingMode != DrawingMode::Inherit; });
    ScopedPolygonMode polygonMode{ hasExplicitMode };

    struct NodeWorld
    {
        Mat4 local{ 1.0f };
        Mat4 inherited{ 1.0f };
        std::vector<Mat4> stages{}; // Inherited matrix before/after each parent slot.
        std::uint8_t state{}; // 0: untouched, 1: visiting, 2: resolved
    };
    std::vector<NodeWorld> worlds(nodes.size());
    std::vector<std::uint8_t> needsStages(nodes.size(), 0);
    for (const auto& node : nodes)
        if (node.parentID != rootID && node.parentTransformID)
            needsStages[node.parentID] = 1;

    // Parents may be created after their children (SpawnRelation::ParentOf).
    const auto resolve = [&](auto&& self, unsigned int id) -> void
    {
        auto& result = worlds[id];
        if (result.state == 2) return;
        if (result.state == 1)
            throw std::logic_error("Object: cyclic Node hierarchy");
        result.state = 1;
        const auto& node = nodes[id];

        Mat4 base{ 1.0f };
        if (node.parentID != rootID)
        {
            self(self, node.parentID);
            const auto& parent = nodes[node.parentID];
            const auto& parentWorld = worlds[node.parentID];
            base = parentWorld.inherited;
            if (node.parentTransformID)
            {
                const auto it = std::find_if(parent.transforms.begin(), parent.transforms.end(),
                    [&](const Transform& t) { return t.id == *node.parentTransformID; });
                if (it == parent.transforms.end())
                    throw std::logic_error("Object: detached parent Transform");
                const size_t index = static_cast<size_t>(it - parent.transforms.begin());
                base = parentWorld.stages[index +
                    (node.attachment == TransformAttachment::After ? 1 : 0)];
            }
        }

        result.local = base;
        result.inherited = base;
        // No per-Transform prefix cache unless a child attaches to one.
        if (needsStages[id])
        {
            result.stages.reserve(node.transforms.size() + 1);
            result.stages.push_back(base);
        }
        for (const auto& transform : node.transforms)
        {
            // Keep each animation at its Transform slot.
            const Timeline::Target* effect = transform.timeline
                ? transform.timeline->FindContribution(id, transform.id)
                : nullptr;
            if (effect) result.local *= effect->before;
            result.local *= transform.m;
            if (effect) result.local *= effect->after;

            if (effect) result.inherited *= effect->inheritedBefore;
            if (transform.passToChild) result.inherited *= transform.m;
            if (effect) result.inherited *= effect->inheritedAfter;
            if (needsStages[id]) result.stages.push_back(result.inherited);
        }
        result.state = 2;
    };

    for (unsigned int i = 0; i < nodes.size(); ++i)
        resolve(resolve, i);

    for (unsigned int i = 0; i < nodes.size(); ++i)
    {
        auto& node = nodes[i];
        const auto& local = worlds[i].local;
        polygonMode.Set(node.drawingMode);
        shader.SetUniform("model", local);
        shader.SetUniform("normalMatrix", glm::inverse(glm::transpose(Mat3{ view * local })));
        if (node.uniformColor)
            shader.SetUniform("objectColor", Vec3{ *node.uniformColor });

        // Draw each Node in insertion order; parenting does not change draw order.
        auto& modelMeshes = node.model.Meshes();
        for (auto& mesh : modelMeshes)
        {
            mesh.Bind();
            if (node.drawingMode == DrawingMode::Outline)
            {
				glDrawElements(GL_LINE_LOOP, static_cast<GLsizei>(mesh.m_Indices.size()),
					GL_UNSIGNED_INT, (const void*)0);
            }
            else
            {
				glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(mesh.m_Indices.size()),
					GL_UNSIGNED_INT, (const void*)0);
            }
        }
    }
}

void Object::Update(float dt)
{
	for (auto& timeline : timelines)
		timeline->Update(dt);
}

void Object::SetColor(unsigned int nodeID, const Vec3& color)
{
	nodes[nodeID].uniformColor = Vec4(color, 1.0f);
}

void Object::SetColor(unsigned int nodeID, const Vec4& color)
{
	nodes[nodeID].uniformColor = color;
}

void Object::SetDrawingMode(unsigned int nodeID, DrawingMode mode)
{
    nodes.at(nodeID).drawingMode = mode;
}

void Object::SetDrawingMode(DrawingMode mode)
{
    for (auto& node : nodes) node.drawingMode = mode;
}

DrawingMode Object::GetDrawingMode(unsigned int nodeID) const
{
    return nodes.at(nodeID).drawingMode;
}

NodeTRS Object::SetTRS(unsigned int nodeID, const Vec3& pos,
    float angle, const Vec3& axis, const Vec3& scale)
{
    auto& node = nodes.at(nodeID);
    if (!node.trs)
    {
        const NodeTRS ids{
            SetPos(nodeID, pos), SetRotation(nodeID, angle, axis), SetScale(nodeID, scale)
        };
        node.trs = ids;
    }
    else
    {
        EditPos(nodeID, *node.trs, pos);
        EditRotation(nodeID, *node.trs, angle, axis);
        EditScale(nodeID, *node.trs, scale);
    }
    return *node.trs;
}

NodeTRS Object::SetTRS(unsigned int nodeID, const Vec3& pos, const Vec3& scale)
{
    return SetTRS(nodeID, pos, 0.0f, Vec3{ 0.0f, 1.0f, 0.0f }, scale);
}

NodeTRS Object::GetTRS(unsigned int nodeID) const
{
    const auto& node = nodes.at(nodeID);
    if (!node.trs) throw std::logic_error("Object: Node has no TRS");
    return *node.trs;
}

SpawnedNode Object::GetNode(unsigned int nodeID)
{
    return { nodeID, GetTRS(nodeID), this };
}

void Object::ValidateAttachment(unsigned int parentID,
    std::optional<unsigned int> transformID, TransformAttachment attachment) const
{
    if (attachment == TransformAttachment::End)
    {
        if (transformID)
            throw std::invalid_argument("Object: End attachment cannot specify a Transform");
        return;
    }
    if (parentID == rootID || !transformID || parentID >= nodes.size())
        throw std::invalid_argument("Object: attachment requires a parent Transform");
    const auto& transforms = nodes[parentID].transforms;
    if (std::none_of(transforms.begin(), transforms.end(),
        [&](const Transform& t) { return t.id == *transformID; }))
        throw std::out_of_range("Object: unknown parent Transform");
}

void Object::ValidateParent(unsigned int nodeID, unsigned int parentID) const
{
    if (nodeID >= nodes.size() || (parentID != rootID && parentID >= nodes.size()))
        throw std::out_of_range("Object: unknown Node");
    for (auto current = parentID; current != rootID; current = nodes[current].parentID)
        if (current == nodeID)
            throw std::invalid_argument("Object: cyclic Node hierarchy");
}

SpawnedNode Object::Spawn(const SpawnConfig& config)
{
    if (config.relation == SpawnRelation::ParentOf && config.parentID == rootID)
        throw std::invalid_argument("Object: ParentOf requires an existing Node");
    if (config.parentID != rootID && config.parentID >= nodes.size())
        throw std::out_of_range("Object: unknown parent/anchor Node");

    // ParentOf: the new Node takes the old parent and its attachment point.
    const unsigned int parentID = config.relation == SpawnRelation::ParentOf
        ? nodes[config.parentID].parentID : config.parentID;
    auto attachment = config.attachment;
    auto anchor = config.parentTransformID;
    if (config.relation == SpawnRelation::ParentOf && attachment == TransformAttachment::End && !anchor)
    {
        attachment = nodes[config.parentID].attachment;
        anchor = nodes[config.parentID].parentTransformID;
    }
    ValidateAttachment(parentID, anchor, attachment);

    const auto id = AddModel(parentID, config.file, config.color);
    auto& node = nodes[id];
    node.attachment = attachment;
    node.parentTransformID = anchor;
    SetDrawingMode(id, config.drawingMode);
    SetTRS(id, config.pos, config.angle, config.axis, config.scale);
    if (config.relation == SpawnRelation::ParentOf)
    {
        nodes[config.parentID].parentID = id;
        nodes[config.parentID].parentTransformID.reset();
        nodes[config.parentID].attachment = TransformAttachment::End;
    }
    return GetNode(id);
}

SpawnedNode Object::SpawnOpaque(const SpawnConfigOpaque& config)
{
    SpawnConfig converted{
        .parentID = config.parentID, .file = config.file,
        .color = config.color ? std::optional<Color4f>{ Vec4{ *config.color, 1.0f } }
                              : std::nullopt,
        .pos = config.pos, .angle = config.angle, .axis = config.axis,
        .scale = config.scale, .drawingMode = config.drawingMode,
        .relation = config.relation, .attachment = config.attachment,
        .parentTransformID = config.parentTransformID
    };
    return Spawn(converted);
}

unsigned int Object::SetPos(unsigned int nodeID, const Vec3& pos)
{
	return SetTransform(nodeID, glm::translate(Mat4{1.0f}, pos), true);
}

unsigned int Object::SetRotation(unsigned int nodeID, float angle, const Vec3& axis)
{
	return SetTransform(
		nodeID, glm::rotate(Mat4{ 1.0f }, glm::radians(angle), axis),
		true
	);
}

unsigned int Object::SetScale(unsigned int nodeID, const Vec3& scale)
{
	return SetTransform(nodeID,
		glm::scale(Mat4{ 1.0f }, scale),false
	);
}

unsigned int Object::SetTransform(unsigned int nodeID, const Mat4& transform, bool passToChild)
{
	auto& node = nodes.at(nodeID);
    if (node.nextTransformID == std::numeric_limits<unsigned int>::max())
        throw std::overflow_error("Object: Transform ID exhausted");
    const unsigned int id = node.nextTransformID;
    node.transforms.emplace_back(id, transform, passToChild);
    ++node.nextTransformID;
	return id;
}


unsigned int Object::InsertTransformBefore(unsigned int nodeID, unsigned int anchorID,
    const Mat4& transform, bool passToChild)
{
    auto& node = nodes.at(nodeID);
    auto it = std::find_if(node.transforms.begin(), node.transforms.end(),
        [anchorID](const Transform& t) { return t.id == anchorID; });
    if (it == node.transforms.end())
        throw std::out_of_range("Object: unknown anchor Transform");
    if (node.nextTransformID == std::numeric_limits<unsigned int>::max())
        throw std::overflow_error("Object: Transform ID exhausted");
    const unsigned int id = node.nextTransformID;
    node.transforms.emplace(it, id, transform, passToChild);
    ++node.nextTransformID;
    return id;
}

unsigned int Object::InsertTransformAfter(unsigned int nodeID, unsigned int anchorID,
    const Mat4& transform, bool passToChild)
{
    auto& node = nodes.at(nodeID);
    auto it = std::find_if(node.transforms.begin(), node.transforms.end(),
        [anchorID](const Transform& t) { return t.id == anchorID; });
    if (it == node.transforms.end())
        throw std::out_of_range("Object: unknown anchor Transform");
    if (node.nextTransformID == std::numeric_limits<unsigned int>::max())
        throw std::overflow_error("Object: Transform ID exhausted");
    const unsigned int id = node.nextTransformID;
    node.transforms.emplace(std::next(it), id, transform, passToChild);
    ++node.nextTransformID;
    return id;
}

void Object::SetTransformPropagation(unsigned int nodeID, unsigned int transformID, bool passToChild)
{
    auto& transforms = nodes.at(nodeID).transforms;
    auto it = std::find_if(transforms.begin(), transforms.end(),
        [transformID](const Transform& t) { return t.id == transformID; });
    if (it == transforms.end()) throw std::out_of_range("Object: unknown Transform");
    it->passToChild = passToChild;
}

bool Object::GetTransformPropagation(unsigned int nodeID, unsigned int transformID) const
{
    const auto& transforms = nodes.at(nodeID).transforms;
    auto it = std::find_if(transforms.begin(), transforms.end(),
        [transformID](const Transform& t) { return t.id == transformID; });
    if (it == transforms.end()) throw std::out_of_range("Object: unknown Transform");
    return it->passToChild;
}

void Object::SetParent(unsigned int nodeID, unsigned int parentID)
{
    ValidateParent(nodeID, parentID);
    auto& node = nodes[nodeID];
    node.parentID = parentID;
    node.parentTransformID.reset();
    node.attachment = TransformAttachment::End;
}

void Object::SetParentAtTransform(unsigned int nodeID, unsigned int parentID,
    unsigned int transformID, TransformAttachment attachment)
{
    ValidateParent(nodeID, parentID);
    if (attachment == TransformAttachment::End)
        throw std::invalid_argument("Object: use SetParent for End attachment");
    ValidateAttachment(parentID, transformID, attachment);
    auto& node = nodes[nodeID];
    node.parentID = parentID;
    node.parentTransformID = transformID;
    node.attachment = attachment;
}

void Object::EditPos(unsigned int nodeID, const Vec3& pos)
{
    EditPos(nodeID, GetTRS(nodeID), pos);
}

void Object::EditRotation(unsigned int nodeID, float angle, const Vec3& axis)
{
    EditRotation(nodeID, GetTRS(nodeID), angle, axis);
}

void Object::EditScale(unsigned int nodeID, const Vec3& scale)
{
    EditScale(nodeID, GetTRS(nodeID), scale);
}

void Object::EditPos(unsigned int nodeID, NodeTRS trs, const Vec3& pos)
{
	EditTransform(nodeID, trs.t, glm::translate(Mat4{ 1.0f }, pos));
}


void Object::EditRotation(unsigned int nodeID, NodeTRS trs, float angle, const Vec3& axis)
{
	EditTransform(nodeID, trs.r, glm::rotate(Mat4{1.0f}, glm::radians(angle), axis));
}


void Object::EditScale(unsigned int nodeID, NodeTRS trs, const Vec3& scale)
{
	EditTransform(nodeID, trs.s, glm::scale(Mat4{ 1.0f }, scale));
}

void Object::EditTransform(unsigned int nodeID, unsigned int transformID, const Mat4& m)
{
	if (nodeID >= nodes.size()) return;

	auto& transforms = nodes[nodeID].transforms;
	auto it = std::find_if(transforms.begin(), transforms.end(), [&](const auto& t) {return t.id == transformID;});
	if (it == transforms.end()) return;

	it->m = m;
}

Timeline& Object::CreateTimeline()
{
	// Object owns the Timeline; the Timeline can safely resolve its Node/Transform IDs.
	auto timeline = std::unique_ptr<Timeline>{ new Timeline{ *this } };
	timelines.push_back(std::move(timeline));
	Timeline& ref = *(timelines.back());
	return ref;
}


bool Object::CanAttachTimeline(unsigned int nodeID, unsigned int transformID,
    const Timeline& timeline) const noexcept
{
    if (nodeID >= nodes.size()) return false;
    const auto& transforms = nodes[nodeID].transforms;
    const auto it = std::find_if(transforms.begin(), transforms.end(),
        [&](const Transform& t) { return t.id == transformID; });
    return it != transforms.end() && (!it->timeline || it->timeline == &timeline);
}

void Object::AttachTimeline(unsigned int nodeID, unsigned int transformID,
    const Timeline& timeline) noexcept
{
    // Called only after CanAttachTimeline() has succeeded.
    for (auto& t : nodes[nodeID].transforms)
        if (t.id == transformID) { t.timeline = &timeline; return; }
}

void Object::DetachTimeline(unsigned int nodeID, unsigned int transformID,
    const Timeline& timeline) noexcept
{
    if (nodeID >= nodes.size()) return;
    for (auto& t : nodes[nodeID].transforms)
        if (t.id == transformID && t.timeline == &timeline)
        { t.timeline = nullptr; return; }
}

NodeTRS SpawnedNode::TRS() const
{
    if (!owner) throw std::logic_error("SpawnedNode: no owning Object");
    return owner->GetTRS(nodeID);
}

void SpawnedNode::EditPos(const Vec3& pos) const
{
    if (!owner) throw std::logic_error("SpawnedNode: no owning Object");
    owner->EditPos(nodeID, pos);
}

void SpawnedNode::EditRotation(float angle, const Vec3& axis) const
{
    if (!owner) throw std::logic_error("SpawnedNode: no owning Object");
    owner->EditRotation(nodeID, angle, axis);
}

void SpawnedNode::EditScale(const Vec3& scale) const
{
    if (!owner) throw std::logic_error("SpawnedNode: no owning Object");
    owner->EditScale(nodeID, scale);
}

void SpawnedNode::EditTransform(unsigned int transformID, const Mat4& m) const
{
    if (!owner) throw std::logic_error("SpawnedNode: no owning Object");
    owner->EditTransform(nodeID, transformID, m);
}

unsigned int SpawnedNode::InsertBefore(unsigned int anchorID, const Mat4& m,
    bool passToChild) const
{
    if (!owner) throw std::logic_error("SpawnedNode: no owning Object");
    return owner->InsertTransformBefore(nodeID, anchorID, m, passToChild);
}

unsigned int SpawnedNode::InsertAfter(unsigned int anchorID, const Mat4& m,
    bool passToChild) const
{
    if (!owner) throw std::logic_error("SpawnedNode: no owning Object");
    return owner->InsertTransformAfter(nodeID, anchorID, m, passToChild);
}

void SpawnedNode::SetParent(unsigned int parentID) const
{
    if (!owner) throw std::logic_error("SpawnedNode: no owning Object");
    owner->SetParent(nodeID, parentID);
}

void SpawnedNode::SetParentAtTransform(unsigned int parentID,
    unsigned int transformID, TransformAttachment attachment) const
{
    if (!owner) throw std::logic_error("SpawnedNode: no owning Object");
    owner->SetParentAtTransform(nodeID, parentID, transformID, attachment);
}
