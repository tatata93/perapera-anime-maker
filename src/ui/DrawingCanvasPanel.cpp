
// src/ui/DrawingCanvasPanel.cpp
//
// DrawingCanvasPanelの実装です。
// ImGuiのDrawListを使って、作画キャンバス、撮影フレーム、ユーザーの線を描きます。

#include "ui/DrawingCanvasPanel.h"

#include "drawing/WorkCanvas.h"
#include "project/RenderFormat.h"

#include "imgui.h"

#include <algorithm>
#include <cmath>

namespace perapera
{
    namespace
    {
        constexpr ImU32 PreviewBackgroundColor = IM_COL32(18, 20, 24, 255);
        constexpr ImU32 CanvasFillColor = IM_COL32(44, 48, 56, 255);
        constexpr ImU32 CanvasBorderColor = IM_COL32(150, 170, 210, 255);
        constexpr ImU32 GridLineColor = IM_COL32(80, 86, 96, 120);
        constexpr ImU32 RenderFrameColor = IM_COL32(255, 220, 80, 255);
        constexpr ImU32 WarningColor = IM_COL32(255, 120, 80, 255);

        float distanceSquared(CanvasPoint a, CanvasPoint b)
        {
            const float dx = a.x - b.x;
            const float dy = a.y - b.y;
            return dx * dx + dy * dy;
        }

        ImU32 toImGuiColor(ColorRgba color)
        {
            const int r = static_cast<int>(std::clamp(color.r, 0.0f, 1.0f) * 255.0f);
            const int g = static_cast<int>(std::clamp(color.g, 0.0f, 1.0f) * 255.0f);
            const int b = static_cast<int>(std::clamp(color.b, 0.0f, 1.0f) * 255.0f);
            const int a = static_cast<int>(std::clamp(color.a, 0.0f, 1.0f) * 255.0f);

            return IM_COL32(r, g, b, a);
        }

        CanvasPoint screenToCanvasPoint(
            ImVec2 screenPoint,
            ImVec2 canvasMin,
            float scale
        )
        {
            // 画面上のマウス座標を、WorkCanvas上の座標に変換する。
            //
            // 例:
            // 表示上では300pxに縮小されていても、
            // 実際のキャンバスが3000pxなら、scaleで割って実寸に戻す。
            return CanvasPoint{
                (screenPoint.x - canvasMin.x) / scale,
                (screenPoint.y - canvasMin.y) / scale
            };
        }

        ImVec2 canvasToScreenPoint(
            CanvasPoint canvasPoint,
            ImVec2 canvasMin,
            float scale
        )
        {
            // WorkCanvas上の座標を、画面表示用の座標に変換する。
            return ImVec2(
                canvasMin.x + canvasPoint.x * scale,
                canvasMin.y + canvasPoint.y * scale
            );
        }

        bool isInsideRect(ImVec2 point, ImVec2 min, ImVec2 max)
        {
            return point.x >= min.x
                && point.y >= min.y
                && point.x <= max.x
                && point.y <= max.y;
        }

        void drawStroke(
            ImDrawList* drawList,
            const Stroke& stroke,
            ImVec2 canvasMin,
            float scale
        )
        {
            if (!stroke.hasDrawablePoints())
            {
                return;
            }

            const ImU32 color = toImGuiColor(stroke.color);

            // キャンバスが縮小表示されているときは、線の太さも縮小する。
            // ただし細すぎると見えないので、最低1.0pxは確保する。
            const float thickness = std::max(1.0f, stroke.radiusPx * 2.0f * scale);

            if (stroke.points.size() == 1)
            {
                const ImVec2 center = canvasToScreenPoint(stroke.points.front(), canvasMin, scale);
                drawList->AddCircleFilled(center, thickness * 0.5f, color);
                return;
            }

            for (std::size_t i = 1; i < stroke.points.size(); ++i)
            {
                const ImVec2 a = canvasToScreenPoint(stroke.points[i - 1], canvasMin, scale);
                const ImVec2 b = canvasToScreenPoint(stroke.points[i], canvasMin, scale);

                drawList->AddLine(a, b, color, thickness);
            }
        }
    }

