#include "ui/AppDrawingModeOverlay.h"

#include <algorithm>
#include <cmath>
#include <cstddef>

#include "ui/Theme.h"

namespace perapera::app_drawing {
namespace {

ImU32 onionColor(bool isPrevious, float opacity)
{
    const int alpha = static_cast<int>(std::lround(std::clamp(opacity, 0.0f, 1.0f) * 255.0f));
    return isPrevious ? IM_COL32(35, 170, 255, alpha) : IM_COL32(255, 95, 70, alpha);
}

void drawOnionStrokeDirect(const Stroke& stroke,
                           bool isPrevious,
                           float opacity,
                           const CanvasView& view,
                           ImVec2 areaMin,
                           ImVec2 areaSize,
                           ImDrawList* drawList)
{
    if (drawList == nullptr || stroke.points.empty() || opacity <= 0.0f) {
        return;
    }
    const ImU32 color = onionColor(isPrevious, opacity * std::max(0.35f, stroke.color[3]));
    const float zoom = std::clamp(view.zoom, 0.05f, 32.0f);
    const float width = std::clamp(stroke.radiusPx * zoom * 0.55f, 1.2f, 4.0f);
    if (stroke.points.size() == 1U) {
        const StrokePoint& point = stroke.points.front();
        const ImVec2 p = view.canvasToScreen(point.x, point.y, areaMin, areaSize);
        drawList->AddCircleFilled(p, width * 0.5f, color, 12);
        return;
    }
    for (std::size_t index = 1; index < stroke.points.size(); ++index) {
        const StrokePoint& previous = stroke.points[index - 1U];
        const StrokePoint& current = stroke.points[index];
        const ImVec2 p0 = view.canvasToScreen(previous.x, previous.y, areaMin, areaSize);
        const ImVec2 p1 = view.canvasToScreen(current.x, current.y, areaMin, areaSize);
        const float pressure = std::max(0.1f, (previous.pressure + current.pressure) * 0.5f);
        drawList->AddLine(p0, p1, color, std::clamp(width * pressure, 1.2f, 4.0f));
    }
}

} // namespace

void drawOnionFrameDirect(const Frame& frame,
                          bool isPrevious,
                          float opacity,
                          const CanvasView& view,
                          ImVec2 areaMin,
                          ImVec2 areaSize,
                          ImDrawList* drawList)
{
    if (drawList == nullptr || opacity <= 0.0f) {
        return;
    }
    const ImVec2 areaMax(areaMin.x + areaSize.x, areaMin.y + areaSize.y);
    drawList->PushClipRect(areaMin, areaMax, true);
    for (const Layer& layer : frame.layers) {
        if (!layer.visible || layer.opacity <= 0.0f) {
            continue;
        }
        for (const Stroke& stroke : layer.strokes) {
            drawOnionStrokeDirect(stroke, isPrevious, opacity * layer.opacity, view, areaMin, areaSize, drawList);
        }
    }
    drawList->PopClipRect();
}

void drawLightweightEraserPreview(const Stroke& eraserStroke,
                                  const CanvasView& view,
                                  ImVec2 areaMin,
                                  ImVec2 areaSize,
                                  ImDrawList* drawList)
{
    if (drawList == nullptr || eraserStroke.points.empty()) {
        return;
    }

    const float zoom = std::clamp(view.zoom, 0.05f, 32.0f);
    const float radius = std::max(2.0f, eraserStroke.radiusPx * zoom);
    const float strokeWidth = radius * 2.0f;

    ImVec4 maskColor = ui::themeColors().canvasBackground;
    maskColor.w = 0.92f;
    const ImU32 mask = ImGui::ColorConvertFloat4ToU32(maskColor);
    const ImU32 guide = IM_COL32(255, 80, 70, 145);

    const ImVec2 areaMax(areaMin.x + areaSize.x, areaMin.y + areaSize.y);
    drawList->PushClipRect(areaMin, areaMax, true);

    const auto toScreen = [&](const StrokePoint& point) {
        return view.canvasToScreen(point.x, point.y, areaMin, areaSize);
    };

    if (eraserStroke.points.size() == 1U) {
        const ImVec2 p = toScreen(eraserStroke.points.front());
        drawList->AddCircleFilled(p, radius, mask, 28);
        drawList->AddCircle(p, radius, guide, 28, 1.5f);
        drawList->PopClipRect();
        return;
    }

    ImVec2 previous = toScreen(eraserStroke.points.front());
    drawList->AddCircleFilled(previous, radius, mask, 24);
    for (std::size_t i = 1; i < eraserStroke.points.size(); ++i) {
        const ImVec2 current = toScreen(eraserStroke.points[i]);
        drawList->AddLine(previous, current, mask, strokeWidth);
        drawList->AddCircleFilled(current, radius, mask, 24);
        previous = current;
    }

    previous = toScreen(eraserStroke.points.front());
    for (std::size_t i = 1; i < eraserStroke.points.size(); ++i) {
        const ImVec2 current = toScreen(eraserStroke.points[i]);
        drawList->AddLine(previous, current, guide, 1.5f);
        previous = current;
    }

    drawList->PopClipRect();
}

} // namespace perapera::app_drawing
