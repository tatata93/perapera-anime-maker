// src/project/RenderFormat.cpp
//
// RenderFormatの処理を実装するファイルです。
// 出力解像度、FPS、ピクセルアスペクト比の補正を担当します。

#include "project/RenderFormat.h"

#include <algorithm>

namespace perapera
{
    void RenderFormat::clampToReasonableValues()
    {
        constexpr int MinimumOutputSizePx = 16;
        constexpr int MaximumOutputSizePx = 16384;

        constexpr int MinimumFps = 1;
        constexpr int MaximumFps = 240;

        constexpr float MinimumPixelAspectRatio = 0.1f;
        constexpr float MaximumPixelAspectRatio = 10.0f;

        outputWidthPx = std::clamp(outputWidthPx, MinimumOutputSizePx, MaximumOutputSizePx);
        outputHeightPx = std::clamp(outputHeightPx, MinimumOutputSizePx, MaximumOutputSizePx);

        framesPerSecond = std::clamp(framesPerSecond, MinimumFps, MaximumFps);

        pixelAspectRatio = std::clamp(
            pixelAspectRatio,
            MinimumPixelAspectRatio,
            MaximumPixelAspectRatio
        );
    }

    float RenderFormat::displayAspectRatio() const
    {
        if (outputHeightPx <= 0)
        {
            return 1.0f;
        }

        return (static_cast<float>(outputWidthPx) * pixelAspectRatio)
            / static_cast<float>(outputHeightPx);
    }
}