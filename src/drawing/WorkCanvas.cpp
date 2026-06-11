// src/drawing/WorkCanvas.cpp
//
// WorkCanvasの処理を実装するファイルです。
// 作画キャンバスのサイズ補正やアスペクト比計算を担当します。

#include "drawing/WorkCanvas.h"

#include <algorithm>

namespace perapera
{
    void WorkCanvas::clampToReasonableSize()
    {
        // 最小サイズ。
        // これより小さいキャンバスは実用上ほぼ意味がないため制限する。
        constexpr int MinimumCanvasSizePx = 16;

        // 最大サイズ。
        // あまり大きいとメモリを大量に使うため、初期段階では控えめに制限する。
        // 将来的には設定で変更できるようにしてもよい。
        constexpr int MaximumCanvasSizePx = 32768;

        widthPx = std::clamp(widthPx, MinimumCanvasSizePx, MaximumCanvasSizePx);
        heightPx = std::clamp(heightPx, MinimumCanvasSizePx, MaximumCanvasSizePx);
    }

    float WorkCanvas::aspectRatio() const
    {
        if (heightPx <= 0)
        {
            return 1.0f;
        }

        return static_cast<float>(widthPx) / static_cast<float>(heightPx);
    }
}