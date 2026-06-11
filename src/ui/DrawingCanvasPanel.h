
// src/ui/DrawingCanvasPanel.h
//
// DrawingCanvasPanelは、ImGui上に簡易作画キャンバスを表示するUIです。
//
// Phase 3Aでは、本物の画像バッファにはまだ描きません。
// まずはStrokeの配列として線を保存し、ImGuiのDrawListで見えるようにします。

#pragma once

#include "drawing/Brush.h"
#include "drawing/Stroke.h"

#include <vector>

namespace perapera
{
    class WorkCanvas;
    class RenderFormat;

    class DrawingCanvasPanel
    {
    public:
        // ImGuiウィンドウとして簡易作画キャンバスを描く。
        void draw(WorkCanvas& workCanvas, const RenderFormat& renderFormat);

    private:
        // 現在のペン設定。
        Brush brush_;

        // すでに描き終わった線。
        std::vector<Stroke> strokes_;

        // 今まさに描いている途中の線。
        Stroke currentStroke_;

        // マウス左ボタンを押して描画中かどうか。
        bool isDrawing_ = false;

        // すべての線を消す。
        void clearAllStrokes();
    };
}