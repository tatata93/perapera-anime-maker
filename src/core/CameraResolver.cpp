#include "core/CameraResolver.h"

#include <algorithm>

namespace perapera {
namespace {

constexpr float kMinimumCameraZoom = 0.001f;

float safeZoom(float zoom)
{
    return std::max(kMinimumCameraZoom, zoom);
}

float lerp(float a, float b, float t)
{
    return a + (b - a) * t;
}

ResolvedCamera2D fromBaseCamera(const CameraSettings& camera)
{
    ResolvedCamera2D resolved;
    resolved.centerX = camera.centerX;
    resolved.centerY = camera.centerY;
    resolved.zoom = safeZoom(camera.zoom);
    return resolved;
}

ResolvedCamera2D fromCameraKey(const CameraKey& key)
{
    ResolvedCamera2D resolved;
    resolved.centerX = key.centerX;
    resolved.centerY = key.centerY;
    resolved.zoom = safeZoom(key.zoom);
    return resolved;
}

} // namespace

ResolvedCamera2D resolveCameraAtFrame(const CameraSettings& camera, int timelineFrame)
{
    const ResolvedCamera2D base = fromBaseCamera(camera);
    if (!camera.animationEnabled || camera.keys.empty()) {
        return base;
    }

    const CameraKey* exact = nullptr;
    const CameraKey* previous = nullptr;
    const CameraKey* next = nullptr;

    for (const CameraKey& key : camera.keys) {
        if (key.frame == timelineFrame) {
            exact = &key;
            continue;
        }

        if (key.frame < timelineFrame) {
            if (previous == nullptr || key.frame >= previous->frame) {
                previous = &key;
            }
            continue;
        }

        if (next == nullptr || key.frame <= next->frame) {
            next = &key;
        }
    }

    if (exact != nullptr) {
        return fromCameraKey(*exact);
    }
    if (previous == nullptr && next == nullptr) {
        return base;
    }
    if (previous == nullptr) {
        return fromCameraKey(*next);
    }
    if (next == nullptr) {
        return fromCameraKey(*previous);
    }

    const int frameSpan = std::max(1, next->frame - previous->frame);
    const float t = std::clamp(
        static_cast<float>(timelineFrame - previous->frame) / static_cast<float>(frameSpan),
        0.0f,
        1.0f);

    ResolvedCamera2D resolved;
    resolved.centerX = lerp(previous->centerX, next->centerX, t);
    resolved.centerY = lerp(previous->centerY, next->centerY, t);
    resolved.zoom = safeZoom(lerp(previous->zoom, next->zoom, t));
    return resolved;
}

} // namespace perapera
