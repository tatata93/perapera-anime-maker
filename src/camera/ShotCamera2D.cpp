// src/camera/ShotCamera2D.cpp
//
// 2D撮影カメラの座標変換を実装する。
// 回転はカメラ中心を基準に行い、その後ズームと出力中心への移動を適用する。

#include "camera/ShotCamera2D.h"

#include <algorithm>
#include <cmath>

namespace perapera {
namespace {

constexpr float kPi = 3.14159265358979323846f;
constexpr float kMinimumZoom = 0.001f;

float degreesToRadians(float degrees) {
    return degrees * kPi / 180.0f;
}

float safeZoom(float zoom) {
    return std::max(kMinimumZoom, zoom);
}

} // namespace

void ShotCamera2D::setOutputSize(int width, int height) {
    outputWidth_ = std::max(1, width);
    outputHeight_ = std::max(1, height);
}

ImVec2 ShotCamera2D::canvasToOutput(float canvasX, float canvasY) const {
    const float z = safeZoom(zoom);
    const float radians = degreesToRadians(rotationDeg);
    const float c = std::cos(radians);
    const float s = std::sin(radians);

    const float localX = canvasX - centerX;
    const float localY = canvasY - centerY;

    const float rotatedX = localX * c - localY * s;
    const float rotatedY = localX * s + localY * c;

    return ImVec2(static_cast<float>(outputWidth_) * 0.5f + rotatedX * z,
                  static_cast<float>(outputHeight_) * 0.5f + rotatedY * z);
}

ImVec2 ShotCamera2D::outputToCanvas(float outputX, float outputY) const {
    const float z = safeZoom(zoom);
    const float radians = degreesToRadians(rotationDeg);
    const float c = std::cos(radians);
    const float s = std::sin(radians);

    const float localX = (outputX - static_cast<float>(outputWidth_) * 0.5f) / z;
    const float localY = (outputY - static_cast<float>(outputHeight_) * 0.5f) / z;

    // 逆回転。回転行列の逆は、角度をマイナスにしたものになる。
    const float unrotatedX = localX * c + localY * s;
    const float unrotatedY = -localX * s + localY * c;

    return ImVec2(centerX + unrotatedX, centerY + unrotatedY);
}

} // namespace perapera
