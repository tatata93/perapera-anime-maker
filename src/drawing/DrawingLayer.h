// src/drawing/DrawingLayer.h
//
// DrawingLayerは、作画キャンバス上の1枚のレイヤーを表すデータです。
//
// アニメ制作では、背景、セルA、セルB、ガイドなどを分けて扱います。
// Phase 3Bでは、まず「複数レイヤーに線を分けて保存する」土台を作ります。
// まだ合成モード、ロック、レイヤー名変更、画像保存は実装しません。

#pragma once

#include "drawing/Stroke.h"

#include <string>
#include <vector>

namespace perapera
{
    class DrawingLayer
    {
    public:
        // UIに表示するレイヤー名。
        // 例: "Layer 1", "Cel A", "Background Rough"
        std::string name = "Layer";

        // 表示するかどうか。
        // falseにすると、このレイヤーの線は画面に出ない。
        bool visible = true;

        // レイヤー全体の不透明度。
        // 1.0で完全表示、0.0で完全透明。
        float opacity = 1.0f;

        // このレイヤーに描かれたストローク一覧。
        std::vector<Stroke> strokes;

        // このレイヤーに線が1本以上あるかを返す。
        bool hasStrokes() const;

        // このレイヤーの線をすべて消す。
        void clear();
    };
}
