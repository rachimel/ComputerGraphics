#include <Timeline.h>
#include <Object.h>

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

std::uint64_t Timeline::Key(unsigned int nodeID, unsigned int transformID) noexcept
{
    return (static_cast<std::uint64_t>(nodeID) << 32) | transformID;
}

Timeline::~Timeline()
{
    Clear();
}

void TimelineEntry::SetInitialT(float t)
{
    if (!std::isfinite(t) || t < 0.0f || t > 1.0f)
        throw std::invalid_argument("TimelineEntry: initialT must be in [0, 1]");
    initialT = t;
    owner->Evaluate();
}

void TimelineEntry::SetSpeed(float value)
{
    if (!(value >= 0.0f) || !std::isfinite(value))
        throw std::invalid_argument("TimelineEntry: speed must be finite and >= 0");
    speed = value;
    owner->Evaluate();
}

void TimelineEntry::SetDuration(float value)
{
    if (!(value >= 0.0f) || !std::isfinite(value))
        throw std::invalid_argument("TimelineEntry: duration must be finite and >= 0");
    duration = value;
    owner->Evaluate();
}

void TimelineEntry::SetClipPlacement(size_t clipIndex, AnimationPlacement placement)
{
    clips.at(clipIndex).placement = placement;
    owner->Evaluate();
}

void TimelineEntry::SetClipPropagation(size_t clipIndex, AnimationPropagation propagation)
{
    clips.at(clipIndex).propagation = propagation;
    owner->Evaluate();
}

void TimelineEntry::SetClipOffset(size_t clipIndex, float offset)
{
    if (!(offset >= 0.0f) || !std::isfinite(offset))
        throw std::invalid_argument("TimelineEntry: invalid Clip offset");
    auto& clip = clips.at(clipIndex);
    if (!std::isfinite(offset + clip.duration))
        throw std::invalid_argument("TimelineEntry: Clip end time overflow");
    clip.offset = offset;
    duration = std::max(duration, offset + clip.duration);
    owner->Evaluate();
}

TimelineEntry& TimelineEntry::Append(AnimationID id, unsigned int nodeID,
    unsigned int transformID, float clipDuration,
    AnimationPlacement placement, AnimationPropagation propagation, float offset)
{
    const Animation* animation = AnimationHandler::Global().Find(id);
    if (!animation) throw std::invalid_argument("TimelineEntry: invalid AnimationID");

    if (clipDuration < 0.0f) clipDuration = animation->defaultDuration;
    if (!(clipDuration > 0.0f) || !std::isfinite(clipDuration) ||
        !(offset >= 0.0f) || !std::isfinite(offset) ||
        !std::isfinite(offset + clipDuration))
        throw std::invalid_argument("TimelineEntry: invalid duration or offset");
    // BindTarget checks ownership and Transform validity.
    const size_t targetIndex = owner->BindTarget(nodeID, transformID);
    // BindTarget() is idempotent. If a later allocation throws, the entry is
    // unchanged, but the unused binding remains until Clear().
    clips.push_back({ id, nodeID, transformID, targetIndex,
        clipDuration, offset, placement, propagation });
    duration = std::max(duration, offset + clipDuration);
    owner->Evaluate();
    return *this;
}

void TimelineEntry::Clear()
{
    clips.clear();
    duration = 0.0f;
    owner->RebuildTargets();
    owner->Evaluate();
}

TimelineEntry& Timeline::AddEntry()
{
    entries.emplace_back(*this);
    Evaluate();
    return entries.back();
}

Timeline& Timeline::Append(AnimationID id, unsigned int nodeID,
    unsigned int transformID, float clipDuration,
    AnimationPlacement placement, AnimationPropagation propagation, float offset)
{
    // Preserve the old sequential Append API.
    AddEntry();
    try
    {
        entries.back().Append(id, nodeID, transformID, clipDuration,
            placement, propagation, offset);
    }
    catch (...)
    {
        entries.pop_back();
        RebuildTargets();
        Evaluate();
        throw;
    }
    return *this;
}

