// src/ui/DrawingCanvasPanel.h
//
// DrawingCanvasPanelは、ImGui上に簡易作画キャンバスを表示するUIです。
//
// Phase 3Gでは、フレーム管理、レイヤー管理、PNG保存、オニオンスキン、
// 再生プレビュー、PNG連番保存に対応します。

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

        // PNG連番保存の出力フォルダ番号。
        // exports/png_sequence_0001, exports/png_sequence_0002 ... のように使う。
        int nextPngSequenceExportNumber_ = 1;

        bool pngTransparentBackground_ = false;

        std::string lastPngExportMessage_;

        bool lastPngExportSucceeded_ = false;

        std::string lastPngSequenceExportMessage_;

        bool lastPngSequenceExportSucceeded_ = false;

        // オニオンスキン表示設定。
        // 前後フレームを薄く表示して、動きの差分を描きやすくする。
        bool onionSkinEnabled_ = true;
        bool showPreviousOnionSkin_ = true;
        bool showNextOnionSkin_ = true;
        int onionSkinRange_ = 1;
        float onionSkinOpacity_ = 0.25f;
        bool onionSkinVisibleLayersOnly_ = true;

        // 再生プレビュー中かどうか。
        // true の間は、RenderFormat の FPS と各フレームの durationFrames を使って
        // activeFrameIndex_ を自動で進める。
        bool isPlaybackPlaying_ = false;

        // 最後のフレームまで行ったら最初に戻るかどうか。
        bool playbackLoopEnabled_ = true;

        // 現在フレームを何コマ分表示したかを数える。
        // durationFrames に達したら次フレームへ進む。
        int playbackSubFrameCounter_ = 0;

        // ImGuiのDeltaTimeをためる。
        // 1 / FPS 秒たまるごとに1コマ進める。
        float playbackTimeAccumulatorSeconds_ = 0.0f;

        void clampActiveFrameIndex();

        void clampActiveLayerIndex();

        AnimationFrame* activeFrame();

        const AnimationFrame* activeFrame() const;

        DrawingLayer* activeLayer();

        const DrawingLayer* activeLayer() const;

        void updatePlayback(const RenderFormat& renderFormat);

        void stopPlayback();

        void resetPlaybackProgress();

        void addFrame();

        void duplicateActiveFrame();

        void deleteActiveFrame();

        void moveToPreviousFrame();

        void moveToNextFrame();

        void addLayer();

        void deleteActiveLayer();

        void moveActiveLayerUp();

        void moveActiveLayerDown();

        void drawFramePanel(const RenderFormat& renderFormat);

        void drawLayerPanel();

        void clearCurrentFrameLayers();

        void exportCurrentRenderFramePng(
            const WorkCanvas& workCanvas,
            const RenderFormat& renderFormat
        );

        void exportAllFramesPngSequence(
            const WorkCanvas& workCanvas,
            const RenderFormat& renderFormat
        );
    };
}
