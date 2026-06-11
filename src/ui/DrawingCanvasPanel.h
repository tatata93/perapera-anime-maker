// src/ui/DrawingCanvasPanel.h
//
// DrawingCanvasPanelは、ImGui上に簡易作画キャンバスを表示するUIです。
//
// Phase 3Cでは、複数レイヤーに加えてPNG保存に対応します。
// まだ本物の画像バッファには常時描きません。
// 保存時にStroke配列を一時的にPNG画像へ変換します。

#pragma once

#include "drawing/Brush.h"
#include "drawing/DrawingLayer.h"
#include "drawing/Stroke.h"
#include "export/PngExporter.h"

#include <string>
#include <vector>

namespace perapera
{
    class WorkCanvas;
    class RenderFormat;

    class DrawingCanvasPanel
    {
    public:
        DrawingCanvasPanel();

        void draw(WorkCanvas& workCanvas, const RenderFormat& renderFormat);

    private:
        Brush brush_;

        std::vector<DrawingLayer> layers_;

        int activeLayerIndex_ = 0;

        Stroke currentStroke_;

        bool isDrawing_ = false;

        int nextLayerNumber_ = 2;

        // PNG保存時の連番。
        int nextPngExportNumber_ = 1;

        // trueなら透明背景PNG、falseなら確認しやすい濃い背景つきPNG。
        bool pngTransparentBackground_ = false;

        // 最後のPNG保存結果をUIに表示する。
        std::string lastPngExportMessage_;

        bool lastPngExportSucceeded_ = false;

        void clampActiveLayerIndex();

        DrawingLayer* activeLayer();

        const DrawingLayer* activeLayer() const;

        void addLayer();

        void deleteActiveLayer();

        void moveActiveLayerUp();

        void moveActiveLayerDown();

        void drawLayerPanel();

        void clearAllLayers();

        // 現在の撮影フレーム範囲をPNGとして保存する。
        void exportCurrentRenderFramePng(
            const WorkCanvas& workCanvas,
            const RenderFormat& renderFormat
        );
    };
}