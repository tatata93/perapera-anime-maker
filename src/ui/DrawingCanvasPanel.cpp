// src/ui/DrawingCanvasPanel.cpp
//
// DrawingCanvasPanelの実装です。
// ImGuiのDrawListを使って、作画キャンバス、撮影フレーム、ユーザーの線を描きます。
//
// Phase 3Bでは、複数レイヤーを扱います。
// レイヤーごとに表示/非表示、不透明度、描き込み先を管理します。

#include "ui/DrawingCanvasPanel.h"

#include "drawing/WorkCanvas.h"
#include "project/RenderFormat.h"

#include "imgui.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <utility>

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

        ImU32 toImGuiColor(ColorRgba color, float layerOpacity)
        {
            // レイヤーの不透明度をストロークのアルファに掛ける。
            // これにより、レイヤー全体を薄く表示できる。
            const float finalAlpha = std::clamp(color.a * layerOpacity, 0.0f, 1.0f);

            const int r = static_cast<int>(std::clamp(color.r, 0.0f, 1.0f) * 255.0f);
            const int g = static_cast<int>(std::clamp(color.g, 0.0f, 1.0f) * 255.0f);
            const int b = static_cast<int>(std::clamp(color.b, 0.0f, 1.0f) * 255.0f);
            const int a = static_cast<int>(finalAlpha * 255.0f);

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
            float scale,
            float layerOpacity
        )
        {
            if (!stroke.hasDrawablePoints())
            {
                return;
            }

            const ImU32 color = toImGuiColor(stroke.color, layerOpacity);

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

        std::string makeDefaultLayerName(int layerNumber)
        {
            return "Layer " + std::to_string(layerNumber);
        }
    }

    DrawingCanvasPanel::DrawingCanvasPanel()
    {
        DrawingLayer firstLayer;
        firstLayer.name = "Layer 1";
        firstLayer.visible = true;
        firstLayer.opacity = 1.0f;

        layers_.push_back(firstLayer);
        activeLayerIndex_ = 0;
        nextLayerNumber_ = 2;
    }

    void DrawingCanvasPanel::clampActiveLayerIndex()
    {
        if (layers_.empty())
        {
            activeLayerIndex_ = -1;
            return;
        }

        activeLayerIndex_ = std::clamp(
            activeLayerIndex_,
            0,
            static_cast<int>(layers_.size()) - 1
        );
    }

    DrawingLayer* DrawingCanvasPanel::activeLayer()
    {
        clampActiveLayerIndex();

        if (activeLayerIndex_ < 0 || activeLayerIndex_ >= static_cast<int>(layers_.size()))
        {
            return nullptr;
        }

        return &layers_[static_cast<std::size_t>(activeLayerIndex_)];
    }

    const DrawingLayer* DrawingCanvasPanel::activeLayer() const
    {
        if (activeLayerIndex_ < 0 || activeLayerIndex_ >= static_cast<int>(layers_.size()))
        {
            return nullptr;
        }

        return &layers_[static_cast<std::size_t>(activeLayerIndex_)];
    }

    void DrawingCanvasPanel::addLayer()
    {
        DrawingLayer layer;
        layer.name = makeDefaultLayerName(nextLayerNumber_);
        layer.visible = true;
        layer.opacity = 1.0f;

        ++nextLayerNumber_;

        // 新しいレイヤーは一番手前に追加する。
        layers_.push_back(layer);
        activeLayerIndex_ = static_cast<int>(layers_.size()) - 1;
    }

    void DrawingCanvasPanel::deleteActiveLayer()
    {
        // 最低1枚は残す。
        // 0枚になると描き込み先がなくなって扱いづらいため。
        if (layers_.size() <= 1)
        {
            return;
        }

        clampActiveLayerIndex();

        layers_.erase(layers_.begin() + activeLayerIndex_);
        clampActiveLayerIndex();
    }

    void DrawingCanvasPanel::moveActiveLayerUp()
    {
        clampActiveLayerIndex();

        // 上へ移動 = 画面上で手前にする。
        if (activeLayerIndex_ < 0 || activeLayerIndex_ >= static_cast<int>(layers_.size()) - 1)
        {
            return;
        }

        std::swap(
            layers_[static_cast<std::size_t>(activeLayerIndex_)],
            layers_[static_cast<std::size_t>(activeLayerIndex_ + 1)]
        );

        ++activeLayerIndex_;
    }

    void DrawingCanvasPanel::moveActiveLayerDown()
    {
        clampActiveLayerIndex();

        // 下へ移動 = 画面上で奥にする。
        if (activeLayerIndex_ <= 0)
        {
            return;
        }

        std::swap(
            layers_[static_cast<std::size_t>(activeLayerIndex_)],
            layers_[static_cast<std::size_t>(activeLayerIndex_ - 1)]
        );

        --activeLayerIndex_;
    }

    void DrawingCanvasPanel::clearAllLayers()
    {
        for (DrawingLayer& layer : layers_)
        {
            layer.clear();
        }

        currentStroke_.points.clear();
        isDrawing_ = false;
    }

    void DrawingCanvasPanel::drawLayerPanel()
    {
        ImGui::Begin("レイヤー");

        ImGui::Text("描き込み先レイヤーを選びます。");
        ImGui::Text("上のレイヤーほど手前に表示されます。");

        ImGui::Separator();

        if (ImGui::Button("レイヤー追加"))
        {
            addLayer();
        }

        ImGui::SameLine();

        if (ImGui::Button("削除"))
        {
            deleteActiveLayer();
        }

        if (ImGui::Button("上へ"))
        {
            moveActiveLayerUp();
        }

        ImGui::SameLine();

        if (ImGui::Button("下へ"))
        {
            moveActiveLayerDown();
        }

        ImGui::Separator();

        // UI上では手前のレイヤーを上に表示したいので、
        // 配列の後ろから順に表示する。
        for (int index = static_cast<int>(layers_.size()) - 1; index >= 0; --index)
        {
            DrawingLayer& layer = layers_[static_cast<std::size_t>(index)];

            ImGui::PushID(index);

            const bool isActive = (index == activeLayerIndex_);

            if (ImGui::Selectable(layer.name.c_str(), isActive))
            {
                activeLayerIndex_ = index;
            }

            ImGui::SameLine();

            ImGui::Checkbox("表示", &layer.visible);

            ImGui::SameLine();

            ImGui::SetNextItemWidth(100.0f);
            ImGui::SliderFloat("不透明度", &layer.opacity, 0.0f, 1.0f);

            ImGui::Text("ストローク数: %d", static_cast<int>(layer.strokes.size()));

            ImGui::PopID();
        }

        ImGui::Separator();

        DrawingLayer* layer = activeLayer();

        if (layer != nullptr)
        {
            ImGui::Text("現在の描き込み先: %s", layer->name.c_str());

            if (!layer->visible)
            {
                ImGui::TextColored(
                    ImVec4(1.0f, 0.45f, 0.25f, 1.0f),
                    "注意: このレイヤーは非表示なので描き込みできません。"
                );
            }

            if (ImGui::Button("アクティブレイヤーを消去"))
            {
                layer->clear();
            }
        }

        if (ImGui::Button("全レイヤー消去"))
        {
            clearAllLayers();
        }

        ImGui::End();
    }

    void DrawingCanvasPanel::draw(WorkCanvas& workCanvas, const RenderFormat& renderFormat)
    {
        drawLayerPanel();

        ImGui::Begin("簡易作画キャンバス");

        ImGui::Text("左ドラッグで、選択中の表示レイヤーに線を描けます。");

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

        DrawingLayer* currentLayer = activeLayer();

        if (currentLayer != nullptr)
        {
            ImGui::Text("描き込み先: %s", currentLayer->name.c_str());
        }

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

        // 線がキャンバス外にはみ出して見えないように、描画範囲をキャンバス内に制限する。
        drawList->PushClipRect(canvasMin, canvasMax, true);

        // レイヤーを奥から手前へ描く。
        for (const DrawingLayer& layer : layers_)
        {
            if (!layer.visible)
            {
                continue;
            }

            for (const Stroke& stroke : layer.strokes)
            {
                drawStroke(drawList, stroke, canvasMin, scale, layer.opacity);
            }
        }

        // 描画中のストロークも表示する。
        if (isDrawing_ && currentLayer != nullptr && currentLayer->visible)
        {
            drawStroke(drawList, currentStroke_, canvasMin, scale, currentLayer->opacity);
        }

        drawList->PopClipRect();

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

        const bool canDraw =
            currentLayer != nullptr
            && currentLayer->visible;

        if (canDraw
            && isInputAreaHovered
            && mouseInsideCanvas
            && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            currentStroke_ = Stroke{};
            currentStroke_.color = brush_.color;
            currentStroke_.radiusPx = brush_.radiusPx;
            currentStroke_.addPoint(screenToCanvasPoint(mousePosition, canvasMin, scale));
            isDrawing_ = true;
        }

        if (canDraw && isDrawing_ && ImGui::IsMouseDown(ImGuiMouseButton_Left))
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
            if (canDraw && currentStroke_.hasDrawablePoints())
            {
                currentLayer->strokes.push_back(currentStroke_);
            }

            currentStroke_.points.clear();
            isDrawing_ = false;
        }

        ImGui::EndChild();
        ImGui::End();
    }
}