#pragma once

// src/camera/ShotCamera2D.h
//
// 撮影台の2Dカメラ設定を保持し、キャンバス座標と出力画面座標を変換する。
// Phase 1では描画補助の土台として用意し、UI接続は後続Stepで行う。

#include <imgui.h>

namespace perapera {

class ShotCamera2D {
public:
    float centerX = 1280.0f;
    float centerY = 720.0f;
    float zoom = 1.0f;
    float rotationDeg = 0.0f;

    void setOutputSize(int width, int height);
    int outputWidth() const { return outputWidth_; }
    int outputHeight() const { return outputHeight_; }

    ImVec2 canvasToOutput(float canvasX, float canvasY) const;
    ImVec2 outputToCanvas(float outputX, float outputY) const;

private:
    int outputWidth_ = 1920;
    int outputHeight_ = 1080;
};

} // namespace perapera
