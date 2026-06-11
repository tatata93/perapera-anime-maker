// src/ui/DrawingCanvasPanel.h
//
// DrawingCanvasPanelは、ImGui上に簡易作画キャンバスを表示するUIです。
//
// Phase 3Dでは、フレーム管理に対応します。
// 1つのAnimationFrameが複数のDrawingLayerを持ち、
// 現在選択中のフレームに描き込みます。

#pragma once

#include "drawing/AnimationFrame.h"
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

        // アニメーションのフレーム一覧。
        // 各フレームが自分専用のレイヤーを持つ。
        std::vector<AnimationFrame> frames_;

        // 現在選択中のフレーム番号。
        int activeFrameIndex_ = 0;

        // 現在選択中のレイヤー番号。
        int activeLayerIndex_ = 0;

        Stroke currentStroke_;

        bool isDrawing_ = false;

        int nextFrameNumber_ = 2;

        int nextLayerNumber_ = 2;

        int nextPngExportNumber_ = 1;

        bool pngTransparentBackground_ = false;

        std::string lastPngExportMessage_;

        bool lastPngExportSucceeded_ = false;

        void clampActiveFrameIndex();

        void clampActiveLayerIndex();

        AnimationFrame* activeFrame();

        const AnimationFrame* activeFrame() const;

        DrawingLayer* activeLayer();

        const DrawingLayer* activeLayer() const;

        void addFrame();

        void duplicateActiveFrame();

        void deleteActiveFrame();

        void moveToPreviousFrame();

        void moveToNextFrame();

        void addLayer();

        void deleteActiveLayer();

        void moveActiveLayerUp();

        void moveActiveLayerDown();

        void drawFramePanel();

        void drawLayerPanel();

        void clearCurrentFrameLayers();

        void exportCurrentRenderFramePng(
            const WorkCanvas& workCanvas,
            const RenderFormat& renderFormat
        );
    };
}