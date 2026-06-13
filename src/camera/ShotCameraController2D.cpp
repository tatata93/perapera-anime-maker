// src/camera/ShotCameraController2D.h
//
// ShotCamera2Dの直接操作に必要なパン・ズーム計算を管理します。
// ImGuiなどのUIライブラリには依存しません。

#pragma once

namespace perapera
{
    class RenderFormat;
    class ShotCamera2D;
    class WorkCanvas;

    class ShotCameraController2D
    {
    public:
        bool panByScreenDelta(
            ShotCamera2D& camera,
            float screenDeltaX,
            float screenDeltaY,
            float canvasScreenScale,
            const WorkCanvas& workCanvas,
            const RenderFormat& renderFormat
        ) const;

        bool zoomAtCanvasPoint(
            ShotCamera2D& camera,
            float canvasX,
            float canvasY,
            float wheelDelta,
            const WorkCanvas& workCanvas,
            const RenderFormat& renderFormat
        ) const;

        float zoomStep() const;

        void setZoomStep(float zoomStep);

    private:
        float zoomStep_ = 1.15f;
    };
}
