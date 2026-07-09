#include "core/CellMotionResolver.h"

#include <algorithm>
#include <vector>

namespace perapera {
namespace {

float interpolateFloat(float a, float b, float t) noexcept
{
    return a + (b - a) * std::clamp(t, 0.0f, 1.0f);
}

CellPlacement interpolatePlacement(const CellPlacement& a, const CellPlacement& b, float t) noexcept
{
    CellPlacement result;
    result.x = interpolateFloat(a.x, b.x, t);
    result.y = interpolateFloat(a.y, b.y, t);
    result.scale = interpolateFloat(a.scale, b.scale, t);
    result.rotation = interpolateFloat(a.rotation, b.rotation, t);
    return result;
}

std::vector<CellMotionKey> sortedMotionKeys(const Cell& cell)
{
    std::vector<CellMotionKey> keys;
    keys.reserve(cell.motionKeys.size());
    for (const CellMotionKey& key : cell.motionKeys) {
        if (key.frame > 0) {
            keys.push_back(key);
        }
    }
    std::stable_sort(keys.begin(), keys.end(), [](const CellMotionKey& lhs, const CellMotionKey& rhs) {
        return lhs.frame < rhs.frame;
    });
    return keys;
}

} // namespace

CellPlacement resolveCellPlacementAtFrame(const Cell& cell, int timelineFrame)
{
    if (timelineFrame <= 0 || cell.motionKeys.empty()) {
        return cell.placement;
    }

    const std::vector<CellMotionKey> keys = sortedMotionKeys(cell);
    const CellMotionKey* previous = nullptr;
    const CellMotionKey* next = nullptr;
    for (const CellMotionKey& key : keys) {
        if (key.frame <= timelineFrame) {
            previous = &key;
            continue;
        }
        next = &key;
        break;
    }

    if (previous == nullptr) {
        return cell.placement;
    }
    if (next == nullptr || previous->frame == next->frame || previous->interpolation == "hold") {
        return previous->placement;
    }

    const float t = static_cast<float>(timelineFrame - previous->frame) /
        static_cast<float>(next->frame - previous->frame);
    return interpolatePlacement(previous->placement, next->placement, t);
}

} // namespace perapera