    void DrawingCanvasPanel::clearAllStrokes()
    {
        strokes_.clear();
        currentStroke_.points.clear();
        isDrawing_ = false;
    }

    void DrawingCanvasPanel::draw(WorkCanvas& workCanvas, const RenderFormat& renderFormat)
    {
        ImGui::Begin("簡易作画キャンバス");

        ImGui::Text("左ドラッグで線を描けます。まだ保存・レイヤー・消しゴムはありません。");

        ImGui::SliderFloat("ペン半径 px", &brush_.radiusPx, 1.0f, 40.0f);

        float color[4] = {
            brush_.color.r,
            brush_.color.g,
            brush_.color.b,
            brush_.color.a
        };

        if (ImGui::ColorEdit4("ペン色", color))
        {
            brush_.color.r = color[0];
            brush_.color.g = color[1];
            brush_.color.b = color[2];
            brush_.color.a = color[3];
        }

        if (ImGui::Button("全消去"))
        {
            clearAllStrokes();
        }

        ImGui::SameLine();
        ImGui::Text("ストローク数: %d", static_cast<int>(strokes_.size()));

        ImGui::Separator();

        ImVec2 availableSize = ImGui::GetContentRegionAvail();
        availableSize.y = std::max(availableSize.y, 420.0f);

        ImGui::BeginChild(
            "DrawingCanvasArea",
            availableSize,
            true,
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse
        );

        ImDrawList* drawList = ImGui::GetWindowDrawList();

        const ImVec2 previewMin = ImGui::GetCursorScreenPos();
        const ImVec2 previewSize = ImGui::GetContentRegionAvail();
        const ImVec2 previewMax(
            previewMin.x + previewSize.x,
            previewMin.y + previewSize.y
        );

        // InvisibleButtonは見えないボタンです。
        // これを置くことで、ImGuiに「この領域でマウス操作を受け取る」と伝えます。
        ImGui::InvisibleButton(
            "DrawingCanvasInput",
            previewSize,
            ImGuiButtonFlags_MouseButtonLeft
        );

        const bool isInputAreaHovered = ImGui::IsItemHovered();

        drawList->AddRectFilled(previewMin, previewMax, PreviewBackgroundColor);

        constexpr float PaddingPx = 24.0f;

        const float availableWidth = std::max(1.0f, previewSize.x - PaddingPx * 2.0f);
        const float availableHeight = std::max(1.0f, previewSize.y - PaddingPx * 2.0f);

        const float scaleX = availableWidth / static_cast<float>(workCanvas.widthPx);
        const float scaleY = availableHeight / static_cast<float>(workCanvas.heightPx);
        const float scale = std::min(scaleX, scaleY);

        const float canvasPreviewWidth = static_cast<float>(workCanvas.widthPx) * scale;
        const float canvasPreviewHeight = static_cast<float>(workCanvas.heightPx) * scale;

        const ImVec2 canvasMin(
            previewMin.x + (previewSize.x - canvasPreviewWidth) * 0.5f,
            previewMin.y + (previewSize.y - canvasPreviewHeight) * 0.5f
        );

        const ImVec2 canvasMax(
            canvasMin.x + canvasPreviewWidth,
            canvasMin.y + canvasPreviewHeight
        );

        drawList->AddRectFilled(canvasMin, canvasMax, CanvasFillColor);
        drawList->AddRect(canvasMin, canvasMax, CanvasBorderColor, 0.0f, 0, 2.0f);

        // グリッド表示。
        constexpr int GridSpacingCanvasPx = 200;

        for (int x = 0; x <= workCanvas.widthPx; x += GridSpacingCanvasPx)
        {
            const float screenX = canvasMin.x + static_cast<float>(x) * scale;
            drawList->AddLine(
                ImVec2(screenX, canvasMin.y),
                ImVec2(screenX, canvasMax.y),
                GridLineColor
            );
        }

        for (int y = 0; y <= workCanvas.heightPx; y += GridSpacingCanvasPx)
        {
            const float screenY = canvasMin.y + static_cast<float>(y) * scale;
            drawList->AddLine(
                ImVec2(canvasMin.x, screenY),
                ImVec2(canvasMax.x, screenY),
                GridLineColor
            );
        }

        // 既存のストロークを描く。
        for (const Stroke& stroke : strokes_)
        {
            drawStroke(drawList, stroke, canvasMin, scale);
        }

        // 描画中のストロークも表示する。
        if (isDrawing_)
        {
            drawStroke(drawList, currentStroke_, canvasMin, scale);
        }

        // 撮影フレームを中央に仮表示する。
        const float renderFramePreviewWidth =
            static_cast<float>(renderFormat.outputWidthPx) * scale;

        const float renderFramePreviewHeight =
            static_cast<float>(renderFormat.outputHeightPx) * scale;

        const ImVec2 renderFrameMin(
            canvasMin.x + (canvasPreviewWidth - renderFramePreviewWidth) * 0.5f,
            canvasMin.y + (canvasPreviewHeight - renderFramePreviewHeight) * 0.5f
        );

        const ImVec2 renderFrameMax(
            renderFrameMin.x + renderFramePreviewWidth,
            renderFrameMin.y + renderFramePreviewHeight
        );

        const bool renderFrameFits =
            renderFrameMin.x >= canvasMin.x
            && renderFrameMin.y >= canvasMin.y
            && renderFrameMax.x <= canvasMax.x
            && renderFrameMax.y <= canvasMax.y;

        drawList->AddRect(
            renderFrameMin,
            renderFrameMax,
            renderFrameFits ? RenderFrameColor : WarningColor,
            0.0f,
            0,
            3.0f
        );

        drawList->AddText(
            ImVec2(renderFrameMin.x + 8.0f, renderFrameMin.y + 8.0f),
            renderFrameFits ? RenderFrameColor : WarningColor,
            "撮影フレーム"
        );

        // マウス入力を作画キャンバス座標に変換して、Strokeとして保存する。
        const ImVec2 mousePosition = ImGui::GetIO().MousePos;
        const bool mouseInsideCanvas = isInsideRect(mousePosition, canvasMin, canvasMax);

        if (isInputAreaHovered
            && mouseInsideCanvas
            && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            currentStroke_ = Stroke{};
            currentStroke_.color = brush_.color;
            currentStroke_.radiusPx = brush_.radiusPx;
            currentStroke_.addPoint(screenToCanvasPoint(mousePosition, canvasMin, scale));
            isDrawing_ = true;
        }

        if (isDrawing_ && ImGui::IsMouseDown(ImGuiMouseButton_Left))
        {
            if (mouseInsideCanvas)
            {
                const CanvasPoint nextPoint =
                    screenToCanvasPoint(mousePosition, canvasMin, scale);

                // 同じような点を大量に保存しすぎると重くなるので、
                // 前の点から少し離れたときだけ追加する。
                constexpr float MinimumPointDistancePx = 2.0f;

                if (currentStroke_.points.empty()
                    || distanceSquared(currentStroke_.points.back(), nextPoint)
                        >= MinimumPointDistancePx * MinimumPointDistancePx)
                {
                    currentStroke_.addPoint(nextPoint);
                }
            }
        }

        if (isDrawing_ && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
        {
            if (currentStroke_.hasDrawablePoints())
            {
                strokes_.push_back(currentStroke_);
            }

            currentStroke_.points.clear();
            isDrawing_ = false;
        }

        ImGui::EndChild();
        ImGui::End();
    }
}