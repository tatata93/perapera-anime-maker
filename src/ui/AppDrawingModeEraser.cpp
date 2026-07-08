#include "ui/AppDrawingModeEraser.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iterator>
#include <limits>
#include <utility>

namespace perapera::app_drawing {
namespace {

float distanceSquared(const StrokePoint& a, const StrokePoint& b)
{
    const float dx = a.x - b.x;
    const float dy = a.y - b.y;
    return dx * dx + dy * dy;
}

float distancePointToSegmentSquared(const StrokePoint& point,
                                    const StrokePoint& segmentA,
                                    const StrokePoint& segmentB)
{
    const float vx = segmentB.x - segmentA.x;
    const float vy = segmentB.y - segmentA.y;
    const float wx = point.x - segmentA.x;
    const float wy = point.y - segmentA.y;
    const float lengthSq = vx * vx + vy * vy;
    if (lengthSq <= 0.0001f) {
        return distanceSquared(point, segmentA);
    }
    const float t = std::clamp((wx * vx + wy * vy) / lengthSq, 0.0f, 1.0f);
    const StrokePoint closest{segmentA.x + vx * t, segmentA.y + vy * t, 1.0f};
    return distanceSquared(point, closest);
}

float distancePointToStrokePathSquared(const StrokePoint& point, const Stroke& stroke)
{
    if (stroke.points.empty()) {
        return std::numeric_limits<float>::infinity();
    }
    if (stroke.points.size() == 1U) {
        return distanceSquared(point, stroke.points.front());
    }
    float best = std::numeric_limits<float>::infinity();
    for (std::size_t i = 1; i < stroke.points.size(); ++i) {
        best = std::min(best,
                        distancePointToSegmentSquared(point,
                                                       stroke.points[i - 1U],
                                                       stroke.points[i]));
    }
    return best;
}

float strokeApproxLength(const Stroke& stroke)
{
    if (stroke.points.size() < 2U) {
        return 0.0f;
    }
    float length = 0.0f;
    for (std::size_t i = 1; i < stroke.points.size(); ++i) {
        const float dx = stroke.points[i].x - stroke.points[i - 1U].x;
        const float dy = stroke.points[i].y - stroke.points[i - 1U].y;
        length += std::sqrt(dx * dx + dy * dy);
    }
    return length;
}

std::vector<StrokePoint> resampleStrokeForEraser(const Stroke& stroke, float spacing)
{
    std::vector<StrokePoint> result;
    if (stroke.points.empty()) {
        return result;
    }
    if (stroke.points.size() == 1U) {
        result.push_back(stroke.points.front());
        return result;
    }
    result.push_back(stroke.points.front());
    const float safeSpacing = std::max(0.5f, spacing);
    for (std::size_t index = 1; index < stroke.points.size(); ++index) {
        const StrokePoint& previous = stroke.points[index - 1U];
        const StrokePoint& current = stroke.points[index];
        const float dx = current.x - previous.x;
        const float dy = current.y - previous.y;
        const float length = std::sqrt(dx * dx + dy * dy);
        const int steps = std::max(1, static_cast<int>(std::ceil(length / safeSpacing)));
        for (int step = 1; step <= steps; ++step) {
            const float t = static_cast<float>(step) / static_cast<float>(steps);
            result.push_back(StrokePoint{previous.x + dx * t,
                                         previous.y + dy * t,
                                         previous.pressure + (current.pressure - previous.pressure) * t});
        }
    }
    return result;
}

float eraserHitRadius(float strokeRadius, float eraserRadius)
{
    return std::max(2.0f, eraserRadius + strokeRadius * 0.35f);
}

void keepLocalEraserGap(std::vector<bool>& hitFlags,
                        std::size_t closestIndex,
                        float spacing,
                        float eraserRadius,
                        float strokeRadius)
{
    if (hitFlags.empty()) {
        return;
    }
    std::fill(hitFlags.begin(), hitFlags.end(), false);
    const float localGapRadius = std::max(2.0f, eraserRadius * 1.15f + strokeRadius * 0.25f);
    const int gapSamples = std::max(1, static_cast<int>(std::ceil(localGapRadius / std::max(0.5f, spacing))));
    const int begin = std::max(0, static_cast<int>(closestIndex) - gapSamples);
    const int end = std::min(static_cast<int>(hitFlags.size()) - 1,
                             static_cast<int>(closestIndex) + gapSamples);
    for (int i = begin; i <= end; ++i) {
        hitFlags[static_cast<std::size_t>(i)] = true;
    }
}

void appendStrokePart(std::vector<Stroke>& output, const Stroke& source, std::vector<StrokePoint>& points)
{
    if (points.empty()) {
        return;
    }
    Stroke part = source;
    part.points = points;
    output.push_back(std::move(part));
    points.clear();
}

} // namespace

std::vector<Stroke> splitStrokeByEraser(const Stroke& stroke,
                                        const Stroke& eraserStroke,
                                        float eraserRadius,
                                        bool& changed)
{
    std::vector<Stroke> result;
    if (stroke.points.empty() || eraserStroke.points.empty()) {
        result.push_back(stroke);
        return result;
    }
    const float spacing = std::max(1.0f, std::min(stroke.radiusPx, eraserRadius) * 0.5f);
    const std::vector<StrokePoint> sampledPoints = resampleStrokeForEraser(stroke, spacing);
    if (sampledPoints.empty()) {
        result.push_back(stroke);
        return result;
    }
    const float hitRadius = eraserHitRadius(stroke.radiusPx, eraserRadius);
    const float hitRadiusSq = hitRadius * hitRadius;
    std::vector<bool> hitFlags(sampledPoints.size(), false);
    int hitCount = 0;
    float bestDistanceSq = std::numeric_limits<float>::infinity();
    std::size_t closestIndex = 0U;
    for (std::size_t i = 0; i < sampledPoints.size(); ++i) {
        const float distanceSq = distancePointToStrokePathSquared(sampledPoints[i], eraserStroke);
        if (distanceSq < bestDistanceSq) {
            bestDistanceSq = distanceSq;
            closestIndex = i;
        }
        if (distanceSq <= hitRadiusSq) {
            hitFlags[i] = true;
            ++hitCount;
        }
    }
    if (hitCount == 0) {
        result.push_back(stroke);
        return result;
    }
    const float targetLength = strokeApproxLength(stroke);
    const float eraserPathLength = strokeApproxLength(eraserStroke);
    const bool shortEraserGesture = eraserPathLength <= std::max(4.0f, eraserRadius * 1.5f);
    const bool mostlyHit = hitCount >= static_cast<int>(sampledPoints.size() * 0.90f);
    if (shortEraserGesture && sampledPoints.size() >= 3U &&
        targetLength > std::max(8.0f, eraserRadius * 2.5f) && mostlyHit) {
        keepLocalEraserGap(hitFlags, closestIndex, spacing, eraserRadius, stroke.radiusPx);
    }
    std::vector<StrokePoint> currentPart;
    bool anyErasedPoint = false;
    for (std::size_t i = 0; i < sampledPoints.size(); ++i) {
        if (hitFlags[i]) {
            anyErasedPoint = true;
            appendStrokePart(result, stroke, currentPart);
        } else {
            currentPart.push_back(sampledPoints[i]);
        }
    }
    appendStrokePart(result, stroke, currentPart);
    if (anyErasedPoint && result.empty() && shortEraserGesture) {
        keepLocalEraserGap(hitFlags, closestIndex, spacing, eraserRadius, stroke.radiusPx);
        currentPart.clear();
        for (std::size_t i = 0; i < sampledPoints.size(); ++i) {
            if (hitFlags[i]) {
                appendStrokePart(result, stroke, currentPart);
            } else {
                currentPart.push_back(sampledPoints[i]);
            }
        }
        appendStrokePart(result, stroke, currentPart);
    }
    if (!anyErasedPoint || result.empty()) {
        result.clear();
        result.push_back(stroke);
        return result;
    }
    changed = true;
    return result;
}

} // namespace perapera::app_drawing
