// src/ui/DrawingCanvasPanel.h
//
// DrawingCanvasPanelは、ImGui上に簡易作画キャンバスを表示するUIです。
//
// Phase 3Bでは、複数レイヤーに対応します。
// まだ本物の画像バッファには描きません。
// 各レイヤーがStroke配列を持ち、ImGuiのDrawListで仮表示します。

#pragma once

#include "drawing/Brush.h"
#include "drawing/DrawingLayer.h"
#include "drawing/Stroke.h"

#include <vector>

namespace perapera
{
    class WorkCanvas;
    class RenderFormat;

    class DrawingCanvasPanel
    {
    public:
        // コンストラクタ。
        // 最初に最低1枚のレイヤーを作る。
        DrawingCanvasPanel();

        // ImGuiウィンドウとして簡易作画キャンバスを描く。
        void draw(WorkCanvas& workCanvas, const RenderFormat& renderFormat);

    private:
        // 現在のペン設定。
        Brush brush_;

        // レイヤー一覧。
        // 0番が奥、後ろの番号ほど手前に描かれる。
        std::vector<DrawingLayer> layers_;

        // 現在描き込み先になっているレイヤー番号。
        int activeLayerIndex_ = 0;

        // 今まさに描いている途中の線。
        Stroke currentStroke_;

        // マウス左ボタンを押して描画中かどうか。
        bool isDrawing_ = false;

        // 次に作るレイヤーの連番。
        int nextLayerNumber_ = 2;

        // アクティブレイヤーを安全な範囲に補正する。
        void clampActiveLayerIndex();

        // アクティブレイヤーを取得する。
        DrawingLayer* activeLayer();

        // アクティブレイヤーを取得する。const版。
        const DrawingLayer* activeLayer() const;

        // 新しいレイヤーを追加する。
        void addLayer();

        // アクティブレイヤーを削除する。
        void deleteActiveLayer();

        // アクティブレイヤーを上へ移動する。
        void moveActiveLayerUp();

        // アクティブレイヤーを下へ移動する。
        void moveActiveLayerDown();

        // レイヤー管理UIを描く。
        void drawLayerPanel();

        // すべてのレイヤーの線を消す。
        void clearAllLayers();
    };
}