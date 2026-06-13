// src/camera/ShotCameraController2D.cpp

#include "camera/ShotCameraController2D.h"

#include "camera/ShotCamera2D.h"
#include "drawing/WorkCanvas.h"
#include "project/RenderFormat.h"

#include <algorithm>
#include <cmath>

namespace perapera
{
    bool ShotCameraController2D::panByScreenDelta(
        ShotCamera2D& camera,
        float screenDeltaX,
        float screenDeltaY,
        float canvasScreenScale,
        const WorkCanvas& workCanvas,
        const RenderFormat& renderFormat
    ) const
    {
        if (canvasScreenScale <= 0.0001f)
        {
            return false;
        }

        if (std::abs(screenDeltaX) <= 0.0001f
            && std::abs(screenDeltaY) <= 0.0001f)
        {
            return false;
        }

        camera.centerX += screenDeltaX / canvasScreenScale;
        camera.centerY += screenDeltaY / canvasScreenScale;
        camera.clampToReasonableValues(workCanvas, renderFormat);
        return true;
    }

    bool ShotCameraController2D::zoomAtCanvasPoint(
        ShotCamera2D& camera,
        float canvasX,
        float canvasY,
        float wheelDelta,
        const WorkCanvas& workCanvas,
        const RenderFormat& renderFormat
    ) const
    {
        if (std::abs(wheelDelta) <= 0.0001f)
        {
            return false;
        }

        const CanvasRect oldViewport = camera.calculateViewportRect(renderFormat);

        if (oldViewport.width <= 0.0001f || oldViewport.height <= 0.0001f)
        {
            return false;
        }

        const float anchorX =
            (canvasX - oldViewport.minX) / oldViewport.width;
        const float anchorY =
            (canvasY - oldViewport.minY) / oldViewport.height;

        const float zoomMultiplier = std::pow(zoomStep_, wheelDelta);
        const float oldZoom = camera.zoom;
        camera.zoom = std::clamp(camera.zoom * zoomMultiplier, 0.1f, 8.0f);

        if (std::abs(camera.zoom - oldZoom) <= 0.0001f)
        {
            return false;
        }

        const CanvasRect newViewport = camera.calculateViewportRect(renderFormat);
        camera.centerX =
            canvasX + (0.5f - anchorX) * newViewport.width;
        camera.centerY =
            canvasY + (0.5f - anchorY) * newViewport.height;
        camera.clampToReasonableValues(workCanvas, renderFormat);
        return true;
    }

    float ShotCameraController2D::zoomStep() const
    {
        return zoomStep_;
    }

    void ShotCameraController2D::setZoomStep(float zoomStep)
    {
        zoomStep_ = std::clamp(zoomStep, 1.01f, 2.0f);
    }
}