size_t Timeline::BindTarget(unsigned int nodeID, unsigned int transformID)
{
    const auto key = Key(nodeID, transformID);
    if (const auto it = targetToIndex.find(key); it != targetToIndex.end())
        return it->second;

    if (!owner->CanAttachTimeline(nodeID, transformID, *this))
        throw std::invalid_argument("Timeline: invalid target Transform");

    const size_t index = targets.size();
    targets.push_back({ .nodeID = nodeID, .transformID = transformID });
    try { targetToIndex.emplace(key, index); }
    catch (...) { targets.pop_back(); throw; }

    owner->AttachTimeline(nodeID, transformID, *this);
    return index;
}

void Timeline::RebuildTargets()
{
    for (const auto& target : targets)
        owner->DetachTimeline(target.nodeID, target.transformID, *this);

    targets.clear();
    targetToIndex.clear();
    nodeEffects.clear();
    nodeToIndex.clear();

    for (auto& entry : entries)
        for (auto& clip : entry.clips)
            clip.targetIndex = BindTarget(clip.nodeID, clip.transformID);
}

void Timeline::Clear()
{
    if (owner)
        for (const auto& target : targets)
            owner->DetachTimeline(target.nodeID, target.transformID, *this);

    entries.clear();
    endTimes.clear();
    targets.clear();
    targetToIndex.clear();
    nodeEffects.clear();
    nodeToIndex.clear();
    time = duration = initialT = 0.0f;
    playing = false;
}

void Timeline::SetSpeed(float value)
{
    if (!(value >= 0.0f) || !std::isfinite(value))
        throw std::invalid_argument("Timeline: speed must be finite and >= 0");
    speed = value;
}

void Timeline::SetInitialT(float t)
{
    if (!std::isfinite(t) || t < 0.0f || t > static_cast<float>(entries.size()))
        throw std::invalid_argument("Timeline: initialT out of range");
    initialT = t;
}

void Timeline::RebuildSchedule()
{
    float total = 0.0f;
    endTimes.resize(entries.size());
    for (size_t i = 0; i < entries.size(); ++i)
    {
        auto& entry = entries[i];
        if (!(entry.duration >= 0.0f) || !std::isfinite(entry.duration) ||
            !(entry.speed >= 0.0f) || !std::isfinite(entry.speed) ||
            !(entry.initialT >= 0.0f) || entry.initialT > 1.0f ||
            !std::isfinite(entry.initialT) || !std::isfinite(total + entry.duration))
            throw std::invalid_argument("Timeline: invalid Entry playback configuration");
        entry.time = total;
        total += entry.duration;
        endTimes[i] = total;
    }
    duration = total;
    time = std::clamp(time, 0.0f, duration);
}

void Timeline::SetT(float t)
{
    if (!std::isfinite(t)) throw std::invalid_argument("Timeline: t must be finite");
    RebuildSchedule();
    if (entries.empty()) { time = 0.0f; return; }
    t = std::clamp(t, 0.0f, static_cast<float>(entries.size()));

    if (t >= static_cast<float>(entries.size()))
        time = duration;
    else
    {
        const size_t idx = static_cast<size_t>(t);
        time = entries[idx].time + (t - static_cast<float>(idx)) * entries[idx].duration;
    }
    Evaluate();
}

void Timeline::SetTime(float seconds)
{
    if (!std::isfinite(seconds))
        throw std::invalid_argument("Timeline: invalid time");
    RebuildSchedule();
    time = std::clamp(seconds, 0.0f, duration);
    Evaluate();
}

float Timeline::GetT() const
{
    if (entries.empty() || endTimes.size() != entries.size()) return 0.0f;
    if (time >= duration) return static_cast<float>(entries.size());

    const auto it = std::upper_bound(endTimes.begin(), endTimes.end(), time);
    const size_t idx = static_cast<size_t>(it - endTimes.begin());
    if (idx >= entries.size()) return static_cast<float>(entries.size());

    const float length = entries[idx].duration;
    if (length == 0.0f) return static_cast<float>(idx);
    return static_cast<float>(idx) +
        std::clamp((time - entries[idx].time) / length, 0.0f, 1.0f);
}

void Timeline::Play(size_t index)
{
    if (index >= entries.size()) throw std::out_of_range("Timeline: invalid Entry index");
    SetT(static_cast<float>(index));
    playing = true;
}

