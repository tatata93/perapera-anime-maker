// src/ui/DrawingCanvasPanel.h
//
// DrawingCanvasPanelは、ImGui上に簡易作画キャンバスを表示するUIです。
//
// Phase 3Lでは、フレーム管理、レイヤー管理、PNG保存、オニオンスキン、
// 再生プレビュー、PNG連番保存、プロジェクト保存/読み込み、
// Undo/Redo、消しゴム、タイムラインUI、名前変更に対応します。

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

        void draw(WorkCanvas& workCanvas, RenderFormat& renderFormat);

    private:
        enum class DrawingTool
        {
            Pen,
            Eraser
        };

        struct EditorHistorySnapshot
        {
            std::vector<AnimationFrame> frames;
            int activeFrameIndex = 0;
            int activeLayerIndex = 0;
            int nextFrameNumber = 2;
            int nextLayerNumber = 2;
        };

        static constexpr int MaxUndoHistoryCount = 50;

        Brush brush_;

        // 現在選択中の作画ツール。
        // Phase 3Jではペンと消しゴムを切り替えられるようにする。
        DrawingTool drawingTool_ = DrawingTool::Pen;

        // 消しゴムの半径。
        // 画面上ではこの半径の円をガイドとして表示する。
        float eraserRadiusPx_ = 24.0f;

        // 消しゴムドラッグ中かどうか。
        bool isErasing_ = false;

        // 前回の消しゴム位置。
        // ドラッグ中に点だけでなく線分として消すために使う。
        CanvasPoint previousEraserPoint_;
        bool hasPreviousEraserPoint_ = false;

        // アニメーションのフレーム一覧.
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

        // Phase 3Hでは、最初の保存/読み込みとして固定パスを使う。
        // ファイルダイアログは後のPhaseで追加する。
        std::string projectFilePath_ = "projects/current_project.perapera_project.txt";

        std::string lastProjectFileMessage_;

        bool lastProjectFileSucceeded_ = false;

        // Undo/Redo用の履歴。
        // Phase 3Iでは、フレーム・レイヤー・ストロークの状態を丸ごと保存する
        // スナップショット方式で実装する。
        std::vector<EditorHistorySnapshot> undoStack_;

        std::vector<EditorHistorySnapshot> redoStack_;

        std::string lastUndoRedoMessage_;

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

        bool eraseActiveLayerAt(CanvasPoint eraserCanvasPoint);

        bool eraseActiveLayerAlongPath(
            CanvasPoint previousCanvasPoint,
            CanvasPoint currentCanvasPoint
        );

        EditorHistorySnapshot makeHistorySnapshot() const;

        void restoreHistorySnapshot(const EditorHistorySnapshot& snapshot);

        void pushUndoSnapshot(const std::string& actionName);

        void clearHistory();

        void undoLastAction();

        void redoLastAction();

        void drawUndoRedoControls();

        void updateUndoRedoShortcuts();

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

        void drawTimelinePanel(const RenderFormat& renderFormat);

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

        void saveProjectFile(
            const WorkCanvas& workCanvas,
            const RenderFormat& renderFormat
        );

        void loadProjectFile(
            WorkCanvas& workCanvas,
            RenderFormat& renderFormat
        );
    };
}
