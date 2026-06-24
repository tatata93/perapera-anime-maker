// このファイルの役割:
// MyPaintBrushEngineのスタブ実装。
// libmypaintの導入可否をCMakeから受け取りつつ、Step 12では既存描画を壊さないため
// CanvasBitmapの既存bakeStroke/eraseCircleへフォールバックする。

#include "brush/MyPaintBrushEngine.h"

#include "render/CanvasBitmap.h"

namespace perapera {

bool MyPaintBrushEngine::isLibraryAvailable() const
{
#ifdef PERAPERA_HAS_LIBMYPAINT
    return true;
#else
    return false;
#endif
}

const char* MyPaintBrushEngine::backendName() const
{
    return isLibraryAvailable() ? "libmypaint stub" : "simple fallback stub";
}

void MyPaintBrushEngine::bakeStroke(CanvasBitmap& canvas, const Stroke& stroke, float opacity)
{
    // Step 12では、MyPaintBrushEngineを選んでも描画結果は既存SimpleBrushEngine相当にする。
    // ここでlibmypaintのdraw_dabへ接続するのはStep 13の責務。
    canvas.bakeStroke(stroke, opacity);
}

void MyPaintBrushEngine::eraseCircle(CanvasBitmap& canvas, float cx, float cy, float radius)
{
    // 消しゴムはブラシエンジン切り替えの影響を受けないよう、既存のピクセル消去を使う。
    canvas.eraseCircle(cx, cy, radius);
}

} // namespace perapera
