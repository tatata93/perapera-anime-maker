// src/drawing/Stroke.h
//
// Strokeは、ユーザーがペンで描いた1本の線を表すデータです。
//
// 例:
// マウスを押して、動かして、離すまでを1つのStrokeとして保存します。
// Phase 3Aでは、点の列を保存し、それをImGuiのDrawListで線として描画します。

#pragma once

#include "drawing/Brush.h"

#include <vector>

namespace perapera
{
    // 作画キャンバス上の座標。
    // 画面上の座標ではなく、WorkCanvas上のピクセル座標として扱う。
    struct CanvasPoint
    {
        float x = 0.0f;
        float y = 0.0f;
    };

    class Stroke
    {
    public:
        // このストロークの色。
        ColorRgba color;

        // このストロークのペン半径。
        float radiusPx = 4.0f;

        // 線を構成する点の列。
        std::vector<CanvasPoint> points;

        // 点を1つ追加する。
        void addPoint(CanvasPoint point);

        // 線として描けるだけの点があるかを返す。
        bool hasDrawablePoints() const;
    };
}