#pragma once

// このファイルの役割:
// libmypaint接続用ブラシエンジンを定義する。
// Phase 1.5 Step 14cでは、確定時一括処理ではなくドラッグ中逐次処理でdabをCanvasBitmapへ流す。
// libmypaint未検出時や保存データ再構築時は既存Simple互換にフォールバックする。

#include <memory>

#include "brush/BrushEngine.h"

namespace perapera {

class MyPaintBrushEngine final : public BrushEngine {
public:
    MyPaintBrushEngine() = default;
    ~MyPaintBrushEngine() override;

    MyPaintBrushEngine(const MyPaintBrushEngine&) = delete;
    MyPaintBrushEngine& operator=(const MyPaintBrushEngine&) = delete;

    bool isLibraryAvailable() const;
    const char* backendName() const;

    // ドラッグ開始時に1回呼ぶ。ブラシ設定・サーフェスを初期化する。
    void beginStroke(CanvasBitmap& canvas, const Stroke& strokeSettings, float opacity);

    // 新しい点が来るたびに呼ぶ。mypaint_brush_stroke_to へ1点だけ投入する。
    // 戻り値: dab生成によりCanvasBitmapがdirtyになった可能性があるならtrue。
    bool addPoint(const StrokePoint& point, float deltaTimeSec);

    // ドラッグ終了時に呼ぶ。libmypaintのブラシ状態を解放する。
    void endStroke();

    // 確定時の一括処理。逐次モードが使えない場合・保存データ再構築時のSimple互換フォールバックとして残す。
    void bakeStroke(CanvasBitmap& canvas, const Stroke& stroke, float opacity) override;
    void eraseCircle(CanvasBitmap& canvas, float cx, float cy, float radius) override;

private:
#ifdef PERAPERA_HAS_LIBMYPAINT
    struct MyPaintState;
    struct MyPaintStateDeleter {
        void operator()(MyPaintState* state) const noexcept;
    };
    std::unique_ptr<MyPaintState, MyPaintStateDeleter> state_;
#endif
};

} // namespace perapera
