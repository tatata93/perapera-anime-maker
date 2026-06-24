#pragma once

// このファイルの役割:
// libmypaint接続用ブラシエンジンのスタブを定義する。
// Step 12ではビルド経路とUI切り替えだけを作り、実際のdraw_dab接続はStep 13で行う。

#include "brush/BrushEngine.h"

namespace perapera {

class MyPaintBrushEngine final : public BrushEngine {
public:
    // libmypaintがCMakeで検出されているかをUIやデバッグから確認するための口。
    // Step 12では検出状態の表示だけに使い、描画結果は安全なSimple互換にする。
    bool isLibraryAvailable() const;
    const char* backendName() const;

    void bakeStroke(CanvasBitmap& canvas, const Stroke& stroke, float opacity) override;
    void eraseCircle(CanvasBitmap& canvas, float cx, float cy, float radius) override;
};

} // namespace perapera
