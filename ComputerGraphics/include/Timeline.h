#pragma once
#include <Animation.h>

#include <cstddef>
#include <cstdint>
#include <deque>
#include <unordered_map>
#include <vector>

class Object;
class Timeline;

// Matrix order: Before * base Transform * After.
enum class AnimationPlacement
{
    Before,
    After
};

// Inherit uses the target Transform's passToChild flag.
enum class AnimationPropagation
{
    Inherit,
    LocalOnly,
    ToChildren
};

enum class AnimationPlay
{
    Forward,
    Backward,
};

struct TimelineClip
{
    AnimationID id{};
    unsigned int nodeID{};
    unsigned int transformID{};
    size_t targetIndex{};
    float duration{ 1.0f }; // Seconds
    float offset{}; // Seconds from Entry start
    AnimationPlacement placement{ AnimationPlacement::After };
    AnimationPropagation propagation{ AnimationPropagation::Inherit };
};

// Clips in one Entry run together. Entries run sequentially.
// Playback uses the parent Timeline's clock; Entry has no separate clock.
class TimelineEntry
{
public:
    TimelineEntry& Append(AnimationID id, unsigned int nodeID,
        unsigned int transformID, float duration = -1.0f,
        AnimationPlacement placement = AnimationPlacement::After,
        AnimationPropagation propagation = AnimationPropagation::Inherit,
        float offset = 0.0f);
    void Clear();
    void SetInitialT(float t);
    void SetSpeed(float speed);
    void SetDuration(float duration);
    void SetClipPlacement(size_t clipIndex, AnimationPlacement placement);
    void SetClipPropagation(size_t clipIndex, AnimationPropagation propagation);
    void SetClipOffset(size_t clipIndex, float offset);

    const std::vector<TimelineClip>& Clips() const noexcept { return clips; }

    float initialT{}; // Initial normalized progress for each Clip
    float time{}; // Derived: start time on the parent Timeline
    float duration{}; // Seconds; Append expands it, SetDuration may override it
    float speed{ 1.0f }; // Clip playback speed; does not change Entry length
    bool playing{ true }; // False: skip contributions; does not pause the Timeline
    bool loop{ false }; // Repeat individual Clips inside this Entry

public:
    // Used by Timeline::AddEntry(); do not construct entries independently.
    explicit TimelineEntry(Timeline& owner) noexcept : owner{ &owner } {}

private:
    friend class Timeline;
    Timeline* owner{}; // Non-owning
    std::vector<TimelineClip> clips{};
};

// A Timeline belongs to one Object and may animate multiple Nodes/Transforms.
// Each Transform may belong to at most one Timeline.
// Animation definitions live in AnimationHandler::Global().
class Timeline
{
public:
    ~Timeline();

    Timeline(const Timeline&) = delete;
    Timeline& operator=(const Timeline&) = delete;
    Timeline(Timeline&&) = delete;
    Timeline& operator=(Timeline&&) = delete;

    // Append() creates one sequential Entry (legacy call style).
    // AddEntry().Append().Append() creates concurrent Clips.
    Timeline& Append(AnimationID id, unsigned int nodeID,
        unsigned int transformID, float duration = -1.0f,
        AnimationPlacement placement = AnimationPlacement::After,
        AnimationPropagation propagation = AnimationPropagation::Inherit,
        float offset = 0.0f);
    TimelineEntry& AddEntry();
    TimelineEntry& operator[](size_t index) { return entries.at(index); }
    const TimelineEntry& operator[](size_t index) const { return entries.at(index); }
    size_t EntryCount() const noexcept { return entries.size(); }

    void Clear();
    void Update(float dt);
    void SetT(float t); // [0, EntryCount()], one unit per sequential Entry
    void SetTime(float seconds);
    void SetSpeed(float speed);
    void SetInitialT(float t);
    void SetLoop(bool loop) { this->loop = loop; }
    void SetDirection(AnimationPlay value) {
        if(direction != value)
        {
            time = duration - time;
        }
        direction = value; 
    }

    void Play() { playing = true; }
    void Play(size_t index); // Seek to Entry, then play
    void PlayAll() { SetT(0.0f); playing = true; }
    void Pause() { playing = false; }
    void Stop() { playing = false; SetT(initialT); }

	bool isEnded() const {
		if (loop || entries.empty()) return false;

		return direction == AnimationPlay::Forward
			? time >= duration
			: time <= 0.0f;
	}
    bool isPlaying() const { return playing; }
    float GetT() const;
    float GetTime() const noexcept { return time; }
    float GetDuration() const { return duration; }

    // Inspection only. A single matrix cannot encode Before/Base/After placement.
    const Mat4* FindTransform(unsigned int nodeID, unsigned int transformID) const noexcept;
    const Mat4* FindNodeTransform(unsigned int nodeID) const noexcept;

private:
    friend class Object;
    friend class TimelineEntry;
    explicit Timeline(Object& owner) noexcept : owner{ &owner } {}

    struct Target
    {
        unsigned int nodeID{};
        unsigned int transformID{};
        bool passToChild{};
        Mat4 before{ 1.0f };
        Mat4 after{ 1.0f };
        Mat4 inheritedBefore{ 1.0f };
        Mat4 inheritedAfter{ 1.0f };
        Mat4 result{ 1.0f }; // Inspection: Before * After (without base)
    };

    struct NodeEffect
    {
        unsigned int nodeID{};
        Mat4 result{ 1.0f }; // Inspection: contributions in Append order
    };

    static std::uint64_t Key(unsigned int nodeID, unsigned int transformID) noexcept;
    const Target* FindContribution(unsigned int nodeID,
        unsigned int transformID) const noexcept;
    size_t BindTarget(unsigned int nodeID, unsigned int transformID);
    void RebuildTargets();
    void RebuildSchedule();
    void Evaluate();

    Object* owner{}; // Non-owning: Object owns the Timeline
    std::deque<TimelineEntry> entries{}; // Append keeps existing Entry references valid
    std::vector<float> endTimes{};
    std::vector<Target> targets{};
    std::unordered_map<std::uint64_t, size_t> targetToIndex{};
    std::vector<NodeEffect> nodeEffects{};
    std::unordered_map<unsigned int, size_t> nodeToIndex{};

    float time{};
    float duration{};
    float speed{ 1.0f };
    float initialT{};
    bool playing{ true };
    bool loop{ false };
    AnimationPlay direction{ AnimationPlay::Forward };
};
