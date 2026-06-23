// src/brush/SimpleBrushEngine.cpp
//
// SimpleBrushEngineは、CanvasBitmapが持つ円スタンプ処理を使う薄い実装。
// 後でlibmypaintへ差し替える時も、App側はBrushEngineだけを見ればよい。

#include "brush/SimpleBrushEngine.h"

#include "core/Stroke.h"
#include "render/CanvasBitmap.h"

namespace perapera {

const char* SimpleBrushEngine::name() const {
    return "Simple Circle Brush";
}

void SimpleBrushEngine::bakeStroke(CanvasBitmap& bitmap, const Stroke& stroke, float opacity) {
    bitmap.bakeStroke(stroke, opacity);
}

void SimpleBrushEngine::eraseCircle(CanvasBitmap& bitmap, float cx, float cy, float radius) {
    bitmap.eraseCircle(cx, cy, radius);
}

} // namespace perapera
