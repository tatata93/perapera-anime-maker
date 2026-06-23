#pragma once

// src/camera/CameraAnimation.h
//
// 2Dカメラのキーフレームアニメーションを管理する。
// Phase 1では線形/ステップ/スムーズ補間の土台だけを実装する。

#include <vector>

#include "camera/ShotCamera2D.h"

namespace perapera {

enum class CameraInterpolation {
    Step,
    Linear,
    Smooth
};

struct CameraKeyframe2D {
    int frame = 0;
    float centerX = 1280.0f;
    float centerY = 720.0f;
    float zoom = 1.0f;
    float rotationDeg = 0.0f;
    CameraInterpolation interpolation = CameraInterpolation::Linear;
};

class CameraAnimation {
public:
    void clear();
    void addOrReplaceKey(const CameraKeyframe2D& keyframe);
    bool empty() const;
    const std::vector<CameraKeyframe2D>& keys() const { return keys_; }

    ShotCamera2D evaluate(int frame, const ShotCamera2D& fallback) const;

private:
    std::vector<CameraKeyframe2D> keys_;

    static CameraKeyframe2D cameraToKey(int frame, const ShotCamera2D& camera);
    static ShotCamera2D keyToCamera(const CameraKeyframe2D& key, const ShotCamera2D& fallback);
    static float interpolate(float a, float b, float t, CameraInterpolation interpolation);
};

} // namespace perapera
