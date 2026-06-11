// src/ui/DrawingCanvasPanel.cpp
//
// DrawingCanvasPanelの実装です。
//
// Phase 3Dでは、フレーム管理に対応します。
// frames_ がAnimationFrameの配列を持ち、
// 各AnimationFrameがDrawingLayerの配列を持ちます。

#include "ui/DrawingCanvasPanel.h"

#include "drawing/WorkCanvas.h"
#include "project/RenderFormat.h"

#include "imgui.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iomanip>
#include <sstream>
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

        std::string makeDefaultFrameName(int frameNumber)
        {
            return "Frame " + std::to_string(frameNumber);
        }

        std::string makeDefaultLayerName(int layerNumber)
        {
            return "Layer " + std::to_string(layerNumber);
        }

        DrawingLayer makeDefaultLayer(const std::string& name)
        {
            DrawingLayer layer;
            layer.name = name;
            layer.visible = true;
            layer.opacity = 1.0f;
            return layer;
        }

        AnimationFrame makeDefaultFrame(int frameNumber)
        {
            AnimationFrame frame;
            frame.name = makeDefaultFrameName(frameNumber);
            frame.durationFrames = 1;
            frame.layers.push_back(makeDefaultLayer("Layer 1"));
            return frame;
        }

        float distanceSquared(CanvasPoint a, CanvasPoint b)
        {
            const float dx = a.x - b.x;
            const float dy = a.y - b.y;
            return dx * dx + dy * dy;
        }

        ImU32 toImGuiColor(ColorRgba color, float layerOpacity)
        {
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

    DrawingCanvasPanel::DrawingCanvasPanel()
    {
        frames_.push_back(makeDefaultFrame(1));

        activeFrameIndex_ = 0;
        activeLayerIndex_ = 0;
        nextFrameNumber_ = 2;
        nextLayerNumber_ = 2;
    }

    void DrawingCanvasPanel::clampActiveFrameIndex()
    {
        if (frames_.empty())
        {
            activeFrameIndex_ = -1;
            return;
        }

        activeFrameIndex_ = std::clamp(
            activeFrameIndex_,
            0,
            static_cast<int>(frames_.size()) - 1
        );
    }

    void DrawingCanvasPanel::clampActiveLayerIndex()
    {
        AnimationFrame* frame = activeFrame();

        if (frame == nullptr || frame->layers.empty())
        {
            activeLayerIndex_ = -1;
            return;
        }

        activeLayerIndex_ = std::clamp(
            activeLayerIndex_,
            0,
            static_cast<int>(frame->layers.size()) - 1
        );
    }

    AnimationFrame* DrawingCanvasPanel::activeFrame()
    {
        if (frames_.empty())
        {
            return nullptr;
        }

        clampActiveFrameIndex();

        if (activeFrameIndex_ < 0 || activeFrameIndex_ >= static_cast<int>(frames_.size()))
        {
            return nullptr;
        }

        return &frames_[static_cast<std::size_t>(activeFrameIndex_)];
    }

    const AnimationFrame* DrawingCanvasPanel::activeFrame() const
    {
        if (activeFrameIndex_ < 0 || activeFrameIndex_ >= static_cast<int>(frames_.size()))
        {
            return nullptr;
        }

        return &frames_[static_cast<std::size_t>(activeFrameIndex_)];
    }

    DrawingLayer* DrawingCanvasPanel::activeLayer()
    {
        AnimationFrame* frame = activeFrame();

        if (frame == nullptr || frame->layers.empty())
        {
            return nullptr;
        }

        clampActiveLayerIndex();

        if (activeLayerIndex_ < 0 || activeLayerIndex_ >= static_cast<int>(frame->layers.size()))
        {
            return nullptr;
        }

        return &frame->layers[static_cast<std::size_t>(activeLayerIndex_)];
    }

    const DrawingLayer* DrawingCanvasPanel::activeLayer() const
    {
        const AnimationFrame* frame = activeFrame();

        if (frame == nullptr || frame->layers.empty())
        {
            return nullptr;
        }

        if (activeLayerIndex_ < 0 || activeLayerIndex_ >= static_cast<int>(frame->layers.size()))
        {
            return nullptr;
        }

        return &frame->layers[static_cast<std::size_t>(activeLayerIndex_)];
    }

    void DrawingCanvasPanel::addFrame()
    {
        frames_.push_back(makeDefaultFrame(nextFrameNumber_));
        ++nextFrameNumber_;

        activeFrameIndex_ = static_cast<int>(frames_.size()) - 1;
        activeLayerIndex_ = 0;

        currentStroke_.points.clear();
        isDrawing_ = false;
    }

    void DrawingCanvasPanel::duplicateActiveFrame()
    {
        const AnimationFrame* currentFrame = activeFrame();

        if (currentFrame == nullptr)
        {
            return;
        }

        AnimationFrame copiedFrame = *currentFrame;
        copiedFrame.name = makeDefaultFrameName(nextFrameNumber_);

        ++nextFrameNumber_;

        frames_.insert(
            frames_.begin() + activeFrameIndex_ + 1,
            copiedFrame
        );

        ++activeFrameIndex_;
        activeLayerIndex_ = 0;

        currentStroke_.points.clear();
        isDrawing_ = false;
    }

    void DrawingCanvasPanel::deleteActiveFrame()
    {
        if (frames_.size() <= 1)
        {
            return;
        }

        clampActiveFrameIndex();

        frames_.erase(frames_.begin() + activeFrameIndex_);
        clampActiveFrameIndex();

        activeLayerIndex_ = 0;

        currentStroke_.points.clear();
        isDrawing_ = false;
    }

    void DrawingCanvasPanel::moveToPreviousFrame()
    {
        if (activeFrameIndex_ > 0)
        {
            --activeFrameIndex_;
            activeLayerIndex_ = 0;
            currentStroke_.points.clear();
            isDrawing_ = false;
        }
    }

    void DrawingCanvasPanel::moveToNextFrame()
    {
        if (activeFrameIndex_ < static_cast<int>(frames_.size()) - 1)
        {
            ++activeFrameIndex_;
            activeLayerIndex_ = 0;
            currentStroke_.points.clear();
            isDrawing_ = false;
        }
    }

    void DrawingCanvasPanel::addLayer()
    {
        AnimationFrame* frame = activeFrame();

        if (frame == nullptr)
        {
            return;
        }

        DrawingLayer layer;
        layer.name = makeDefaultLayerName(nextLayerNumber_);
        layer.visible = true;
        layer.opacity = 1.0f;

        ++nextLayerNumber_;

        frame->layers.push_back(layer);
        activeLayerIndex_ = static_cast<int>(frame->layers.size()) - 1;
    }

    void DrawingCanvasPanel::deleteActiveLayer()
    {
        AnimationFrame* frame = activeFrame();

        if (frame == nullptr || frame->layers.size() <= 1)
        {
            return;
        }

        clampActiveLayerIndex();

        frame->layers.erase(frame->layers.begin() + activeLayerIndex_);
        clampActiveLayerIndex();
    }

    void DrawingCanvasPanel::moveActiveLayerUp()
    {
        AnimationFrame* frame = activeFrame();

        if (frame == nullptr)
        {
            return;
        }

        clampActiveLayerIndex();

        if (activeLayerIndex_ < 0 || activeLayerIndex_ >= static_cast<int>(frame->layers.size()) - 1)
        {
            return;
        }

        std::swap(
            frame->layers[static_cast<std::size_t>(activeLayerIndex_)],
            frame->layers[static_cast<std::size_t>(activeLayerIndex_ + 1)]
        );

        ++activeLayerIndex_;
    }

    void DrawingCanvasPanel::moveActiveLayerDown()
    {
        AnimationFrame* frame = activeFrame();

        if (frame == nullptr)
        {
            return;
        }

        clampActiveLayerIndex();

        if (activeLayerIndex_ <= 0)
        {
            return;
        }

        std::swap(
            frame->layers[static_cast<std::size_t>(activeLayerIndex_)],
            frame->layers[static_cast<std::size_t>(activeLayerIndex_ - 1)]
        );

        --activeLayerIndex_;
    }

    void DrawingCanvasPanel::clearCurrentFrameLayers()
    {
        AnimationFrame* frame = activeFrame();

        if (frame == nullptr)
        {
            return;
        }

        frame->clearAllLayers();
        currentStroke_.points.clear();
        isDrawing_ = false;
    }

    void DrawingCanvasPanel::exportCurrentRenderFramePng(
        const WorkCanvas& workCanvas,
        const RenderFormat& renderFormat
    )
    {
        const AnimationFrame* frame = activeFrame();

        if (frame == nullptr)
        {
            lastPngExportSucceeded_ = false;
            lastPngExportMessage_ = "PNG保存失敗: 現在フレームがありません。";
            return;
        }

        std::ostringstream fileNameStream;

        fileNameStream
            << "frame_"
            << std::setw(4)
            << std::setfill('0')
            << nextPngExportNumber_
            << ".png";

        const std::filesystem::path outputPath =
            std::filesystem::path("exports") / fileNameStream.str();

        PngExportOptions options;
        options.transparentBackground = pngTransparentBackground_;

        std::string errorMessage;

        const bool succeeded = PngExporter::exportCenteredRenderFrame(
            outputPath,
            workCanvas,
            renderFormat,
            frame->layers,
            options,
            errorMessage
        );

        if (succeeded)
        {
            lastPngExportSucceeded_ = true;
            lastPngExportMessage_ = "PNG保存成功: " + outputPath.string();
            ++nextPngExportNumber_;
        }
        else
        {
            lastPngExportSucceeded_ = false;
            lastPngExportMessage_ = "PNG保存失敗: " + errorMessage;
        }
    }

    void DrawingCanvasPanel::drawFramePanel()
    {
        ImGui::Begin("フレーム");

        ImGui::Text("現在のフレームを選びます。");
        ImGui::Text("Phase 3Dでは再生はまだ行いません。");

        ImGui::Separator();

        if (ImGui::Button("前のフレーム"))
        {
            moveToPreviousFrame();
        }

        ImGui::SameLine();

        if (ImGui::Button("次のフレーム"))
        {
            moveToNextFrame();
        }

        if (ImGui::Button("空フレーム追加"))
        {
            addFrame();
        }

        ImGui::SameLine();

        if (ImGui::Button("現在フレームを複製"))
        {
            duplicateActiveFrame();
        }

        if (ImGui::Button("現在フレームを削除"))
        {
            deleteActiveFrame();
        }

        ImGui::Separator();

        for (int index = 0; index < static_cast<int>(frames_.size()); ++index)
        {
            AnimationFrame& frame = frames_[static_cast<std::size_t>(index)];

            ImGui::PushID(index);

            const bool isActive = (index == activeFrameIndex_);

            if (ImGui::RadioButton(frame.name.c_str(), isActive))
            {
                activeFrameIndex_ = index;
                activeLayerIndex_ = 0;
                currentStroke_.points.clear();
                isDrawing_ = false;
            }

            ImGui::Text("ストローク数: %d", frame.strokeCount());

            ImGui::SetNextItemWidth(120.0f);
            ImGui::SliderInt("保持コマ数", &frame.durationFrames, 1, 24);

            ImGui::Separator();

            ImGui::PopID();
        }

        const AnimationFrame* frame = activeFrame();

        if (frame != nullptr)
        {
            ImGui::Text("現在フレーム: %s", frame->name.c_str());
        }

        ImGui::End();
    }

    void DrawingCanvasPanel::drawLayerPanel()
    {
        ImGui::Begin("レイヤー");

        AnimationFrame* frame = activeFrame();

        if (frame == nullptr)
        {
            ImGui::Text("現在フレームがありません。");
            ImGui::End();
            return;
        }

        ImGui::Text("現在フレーム内のレイヤーを管理します。");
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

        for (int index = static_cast<int>(frame->layers.size()) - 1; index >= 0; --index)
        {
            DrawingLayer& layer = frame->layers[static_cast<std::size_t>(index)];

            ImGui::PushID(index);

            const bool isActive = (index == activeLayerIndex_);

            ImGui::Text("%s", layer.name.c_str());

            if (ImGui::RadioButton("このレイヤーに描く", isActive))
            {
                activeLayerIndex_ = index;
            }

            ImGui::Checkbox("表示する", &layer.visible);

            ImGui::SetNextItemWidth(180.0f);
            ImGui::SliderFloat("不透明度", &layer.opacity, 0.0f, 1.0f);

            ImGui::Text("ストローク数: %d", static_cast<int>(layer.strokes.size()));

            if (isActive)
            {
                ImGui::TextColored(
                    ImVec4(0.45f, 0.80f, 1.0f, 1.0f),
                    "現在の描き込み先"
                );
            }

            ImGui::Separator();

            ImGui::PopID();
        }

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

        if (ImGui::Button("現在フレームの全レイヤー消去"))
        {
            clearCurrentFrameLayers();
        }

        ImGui::End();
    }

    void DrawingCanvasPanel::draw(WorkCanvas& workCanvas, const RenderFormat& renderFormat)
    {
        drawFramePanel();
        drawLayerPanel();

        ImGui::Begin("簡易作画キャンバス");

        const AnimationFrame* currentFrame = activeFrame();

        if (currentFrame != nullptr)
        {
            ImGui::Text("現在フレーム: %s", currentFrame->name.c_str());
        }

        ImGui::Text("左ドラッグで、選択中フレームの選択中レイヤーに線を描けます。");

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

        ImGui::Separator();

        ImGui::Text("PNG保存");

        ImGui::Checkbox("透明背景で保存", &pngTransparentBackground_);

        if (ImGui::Button("現在の撮影フレームをPNG保存"))
        {
            exportCurrentRenderFramePng(workCanvas, renderFormat);
        }

        if (!lastPngExportMessage_.empty())
        {
            if (lastPngExportSucceeded_)
            {
                ImGui::TextColored(
                    ImVec4(0.45f, 1.0f, 0.55f, 1.0f),
                    "%s",
                    lastPngExportMessage_.c_str()
                );
            }
            else
            {
                ImGui::TextColored(
                    ImVec4(1.0f, 0.45f, 0.25f, 1.0f),
                    "%s",
                    lastPngExportMessage_.c_str()
                );
            }
        }

        ImGui::Separator();

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

        drawList->PushClipRect(canvasMin, canvasMax, true);

        if (currentFrame != nullptr)
        {
            for (const DrawingLayer& layer : currentFrame->layers)
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
        }

        if (isDrawing_ && currentLayer != nullptr && currentLayer->visible)
        {
            drawStroke(drawList, currentStroke_, canvasMin, scale, currentLayer->opacity);
        }

        drawList->PopClipRect();

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