void Timeline::Update(float dt)
{
    if (!std::isfinite(dt) || dt < 0.0f)
        throw std::invalid_argument("Timeline: dt must be finite and >= 0");
    RebuildSchedule();
    if (playing && !entries.empty() && dt > 0.0f && speed > 0.0f && duration > 0.0f)
    {
        const double sign = (direction == AnimationPlay::Forward) ? 1.0 : -1.0;
        double next = static_cast<double>(time) + static_cast<double>(dt) * speed * sign;

        if (playing && !entries.empty() && dt > 0.0f
            && speed > 0.0f && duration > 0.0f)
        {
            const double sign =
                direction == AnimationPlay::Forward ? 1.0 : -1.0;

            double next = static_cast<double>(time)
                + static_cast<double>(dt) * speed * sign;

            if (loop)
            {
                next = std::fmod(next, static_cast<double>(duration));

                if (next < 0.0)
                    next += duration;

                time = static_cast<float>(next);
            }
            else
            {
                time = static_cast<float>(
                    std::clamp(next, 0.0, static_cast<double>(duration))
                    );

                if (next <= 0.0 || next >= duration)
                    playing = false;
            }
        }
    }
    Evaluate();
}

const Timeline::Target* Timeline::FindContribution(unsigned int nodeID,
    unsigned int transformID) const noexcept
{
    const auto it = targetToIndex.find(Key(nodeID, transformID));
    return it == targetToIndex.end() ? nullptr : &targets[it->second];
}

const Mat4* Timeline::FindTransform(unsigned int nodeID, unsigned int transformID) const noexcept
{
    const auto* target = FindContribution(nodeID, transformID);
    return target ? &target->result : nullptr;
}

const Mat4* Timeline::FindNodeTransform(unsigned int nodeID) const noexcept
{
    const auto it = nodeToIndex.find(nodeID);
    return it == nodeToIndex.end() ? nullptr : &nodeEffects[it->second].result;
}

void Timeline::Evaluate()
{
    RebuildSchedule();
    const auto& handler = AnimationHandler::Global();

    for (auto& target : targets)
    {
        target.before = target.after = Mat4{ 1.0f };
        target.inheritedBefore = target.inheritedAfter = Mat4{ 1.0f };
        target.result = Mat4{ 1.0f };
        target.passToChild = owner->GetTransformPropagation(target.nodeID, target.transformID);
    }
    nodeEffects.clear();
    nodeToIndex.clear();

    // Re-evaluate from the Timeline clock. Seek, rewind and loop stay deterministic.
    for (const auto& entry : entries)
    {
        if (time < entry.time) break;
        if (!entry.playing) continue;

        const float elapsed = std::clamp(time - entry.time, 0.0f, entry.duration);
        for (const auto& clip : entry.clips)
        {
            if (elapsed < clip.offset) continue;
            float t = entry.initialT + (elapsed - clip.offset) * entry.speed / clip.duration;
            if (entry.loop)
                t -= std::floor(t);
            else
                t = std::clamp(t, 0.0f, 1.0f);

            const Mat4 m = handler.GetTransform(clip.id, t);
            auto& target = targets[clip.targetIndex];
            const bool inherited = clip.propagation == AnimationPropagation::ToChildren ||
                (clip.propagation == AnimationPropagation::Inherit && target.passToChild);

            // Column vectors: later Clips multiply on the left.
            if (clip.placement == AnimationPlacement::Before)
            {
                target.before = m * target.before;
                if (inherited) target.inheritedBefore = m * target.inheritedBefore;
            }
            else
            {
                target.after = m * target.after;
                if (inherited) target.inheritedAfter = m * target.inheritedAfter;
            }

            // Kept for compatibility with the old inspection API.
            const auto [it, added] = nodeToIndex.try_emplace(clip.nodeID, nodeEffects.size());
            if (added) nodeEffects.push_back({ .nodeID = clip.nodeID });
            nodeEffects[it->second].result = m * nodeEffects[it->second].result;
        }
    }

    for (auto& target : targets)
        target.result = target.before * target.after;
}
