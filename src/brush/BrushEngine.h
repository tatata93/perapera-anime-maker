#pragma once

// このファイルの役割:
// ブラシエンジンの共通インターフェースを定義する。
// Phase 1ではSimpleBrushEngineを使い、Phase 1.5でlibmypaint実装へ差し替えられるようにする。

#include "core/Stroke.h"

namespace perapera {

class CanvasBitmap;

class BrushEngine {
public:
    virtual ~BrushEngine() = default;

    // ストロークをキャンバスへ焼き込む。
    // 描画中のプレビューではなく、ペンを離して確定した時だけ呼ぶ想定。
    virtual void bakeStroke(CanvasBitmap& canvas, const Stroke& stroke, float opacity) = 0;

    // 円形消しゴムを適用する。
    virtual void eraseCircle(CanvasBitmap& canvas, float cx, float cy, float radius) = 0;
};

} // namespace perapera
