// src/camera/ShotCamera2D.h
//
// ShotCamera2Dは、手描き用の広いWorkCanvasから
// 最終映像として切り出す範囲を決める「撮影用2Dカメラ」です。
//
// 重要:
// - これは将来追加する3D作画補助用カメラとは別概念です。
// - ShotCamera2Dは最終PNG/動画の切り出しに影響します。
// - 3D補助カメラは床グリッド、人形、箱などの見え方を決める予定です。

#pragma once

namespace perapera
{
    class WorkCanvas;
    class RenderFormat;

    struct CanvasRect
    {
        float minX = 0.0f;
        float minY = 0.0f;
        float width = 1.0f;
        float height = 1.0f;
    };

    class ShotCamera2D
    {
    public:
        // 撮影範囲の中心座標。
        // WorkCanvas上のピクセル座標として扱う。
        float centerX = 1920.0f;
        float centerY = 1080.0f;

        // 撮影ズーム。
        // 1.0なら出力サイズと同じ広さを切り出す。
        // 2.0なら半分の広さを切り出して拡大する。
        // 0.5なら2倍の広さを切り出して縮小する。
        float zoom = 1.0f;

        void resetToCanvasCenter(const WorkCanvas& workCanvas);

        void resetZoom();

        void clampToReasonableValues(
            const WorkCanvas& workCanvas,
            const RenderFormat& renderFormat
        );

        CanvasRect calculateViewportRect(const RenderFormat& renderFormat) const;

        float canvasToOutputScaleX(const RenderFormat& renderFormat) const;

        float canvasToOutputScaleY(const RenderFormat& renderFormat) const;
    };
}
