// このファイルの役割:
// ライトテーブルUIと、任意フレームを半透明でキャンバスへ重ねる描画を担当する。

#include "ui/App.h"

#include <algorithm>
#include <cmath>
#include <cstddef>

#include <imgui.h>

namespace perapera {
namespace {

const char* u8c(const char8_t* text)
{
    return reinterpret_cast<const char*>(text);
}

bool containsFrameIndex(const std::vector<int>& indices, int frameIndex)
{
    return std::find(indices.begin(), indices.end(), frameIndex) != indices.end();
}

void setFrameIndexEnabled(std::vector<int>& indices, int frameIndex, bool enabled)
{
    auto found = std::find(indices.begin(), indices.end(), frameIndex);
    if (enabled && found == indices.end()) {
        indices.push_back(frameIndex);
        std::sort(indices.begin(), indices.end());
    } else if (!enabled && found != indices.end()) {
        indices.erase(found);
    }
}

ImU32 lightTableColor(int colorMode, int frameIndex, int activeFrameIndex, float opacity)
{
    const int alpha = static_cast<int>(std::lround(std::clamp(opacity, 0.05f, 0.90f) * 255.0f));
    if (colorMode == 1) {
        return IM_COL32(255, 135, 70, alpha);
    }
    if (colorMode == 2) {
        return IM_COL32(70, 210, 255, alpha);
    }
    if (colorMode == 3) {
        return IM_COL32(210, 255, 100, alpha);
    }
    return frameIndex < activeFrameIndex ? IM_COL32(70, 170, 255, alpha) : IM_COL32(255, 105, 80, alpha);
}

void drawLightStroke(const Stroke& stroke,
                     ImU32 color,
                     const CanvasView& view,
                     ImVec2 areaMin,
                     ImVec2 areaSize,
                     ImDrawList* drawList)
{
    if (drawList == nullptr || stroke.points.empty()) {
        return;
    }
    const float zoom = std::clamp(view.zoom, 0.05f, 32.0f);
    const float baseWidth = std::clamp(stroke.radiusPx * zoom * 0.45f, 1.0f, 3.5f);
    if (stroke.points.size() == 1U) {
        const StrokePoint& point = stroke.points.front();
        const ImVec2 p = view.canvasToScreen(point.x, point.y, areaMin, areaSize);
        drawList->AddCircleFilled(p, baseWidth * 0.5f, color, 12);
        return;
    }
    for (std::size_t index = 1; index < stroke.points.size(); ++index) {
        const StrokePoint& a = stroke.points[index - 1U];
        const StrokePoint& b = stroke.points[index];
        const ImVec2 p0 = view.canvasToScreen(a.x, a.y, areaMin, areaSize);
        const ImVec2 p1 = view.canvasToScreen(b.x, b.y, areaMin, areaSize);
        const float pressure = std::max(0.15f, (a.pressure + b.pressure) * 0.5f);
        drawList->AddLine(p0, p1, color, std::clamp(baseWidth * pressure, 1.0f, 3.5f));
    }
}

void drawLightFrame(const Frame& frame,
                    ImU32 color,
                    const CanvasView& view,
                    ImVec2 areaMin,
                    ImVec2 areaSize,
                    ImDrawList* drawList)
{
    for (const Layer& layer : frame.layers) {
        if (!layer.visible || layer.opacity <= 0.0f || layer.type == LayerType::ShadowGuide) {
            continue;
        }
        for (const Stroke& stroke : layer.strokes) {
            drawLightStroke(stroke, color, view, areaMin, areaSize, drawList);
        }
    }
}

} // namespace

void App::drawLightTableControls()
{
    Cell* cell = activeCell();
    if (cell == nullptr) {
        return;
    }

    ImGui::PushID("LightTable_v29_controls");
    ImGui::Separator();
    ImGui::Checkbox(u8c(u8"ライトテーブル"), &lightTableEnabled_);
    ImGui::SameLine();
    if (ImGui::Button(u8c(u8"前後追加"))) {
        setFrameIndexEnabled(lightTableFrameIndices_, activeFrameIndex_ - 1, activeFrameIndex_ > 0);
        setFrameIndexEnabled(lightTableFrameIndices_, activeFrameIndex_ + 1,
                             activeFrameIndex_ + 1 < static_cast<int>(cell->frames.size()));
        lightTableEnabled_ = true;
    }
    ImGui::SameLine();
    if (ImGui::Button(u8c(u8"クリア"))) {
        lightTableFrameIndices_.clear();
    }

    ImGui::SetNextItemWidth(170.0f);
    ImGui::SliderFloat(u8c(u8"透過度"), &lightTableOpacity_, 0.05f, 0.85f, "%.2f");
    const char* colorItems[] = {u8c(u8"前=青 / 後=赤"), u8c(u8"橙"), u8c(u8"水色"), u8c(u8"黄緑")};
    ImGui::SetNextItemWidth(170.0f);
    ImGui::Combo(u8c(u8"表示色"), &lightTableColorMode_, colorItems, IM_ARRAYSIZE(colorItems));

    ImGui::BeginChild("LightTableFrameList_v29", ImVec2(0.0f, 58.0f), true,
                      ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_AlwaysHorizontalScrollbar);
    for (int i = 0; i < static_cast<int>(cell->frames.size()); ++i) {
        if (i == activeFrameIndex_) {
            ImGui::BeginDisabled();
        }
        bool selected = containsFrameIndex(lightTableFrameIndices_, i);
        const std::string label = "F" + std::to_string(i + 1);
        if (ImGui::Checkbox(label.c_str(), &selected)) {
            setFrameIndexEnabled(lightTableFrameIndices_, i, selected);
            lightTableEnabled_ = true;
        }
        if (i == activeFrameIndex_) {
            ImGui::EndDisabled();
        }
        if (i + 1 < static_cast<int>(cell->frames.size())) {
            ImGui::SameLine();
        }
    }
    ImGui::EndChild();
    ImGui::PopID();
}

void App::drawLightTableOverlay(ImVec2 areaMin, ImVec2 areaSize, ImDrawList* drawList)
{
    if (!lightTableEnabled_ || drawList == nullptr || lightTableFrameIndices_.empty()) {
        return;
    }
    const Cell* cell = activeCell();
    if (cell == nullptr || cell->frames.empty()) {
        return;
    }

    const ImVec2 areaMax(areaMin.x + areaSize.x, areaMin.y + areaSize.y);
    drawList->PushClipRect(areaMin, areaMax, true);
    for (int frameIndex : lightTableFrameIndices_) {
        if (frameIndex == activeFrameIndex_) {
            continue;
        }
        const Frame* frame = cell->frameOrNull(frameIndex);
        if (frame == nullptr) {
            continue;
        }
        const ImU32 color = lightTableColor(lightTableColorMode_, frameIndex, activeFrameIndex_, lightTableOpacity_);
        drawLightFrame(*frame, color, canvasView_, areaMin, areaSize, drawList);
    }
    drawList->PopClipRect();
}

} // namespace perapera
