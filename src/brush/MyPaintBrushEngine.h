#pragma once

// このファイルの役割:
// libmypaint接続用ブラシエンジンを定義する。
// Phase 1.5 Step 13では、libmypaintのdraw_dabをCanvasBitmap::paintDabへ接続する。
// libmypaint未検出時は既存Simple互換にフォールバックする。

#include "brush/BrushEngine.h"

namespace perapera {

class MyPaintBrushEngine final : public BrushEngine {
public:
    bool isLibraryAvailable() const;
    const char* backendName() const;

    void bakeStroke(CanvasBitmap& canvas, const Stroke& stroke, float opacity) override;
    void eraseCircle(CanvasBitmap& canvas, float cx, float cy, float radius) override;
};

} // namespace perapera
