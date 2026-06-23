#pragma once

// src/brush/BrushEngine.h
//
// ブラシエンジンの抽象インターフェース。
// Phase 1ではSimpleBrushEngineだけを使い、Phase 1.5でlibmypaint実装を
// 同じインターフェースに差し替えられるようにする。

namespace perapera {

class CanvasBitmap;
class Stroke;

class BrushEngine {
public:
    virtual ~BrushEngine() = default;

    virtual const char* name() const = 0;

    // ストローク確定時に呼ぶ。
    // 毎フレームのプレビュー描画はCanvasRenderer側で軽く行う。
    virtual void bakeStroke(CanvasBitmap& bitmap, const Stroke& stroke, float opacity) = 0;

    virtual void eraseCircle(CanvasBitmap& bitmap, float cx, float cy, float radius) = 0;
};

} // namespace perapera
