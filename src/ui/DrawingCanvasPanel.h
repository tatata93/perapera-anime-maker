// src/ui/DrawingCanvasPanel.h
//
// DrawingCanvasPanelは、ImGui上に簡易作画キャンバスを表示するUIです。
//
// Phase 3Oでは、フレーム管理、レイヤー管理、PNG保存、オニオンスキン、
// 再生プレビュー、PNG連番保存、プロジェクト保存/読み込み、
// Undo/Redo、消しゴム、タイムラインUI、名前変更、
// ブラシ補間・手ぶれ補正・簡易入り抜き、
// 撮影用2Dカメラのパン・ズームとカメラキーに対応します。

#pragma once

#include "camera/ShotCamera2D.h"
#include "camera/ShotCameraAnimation.h"
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
            ShotCameraAnimation shotCameraAnimation;
            ShotCamera2D shotCamera;
            bool shotCameraAnimationEnabled = true;
            int activeFrameIndex = 0;
            int activeLayerIndex = 0;
            int nextFrameNumber = 2;
            int nextLayerNumber = 2;
        };

        static constexpr int MaxUndoHistoryCount = 50;

        Brush brush_;

        // 最終出力に使う撮影用2Dカメラ。
        // 将来追加する3D補助カメラとは別概念として管理する。
        ShotCamera2D shotCamera2D_;

        ShotCameraAnimation shotCameraAnimation_;

        bool shotCameraAnimationEnabled_ = true;

        ShotCameraInterpolation newShotCameraKeyInterpolation_ =
            ShotCameraInterpolation::Smooth;

        int lastAppliedShotCameraTimelineFrame_ = -1;

        // 撮影カメラの上下左右移動ボタンで使う移動量。
        float shotCameraNudgePx_ = 100.0f;

        // 現在選択中の作画ツール。
        // Phase 3Jではペンと消しゴムを切り替えられるようにする。
        DrawingTool drawingTool_ = DrawingTool::Pen;

        // 消しゴムの半径。
        // 画面上ではこの半径の円をガイドとして表示する。
        float eraserRadiusPx_ = 24.0f;

        // 消しゴムドラッグ中かどうか。
        bool isErasing_ = false;

        // 線の補間を有効にするか。
        // 点と点の間が空きすぎたときに中間点を追加し、線の途切れ感を減らす。
        bool strokeInterpolationEnabled_ = true;

        // 補間した点同士の目安間隔。
        // 小さいほど点が増えてなめらかになるが、保存データも少し増える。
        float strokePointSpacingPx_ = 4.0f;

        // 手ぶれ補正を有効にするか。
        // マウス入力を少し遅れて追従させ、細かなブレを抑える。
        bool strokeSmoothingEnabled_ = true;

        // 手ぶれ補正の強さ。
        // 0.0 は補正なし、0.85 に近いほど強く補正する。
        float strokeSmoothingStrength_ = 0.35f;

        // 描き終わったあとに、点列を何回ならすか。
        // 0なら仕上げ平滑化なし。
        int strokeSmoothingPassCount_ = 1;

        // 簡易入り抜きを有効にするか。
        // 1本の線を短い複数ストロークに分け、始点/終点付近の半径を小さくする。
        bool strokeTaperEnabled_ = false;

        // 線全体のうち、始点側/終点側を細くする割合。
        float strokeTaperFraction_ = 0.18f;

        // 入り抜き部分の最小太さ。
        // 0.25なら通常太さの25%まで細くする。
        float strokeTaperMinimumScale_ = 0.25f;

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

        std::vector<Stroke> makeFinalizedBrushStrokes(const Stroke& sourceStroke) const;

        EditorHistorySnapshot makeHistorySnapshot() const;

        void restoreHistorySnapshot(const EditorHistorySnapshot& snapshot);

        void pushUndoSnapshot(const std::string& actionName);

        void clearHistory();

        void undoLastAction();

        void redoLastAction();

        void drawUndoRedoControls();

        void updateUndoRedoShortcuts();

        void updatePlayback(const RenderFormat& renderFormat);

        int timelineFrameForDrawingFrame(int drawingFrameIndex) const;

        int currentTimelineFrame() const;

        void seekTimelineFrame(int timelineFrame);

        void invalidateShotCameraEvaluation();

        void synchronizeShotCameraToTimeline(
            const WorkCanvas& workCanvas,
            const RenderFormat& renderFormat
        );

        void changeFrameDuration(int frameIndex, int newDurationFrames);

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

        void drawShotCameraPanel(
            const WorkCanvas& workCanvas,
            const RenderFormat& renderFormat
        );

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
