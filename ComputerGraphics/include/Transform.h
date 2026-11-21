#pragma once
#include <Alias.h>

class Timeline;

struct Transform
{
    Transform() = default;
    Transform(unsigned int id, const Mat4& m, bool passToChild)
        : m{ m }, id{ id }, passToChild{ passToChild } {}

    Mat4 m{ 1.0f };
    unsigned int id{};
    bool passToChild{};
    const Timeline* timeline{}; // Non-owning; Object owns the Timeline.
};
