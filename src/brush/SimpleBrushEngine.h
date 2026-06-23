#pragma once

// src/brush/SimpleBrushEngine.h
//
// Phase 1用の最小ブラシエンジン。
// 点列を円スタンプでCanvasBitmapへ焼く。

#include "brush/BrushEngine.h"

namespace perapera {

class SimpleBrushEngine final : public BrushEngine {
public:
    const char* name() const override;
    void bakeStroke(CanvasBitmap& bitmap, const Stroke& stroke, float opacity) override;
    void eraseCircle(CanvasBitmap& bitmap, float cx, float cy, float radius) override;
};

} // namespace perapera
