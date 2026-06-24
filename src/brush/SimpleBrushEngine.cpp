// このファイルの役割:
// SimpleBrushEngineの実装。
// 実際のピクセル操作はCanvasBitmapに集約し、ブラシエンジン側は呼び出し口だけを持つ。

#include "brush/SimpleBrushEngine.h"

#include "render/CanvasBitmap.h"

namespace perapera {

void SimpleBrushEngine::bakeStroke(CanvasBitmap& canvas, const Stroke& stroke, float opacity)
{
    canvas.bakeStroke(stroke, opacity);
}

void SimpleBrushEngine::eraseCircle(CanvasBitmap& canvas, float cx, float cy, float radius)
{
    canvas.eraseCircle(cx, cy, radius);
}

} // namespace perapera
