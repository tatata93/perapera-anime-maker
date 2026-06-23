// このファイルの役割:
// 作画キャンバス内のマウス入力を処理する。
// 入力中ストロークだけDrawListでプレビューし、確定時にProjectとCanvasBitmapへ反映する。

#include "ui/App.h"

#include <algorithm>
#include <cmath>

#include <imgui.h>

namespace perapera {
namespace {

float distanceSquared(const StrokePoint& a, const StrokePoint& b)
{
    const float dx = a.x - b.x;
    const float dy = a.y - b.y;
    return dx * dx + dy * dy;
}

bool isPointInside(ImVec2 point, ImVec2 min, ImVec2 size)
{
    return point.x >= min.x && point.y >= min.y && point.x <= min.x + size.x && point.y <= min.y + size.y;
}

} // namespace

void App::handleCanvasInput(ImVec2 areaMin, ImVec2 areaSize)
{
    ImGuiIO& io = ImGui::GetIO();
    const ImVec2 mouse = io.MousePos;
    const bool hovered = ImGui::IsWindowHovered() && isPointInside(mouse, areaMin, areaSize);

    if (hovered && io.MouseWheel != 0.0f) {
        const ImVec2 before = canvasView_.screenToCanvas(mouse.x, mouse.y, areaMin, areaSize);
        const float zoomFactor = io.MouseWheel > 0.0f ? 1.15f : 1.0f / 1.15f;
        canvasView_.zoom = std::clamp(canvasView_.zoom * zoomFactor, 0.05f, 32.0f);
        const ImVec2 after = canvasView_.screenToCanvas(mouse.x, mouse.y, areaMin, areaSize);
        canvasView_.panX += (after.x - before.x) * canvasView_.zoom;
        canvasView_.panY += (after.y - before.y) * canvasView_.zoom;
    }

    if (hovered && ImGui::IsMouseDragging(ImGuiMouseButton_Middle, 0.0f)) {
        canvasView_.panX += io.MouseDelta.x;
        canvasView_.panY += io.MouseDelta.y;
    }

    if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        beginStroke(mouse, areaMin, areaSize);
    }

    if (isDrawingStroke_ && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        updateStroke(mouse, areaMin, areaSize);
    }

    if (isDrawingStroke_ && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
        finishStroke();
    }

    if (isDrawingStroke_ && ImGui::IsKeyPressed(ImGuiKey_Escape)) {
        cancelStroke();
    }
}

void App::beginStroke(ImVec2 mouseScreen, ImVec2 areaMin, ImVec2 areaSize)
{
    Layer* layer = activeLayer();
    if (layer == nullptr) {
        return;
    }

    currentStroke_ = Stroke{};
    currentStroke_.color = brushSettings_.color;
    currentStroke_.radiusPx = brushSettings_.radiusPx;

    if (brushSettings_.tool == ui::ToolKind::Eraser) {
        currentStroke_.color = {1.0f, 0.2f, 0.2f, 0.75f};
    }

    isDrawingStroke_ = true;
    updateStroke(mouseScreen, areaMin, areaSize);
    (void)areaSize;
}

void App::updateStroke(ImVec2 mouseScreen, ImVec2 areaMin, ImVec2 areaSize)
{
    if (!isDrawingStroke_) {
        return;
    }

    const ImVec2 canvas = canvasView_.screenToCanvas(mouseScreen.x, mouseScreen.y, areaMin, areaSize);
    StrokePoint point;
    point.x = std::clamp(canvas.x, 0.0f, static_cast<float>(std::max(1, project_.canvas.width)));
    point.y = std::clamp(canvas.y, 0.0f, static_cast<float>(std::max(1, project_.canvas.height)));
    point.pressure = 1.0f;

    if (currentStroke_.points.empty()) {
        currentStroke_.points.push_back(point);
        return;
    }

    const float minDistance = std::max(1.0f, brushSettings_.radiusPx * 0.25f);
    if (distanceSquared(currentStroke_.points.back(), point) >= minDistance * minDistance) {
        currentStroke_.points.push_back(point);
    }
}

} // namespace perapera
