// src/drawing/AnimationFrame.h
//
// AnimationFrameは、アニメーションの1コマを表すデータです。
//
// Phase 3Dでは、1フレームが複数のDrawingLayerを持ちます。
// これにより、Frame 1、Frame 2、Frame 3... のように、
// コマごとに別の絵を描ける土台を作ります。
//
// 注意:
// 以前は strokeCount() と clearAllLayers() を AnimationFrame.cpp に書いていました。
// しかし環境側で AnimationFrame.cpp がリンクされず LNK2019 が出たため、
// Phase 3Dではこの小さい処理をヘッダー内の inline 実装にしています。

#pragma once

#include "drawing/DrawingLayer.h"

#include <string>
#include <vector>

namespace perapera
{
    class AnimationFrame
    {
    public:
        // UIに表示するフレーム名。
        // 例: "Frame 1"
        std::string name = "Frame";

        // このフレームを何コマ分保持するか。
        // Phase 3Dではまだ再生には使わないが、後でタイムラインに使う。
        int durationFrames = 1;

        // このフレームに含まれるレイヤー一覧。
        // 各フレームが自分専用のレイヤーを持つ。
        std::vector<DrawingLayer> layers;

        // このフレーム内の全ストローク数を数える。
        int strokeCount() const
        {
            int count = 0;

            for (const DrawingLayer& layer : layers)
            {
                count += static_cast<int>(layer.strokes.size());
            }

            return count;
        }

        // このフレーム内の全レイヤーを消す。
        void clearAllLayers()
        {
            for (DrawingLayer& layer : layers)
            {
                layer.clear();
            }
        }
    };
}