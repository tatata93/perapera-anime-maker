#pragma once

// このファイルの役割:
// Phase 1用の最小ブラシエンジンを定義する。
// 入り抜き・水彩などはまだ行わず、円スタンプ方式で線を焼く。

#include "brush/BrushEngine.h"

namespace perapera {

class SimpleBrushEngine final : public BrushEngine {
public:
    void bakeStroke(CanvasBitmap& canvas, const Stroke& stroke, float opacity) override;
    void eraseCircle(CanvasBitmap& canvas, float cx, float cy, float radius) override;
};

} // namespace perapera
