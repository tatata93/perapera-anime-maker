// src/ui/CanvasPreview.h
//
// CanvasPreviewは、ImGui上に作画キャンバスと撮影フレームを仮表示するUIです。
//
// Phase 2では、まだ本物の絵は描きません。
// まず「広い作画キャンバス」と「その中から切り出す撮影フレーム」を
// 視覚的に確認できるようにします。

#pragma once

namespace perapera
{
    class WorkCanvas;
    class RenderFormat;

    class CanvasPreview
    {
    public:
        // ImGuiウィンドウとしてキャンバスプレビューを描く。
        void draw(const WorkCanvas& workCanvas, const RenderFormat& renderFormat);
    };
}