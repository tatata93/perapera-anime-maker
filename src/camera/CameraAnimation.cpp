// src/camera/CameraAnimation.cpp
//
// カメラキーフレームを時刻順に保持し、指定フレームのカメラ値を返す。
// Step補間は前キーの値を維持し、Linear/Smoothは次キーへ補間する。

#include "camera/CameraAnimation.h"

#include <algorithm>

namespace perapera {
namespace {

float clamp01(float value) {
    return std::clamp(value, 0.0f, 1.0f);
}

float smoothstep(float value) {
    const float t = clamp01(value);
    return t * t * (3.0f - 2.0f * t);
}

} // namespace

void CameraAnimation::clear() {
    keys_.clear();
}

void CameraAnimation::addOrReplaceKey(const CameraKeyframe2D& keyframe) {
    auto it = std::lower_bound(keys_.begin(), keys_.end(), keyframe.frame,
        [](const CameraKeyframe2D& key, int frame) {
            return key.frame < frame;
        });

    if (it != keys_.end() && it->frame == keyframe.frame) {
        *it = keyframe;
        return;
    }

    keys_.insert(it, keyframe);
}

bool CameraAnimation::empty() const {
    return keys_.empty();
}

ShotCamera2D CameraAnimation::evaluate(int frame, const ShotCamera2D& fallback) const {
    if (keys_.empty()) {
        return fallback;
    }

    if (frame <= keys_.front().frame) {
        return keyToCamera(keys_.front(), fallback);
    }

    if (frame >= keys_.back().frame) {
        return keyToCamera(keys_.back(), fallback);
    }

    auto nextIt = std::upper_bound(keys_.begin(), keys_.end(), frame,
        [](int targetFrame, const CameraKeyframe2D& key) {
            return targetFrame < key.frame;
        });

    const CameraKeyframe2D& next = *nextIt;
    const CameraKeyframe2D& previous = *(nextIt - 1);

    const int frameSpan = std::max(1, next.frame - previous.frame);
    const float rawT = static_cast<float>(frame - previous.frame) / static_cast<float>(frameSpan);

    CameraKeyframe2D mixed = cameraToKey(frame, fallback);
    mixed.centerX = interpolate(previous.centerX, next.centerX, rawT, previous.interpolation);
    mixed.centerY = interpolate(previous.centerY, next.centerY, rawT, previous.interpolation);
    mixed.zoom = interpolate(previous.zoom, next.zoom, rawT, previous.interpolation);
    mixed.rotationDeg = interpolate(previous.rotationDeg, next.rotationDeg, rawT, previous.interpolation);

    return keyToCamera(mixed, fallback);
}

CameraKeyframe2D CameraAnimation::cameraToKey(int frame, const ShotCamera2D& camera) {
    CameraKeyframe2D key;
    key.frame = frame;
    key.centerX = camera.centerX;
    key.centerY = camera.centerY;
    key.zoom = camera.zoom;
    key.rotationDeg = camera.rotationDeg;
    return key;
}

ShotCamera2D CameraAnimation::keyToCamera(const CameraKeyframe2D& key, const ShotCamera2D& fallback) {
    ShotCamera2D camera = fallback;
    camera.centerX = key.centerX;
    camera.centerY = key.centerY;
    camera.zoom = key.zoom;
    camera.rotationDeg = key.rotationDeg;
    return camera;
}

float CameraAnimation::interpolate(float a, float b, float t, CameraInterpolation interpolation) {
    if (interpolation == CameraInterpolation::Step) {
        return a;
    }

    const float adjustedT = interpolation == CameraInterpolation::Smooth ? smoothstep(t) : clamp01(t);
    return a + (b - a) * adjustedT;
}

} // namespace perapera
