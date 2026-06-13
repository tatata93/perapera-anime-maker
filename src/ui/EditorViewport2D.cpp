// src/ui/EditorViewport2D.cpp

#include "ui/EditorViewport2D.h"

#include "drawing/WorkCanvas.h"

#include <algorithm>
#include <cmath>

namespace perapera
{
    void EditorViewport2D::ensureInitialized(const WorkCanvas& workCanvas)
    {
        if (!initialized_)
        {
            reset(workCanvas);
        }
    }

    void EditorViewport2D::reset(const WorkCanvas& workCanvas)
    {
        centerX = static_cast<float>(workCanvas.widthPx) * 0.5f;
        centerY = static_cast<float>(workCanvas.heightPx) * 0.5f;
        zoom = 1.0f;
        initialized_ = true;
    }

    bool EditorViewport2D::panByScreenDelta(
        float screenDeltaX,
        float screenDeltaY,
        float baseCanvasScale,
        float viewportWidthPx,
        float viewportHeightPx,
        const WorkCanvas& workCanvas
    )
    {
        const float scale = canvasScreenScale(baseCanvasScale);

        if (scale <= 0.0001f)
        {
            return false;
        }

        if (std::abs(screenDeltaX) <= 0.0001f
            && std::abs(screenDeltaY) <= 0.0001f)
        {
            return false;
        }

        centerX -= screenDeltaX / scale;
        centerY -= screenDeltaY / scale;
        clampToCanvas(
            baseCanvasScale,
            viewportWidthPx,
            viewportHeightPx,
            workCanvas
        );
        return true;
    }

    bool EditorViewport2D::zoomAtScreenPoint(
        float screenOffsetFromCenterX,
        float screenOffsetFromCenterY,
        float wheelDelta,
        float baseCanvasScale,
        float viewportWidthPx,
        float viewportHeightPx,
        const WorkCanvas& workCanvas
    )
    {
        const float oldScale = canvasScreenScale(baseCanvasScale);

        if (oldScale <= 0.0001f || std::abs(wheelDelta) <= 0.0001f)
        {
            return false;
        }

        const float anchorCanvasX =
            centerX + screenOffsetFromCenterX / oldScale;
        const float anchorCanvasY =
            centerY + screenOffsetFromCenterY / oldScale;

        const float oldZoom = zoom;
        zoom = std::clamp(
            zoom * std::pow(zoomStep_, wheelDelta),
            0.25f,
            16.0f
        );

        if (std::abs(zoom - oldZoom) <= 0.0001f)
        {
            return false;
        }

        const float newScale = canvasScreenScale(baseCanvasScale);
        centerX = anchorCanvasX - screenOffsetFromCenterX / newScale;
        centerY = anchorCanvasY - screenOffsetFromCenterY / newScale;
        clampToCanvas(
            baseCanvasScale,
            viewportWidthPx,
            viewportHeightPx,
            workCanvas
        );
        return true;
    }

    float EditorViewport2D::canvasScreenScale(float baseCanvasScale) const
    {
        return std::max(0.0001f, baseCanvasScale)
            * std::clamp(zoom, 0.25f, 16.0f);
    }

    void EditorViewport2D::clampToCanvas(
        float baseCanvasScale,
        float viewportWidthPx,
        float viewportHeightPx,
        const WorkCanvas& workCanvas
    )
    {
        zoom = std::clamp(zoom, 0.25f, 16.0f);

        const float scale = canvasScreenScale(baseCanvasScale);
        const float visibleCanvasWidth =
            std::max(1.0f, viewportWidthPx) / scale;
        const float visibleCanvasHeight =
            std::max(1.0f, viewportHeightPx) / scale;

        const float canvasWidth = static_cast<float>(workCanvas.widthPx);
        const float canvasHeight = static_cast<float>(workCanvas.heightPx);

        constexpr float MinimumVisibleScreenPx = 96.0f;

        const float minimumVisibleCanvasWidth =
            std::min(canvasWidth, MinimumVisibleScreenPx / scale);
        const float minimumVisibleCanvasHeight =
            std::min(canvasHeight, MinimumVisibleScreenPx / scale);

        const float halfVisibleWidth = visibleCanvasWidth * 0.5f;
        const float halfVisibleHeight = visibleCanvasHeight * 0.5f;

        centerX = std::clamp(
            centerX,
            minimumVisibleCanvasWidth - halfVisibleWidth,
            canvasWidth - minimumVisibleCanvasWidth + halfVisibleWidth
        );

        centerY = std::clamp(
            centerY,
            minimumVisibleCanvasHeight - halfVisibleHeight,
            canvasHeight - minimumVisibleCanvasHeight + halfVisibleHeight
        );
    }
}
