// src/camera/ShotCamera2D.cpp
//
// 撮影用2Dカメラの実装です。
// WorkCanvasのどこを最終出力に入れるかを、中心座標とズームで管理します。

#include "camera/ShotCamera2D.h"

#include "drawing/WorkCanvas.h"
#include "project/RenderFormat.h"

#include <algorithm>

namespace perapera
{
    void ShotCamera2D::resetToCanvasCenter(const WorkCanvas& workCanvas)
    {
        centerX = static_cast<float>(workCanvas.widthPx) * 0.5f;
        centerY = static_cast<float>(workCanvas.heightPx) * 0.5f;
    }

    void ShotCamera2D::resetZoom()
    {
        zoom = 1.0f;
    }

    void ShotCamera2D::clampToReasonableValues(
        const WorkCanvas& workCanvas,
        const RenderFormat& renderFormat
    )
    {
        zoom = std::clamp(zoom, 0.1f, 8.0f);

        const CanvasRect viewport = calculateViewportRect(renderFormat);
        const float halfWidth = viewport.width * 0.5f;
        const float halfHeight = viewport.height * 0.5f;

        const float minCenterX = halfWidth;
        const float maxCenterX = static_cast<float>(workCanvas.widthPx) - halfWidth;
        const float minCenterY = halfHeight;
        const float maxCenterY = static_cast<float>(workCanvas.heightPx) - halfHeight;

        if (minCenterX <= maxCenterX)
        {
            centerX = std::clamp(centerX, minCenterX, maxCenterX);
        }
        else
        {
            centerX = static_cast<float>(workCanvas.widthPx) * 0.5f;
        }

        if (minCenterY <= maxCenterY)
        {
            centerY = std::clamp(centerY, minCenterY, maxCenterY);
        }
        else
        {
            centerY = static_cast<float>(workCanvas.heightPx) * 0.5f;
        }
    }

    CanvasRect ShotCamera2D::calculateViewportRect(const RenderFormat& renderFormat) const
    {
        const float safeZoom = std::clamp(zoom, 0.1f, 8.0f);
        const float viewportWidth = static_cast<float>(renderFormat.outputWidthPx) / safeZoom;
        const float viewportHeight = static_cast<float>(renderFormat.outputHeightPx) / safeZoom;

        CanvasRect rect;
        rect.width = viewportWidth;
        rect.height = viewportHeight;
        rect.minX = centerX - viewportWidth * 0.5f;
        rect.minY = centerY - viewportHeight * 0.5f;
        return rect;
    }

    float ShotCamera2D::canvasToOutputScaleX(const RenderFormat& renderFormat) const
    {
        const CanvasRect viewport = calculateViewportRect(renderFormat);

        if (viewport.width <= 0.0001f)
        {
            return 1.0f;
        }

        return static_cast<float>(renderFormat.outputWidthPx) / viewport.width;
    }

    float ShotCamera2D::canvasToOutputScaleY(const RenderFormat& renderFormat) const
    {
        const CanvasRect viewport = calculateViewportRect(renderFormat);

        if (viewport.height <= 0.0001f)
        {
            return 1.0f;
        }

        return static_cast<float>(renderFormat.outputHeightPx) / viewport.height;
    }
}
