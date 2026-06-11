// src/drawing/Stroke.cpp
//
// Strokeの処理を実装するファイルです。
// Phase 3Aでは、点の追加と、描画可能かどうかの確認だけを行います。

#include "drawing/Stroke.h"

namespace perapera
{
    void Stroke::addPoint(CanvasPoint point)
    {
        points.push_back(point);
    }

    bool Stroke::hasDrawablePoints() const
    {
        // 点が1つでもあれば、点として表示できる。
        // 点が2つ以上あれば、線として表示できる。
        return !points.empty();
    }
}