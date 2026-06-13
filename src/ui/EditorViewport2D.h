// src/ui/EditorViewport2D.h
//
// 作画キャンバスを編集画面上で見るためのパン・ズーム状態です。
// 最終出力に使うShotCamera2Dとは独立しています。

#pragma once

namespace perapera
{
    class WorkCanvas;

    class EditorViewport2D
    {
    public:
        float centerX = 0.0f;
        float centerY = 0.0f;
        float zoom = 1.0f;

        void ensureInitialized(const WorkCanvas& workCanvas);

        void reset(const WorkCanvas& workCanvas);

        bool panByScreenDelta(
            float screenDeltaX,
            float screenDeltaY,
            float baseCanvasScale,
            float viewportWidthPx,
            float viewportHeightPx,
            const WorkCanvas& workCanvas
        );

        bool zoomAtScreenPoint(
            float screenOffsetFromCenterX,
            float screenOffsetFromCenterY,
            float wheelDelta,
            float baseCanvasScale,
            float viewportWidthPx,
            float viewportHeightPx,
            const WorkCanvas& workCanvas
        );

        float canvasScreenScale(float baseCanvasScale) const;

        void clampToCanvas(
            float baseCanvasScale,
            float viewportWidthPx,
            float viewportHeightPx,
            const WorkCanvas& workCanvas
        );

    private:
        bool initialized_ = false;
        float zoomStep_ = 1.15f;
    };
}
