// src/drawing/DrawingLayer.cpp
//
// DrawingLayerの処理を実装するファイルです。
// Phase 3Bでは、ストロークの有無確認と全消去だけを担当します。

#include "drawing/DrawingLayer.h"

namespace perapera
{
    bool DrawingLayer::hasStrokes() const
    {
        return !strokes.empty();
    }

    void DrawingLayer::clear()
    {
        strokes.clear();
    }
}