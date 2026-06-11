// src/ui/CanvasPreview.cpp
//
// CanvasPreviewの実装です。
// ImGuiのDrawListを使って、作画キャンバス、グリッド、撮影フレームを描きます。

#include "ui/CanvasPreview.h"

#include "drawing/WorkCanvas.h"
#include "project/RenderFormat.h"

#include "imgui.h"

#include <algorithm>

namespace perapera
{
    namespace
    {
        // 線の色などをまとめる。
        // ImGuiは色をIM_COL32(R,G,B,A)の形式で扱える。
        constexpr ImU32 PreviewBackgroundColor = IM_COL32(18, 20, 24, 255);
        constexpr ImU32 CanvasFillColor = IM_COL32(44, 48, 56, 255);
        constexpr ImU32 CanvasBorderColor = IM_COL32(150, 170, 210, 255);
        constexpr ImU32 GridLineColor = IM_COL32(80, 86, 96, 120);
        constexpr ImU32 RenderFrameColor = IM_COL32(255, 220, 80, 255);
        constexpr ImU32 WarningColor = IM_COL32(255, 120, 80, 255);

        // 値valueを、inMin〜inMaxの範囲からoutMin〜outMaxへ線形変換する。
        //
        // 例:
        // キャンバス上のx座標 0〜3840 を、
        // プレビュー上のx座標 100〜900 に変換する。
        float remap(float value, float inMin, float inMax, float outMin, float outMax)
        {
            if (inMax == inMin)
            {
                return outMin;
            }

            const float t = (value - inMin) / (inMax - inMin);
            return outMin + t * (outMax - outMin);
        }
    }

    void CanvasPreview::draw(const WorkCanvas& workCanvas, const RenderFormat& renderFormat)
    {
        ImGui::Begin("作画キャンバス プレビュー");

        ImGui::Text("作画キャンバス: %d x %d px", workCanvas.widthPx, workCanvas.heightPx);
        ImGui::Text("撮影フレーム: %d x %d px / %d fps",
            renderFormat.outputWidthPx,
            renderFormat.outputHeightPx,
            renderFormat.framesPerSecond
        );

        if (renderFormat.outputWidthPx > workCanvas.widthPx
            || renderFormat.outputHeightPx > workCanvas.heightPx)
        {
            ImGui::TextColored(
                ImVec4(1.0f, 0.45f, 0.25f, 1.0f),
                "警告: 撮影フレームが作画キャンバスより大きいです。"
            );
        }

        ImGui::Separator();

        // 現在のImGuiウィンドウ内で使える領域を取得する。
        ImVec2 availableSize = ImGui::GetContentRegionAvail();

        // 低すぎると見づらいので、最低高さを決める。
        availableSize.y = std::max(availableSize.y, 360.0f);

        ImGui::BeginChild(
            "CanvasPreviewArea",
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

        // プレビュー領域の背景を描く。
        drawList->AddRectFilled(previewMin, previewMax, PreviewBackgroundColor);

        constexpr float PaddingPx = 24.0f;

        const float availableWidth = std::max(1.0f, previewSize.x - PaddingPx * 2.0f);
        const float availableHeight = std::max(1.0f, previewSize.y - PaddingPx * 2.0f);

        // 作画キャンバスがプレビュー領域に収まるように縮小率を決める。
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

        // 作画キャンバス本体を描く。
        drawList->AddRectFilled(canvasMin, canvasMax, CanvasFillColor);
        drawList->AddRect(canvasMin, canvasMax, CanvasBorderColor, 0.0f, 0, 2.0f);

        // グリッドを描く。
        // 実寸200pxごとに線を引き、キャンバスの広さを見やすくする。
        constexpr int GridSpacingCanvasPx = 200;

        for (int x = 0; x <= workCanvas.widthPx; x += GridSpacingCanvasPx)
        {
            const float previewX = remap(
                static_cast<float>(x),
                0.0f,
                static_cast<float>(workCanvas.widthPx),
                canvasMin.x,
                canvasMax.x
            );

            drawList->AddLine(
                ImVec2(previewX, canvasMin.y),
                ImVec2(previewX, canvasMax.y),
                GridLineColor
            );
        }

        for (int y = 0; y <= workCanvas.heightPx; y += GridSpacingCanvasPx)
        {
            const float previewY = remap(
                static_cast<float>(y),
                0.0f,
                static_cast<float>(workCanvas.heightPx),
                canvasMin.y,
                canvasMax.y
            );

            drawList->AddLine(
                ImVec2(canvasMin.x, previewY),
                ImVec2(canvasMax.x, previewY),
                GridLineColor
            );
        }

        // 撮影フレームを作画キャンバス中央に仮配置する。
        //
        // 今後カメラを実装したら、この撮影フレームはカメラの位置・ズーム・画角で動く。
        // Phase 2では、まず中央固定の切り出し枠として表示する。
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

        // ImGuiに「この大きさの領域を使った」と伝える。
        // DrawListで直接描く場合でも、Dummyを置かないとレイアウトが崩れる。
        ImGui::Dummy(previewSize);

        ImGui::EndChild();
        ImGui::End();
    }
}