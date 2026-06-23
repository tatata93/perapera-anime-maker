// このファイルの役割:
// 作画モードのレイアウト、CanvasRenderer接続、作画入力の確定処理を実装する。

#include "ui/App.h"

#include <algorithm>
#include <cmath>
#include <cstddef>

#include <imgui.h>

#include "ui/Theme.h"
#include "ui/panels/BrushPanel.h"
#include "ui/panels/ExportPanel.h"
#include "ui/panels/FramePanel.h"
#include "ui/panels/LayerPanel.h"
#include "ui/panels/TimelinePanel.h"

namespace perapera {
namespace {

const char* u8c(const char8_t* text)
{
    return reinterpret_cast<const char*>(text);
}

float distanceSquared(const StrokePoint& point, const StrokePoint& other)
{
    const float dx = point.x - other.x;
    const float dy = point.y - other.y;
    return dx * dx + dy * dy;
}

bool strokeNearEraserPoint(const Stroke& stroke, const StrokePoint& eraserPoint, float radius)
{
    const StrokeBounds bounds = stroke.bounds();
    if (!bounds.valid) {
        return false;
    }

    if (eraserPoint.x < bounds.minX - radius || eraserPoint.x > bounds.maxX + radius ||
        eraserPoint.y < bounds.minY - radius || eraserPoint.y > bounds.maxY + radius) {
        return false;
    }

    const float hitRadius = radius + stroke.radiusPx;
    const float hitRadiusSq = hitRadius * hitRadius;
    for (const StrokePoint& point : stroke.points) {
        if (distanceSquared(point, eraserPoint) <= hitRadiusSq) {
            return true;
        }
    }
    return false;
}

} // namespace

void App::drawDrawingMode()
{
    clampSelection();

    ImGui::BeginChild("DrawingWorkspace", ImVec2(0.0f, -28.0f), true);

    const float sideWidth = 245.0f;
    const float rightWidth = 315.0f;
    const float timelineHeight = 120.0f;
    const float centerHeight = std::max(160.0f, ImGui::GetContentRegionAvail().y - timelineHeight - ImGui::GetStyle().ItemSpacing.y);

    ImGui::BeginChild("DrawingUpperArea", ImVec2(0.0f, centerHeight), false);

    ImGui::BeginChild("DrawingLeftSidebar", ImVec2(sideWidth, 0.0f), true);
    drawLeftSidebar();
    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("DrawingCanvasHost", ImVec2(-(rightWidth + ImGui::GetStyle().ItemSpacing.x), 0.0f), true);
    drawCanvasArea(rightWidth);
    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("DrawingRightSidebar", ImVec2(rightWidth, 0.0f), true,
        ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_HorizontalScrollbar);
    ImGui::PushID("DrawingRightSidebarContent_v4");
    drawRightSidebar();
    ImGui::PopID();
    ImGui::EndChild();

    ImGui::EndChild();

    drawTimelineArea();

    ImGui::EndChild();
}

void App::drawLeftSidebar()
{
    ui::drawBrushPanel(brushSettings_);

    ImGui::Separator();
    ImGui::TextUnformatted(u8c(u8"操作"));
    if (ImGui::Button("Undo", ImVec2(-1.0f, 0.0f))) {
        undo();
    }
    if (ImGui::Button("Redo", ImVec2(-1.0f, 0.0f))) {
        redo();
    }
    if (ImGui::Button(u8c(u8"全体表示"), ImVec2(-1.0f, 0.0f))) {
        canvasViewInitialized_ = false;
    }

    ImGui::TextWrapped(u8c(u8"左ドラッグ: 描く/消す\n中ボタンドラッグ: パン\nホイール: ズーム"));
}

void App::drawRightSidebar()
{
    Frame* frame = activeFrame();
    Cell* cell = activeCell();
    if (frame == nullptr || cell == nullptr) {
        ImGui::TextUnformatted(u8c(u8"アクティブなセルまたはフレームがありません。"));
        return;
    }

    const ui::LayerPanelAction layerAction = ui::drawLayerPanel(*frame, activeLayerIndex_);
    if (layerAction == ui::LayerPanelAction::AddLayer) {
        addLayer();
    } else if (layerAction == ui::LayerPanelAction::DeleteLayer) {
        deleteActiveLayer();
    } else if (layerAction == ui::LayerPanelAction::ClearLayer) {
        clearActiveLayer();
    }

    ImGui::Separator();
    const ui::FramePanelAction frameAction = ui::drawFramePanel(*cell, activeFrameIndex_);
    if (frameAction == ui::FramePanelAction::AddFrame) {
        addFrame();
    } else if (frameAction == ui::FramePanelAction::DuplicateFrame) {
        duplicateFrame();
    } else if (frameAction == ui::FramePanelAction::DeleteFrame) {
        deleteActiveFrame();
    }

    ImGui::Separator();
    const ui::ExportPanelAction exportAction = ui::drawExportPanel(exportState_, lastMessage_.c_str());
    if (exportAction == ui::ExportPanelAction::SaveProject) {
        saveProject();
    } else if (exportAction == ui::ExportPanelAction::LoadProject) {
        loadProject();
    } else if (exportAction == ui::ExportPanelAction::ExportActivePng) {
        exportActivePng();
    } else if (exportAction == ui::ExportPanelAction::ExportPngSequence) {
        exportPngSequence();
    } else if (exportAction == ui::ExportPanelAction::ExportMp4) {
        exportMp4();
    }
}

void App::drawTimelineArea()
{
    Cell* cell = activeCell();
    if (cell == nullptr) {
        return;
    }

    ImGui::BeginChild("DrawingTimeline", ImVec2(0.0f, 0.0f), true);
    const ui::TimelinePanelAction timelineAction =
        ui::drawTimelinePanel(*cell, activeFrameIndex_, onionPrevious_, onionNext_);
    if (timelineAction == ui::TimelinePanelAction::AddFrame) {
        addFrame();
    } else if (timelineAction == ui::TimelinePanelAction::DuplicateFrame) {
        duplicateFrame();
    } else if (timelineAction == ui::TimelinePanelAction::DeleteFrame) {
        deleteActiveFrame();
    }
    ImGui::EndChild();
}

void App::drawCanvasArea(float rightWidth)
{
    (void)rightWidth;
    Frame* frame = activeFrame();
    if (frame == nullptr) {
        ImGui::TextUnformatted(u8c(u8"フレームがありません。"));
        return;
    }

    canvasRenderer_.setRenderer(renderer_);
    canvasRenderer_.setCanvasSize(project_.canvas.width, project_.canvas.height);

    const ImVec2 areaMin = ImGui::GetCursorScreenPos();
    const ImVec2 areaSize = ImGui::GetContentRegionAvail();
    fitCanvasToArea(areaSize);

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    const ImU32 background = ImGui::ColorConvertFloat4ToU32(ui::themeColors().canvasBackground);
    drawList->AddRectFilled(areaMin, ImVec2(areaMin.x + areaSize.x, areaMin.y + areaSize.y), background);

    if (onionPrevious_ && activeFrameIndex_ > 0) {
        const Cell* cell = activeCell();
        if (cell != nullptr) {
            const Frame* previous = cell->frameOrNull(activeFrameIndex_ - 1);
            if (previous != nullptr) {
                canvasRenderer_.drawOnionSkin(*previous, activeFrameIndex_ - 1, true, 0.35f, canvasView_, areaMin, areaSize, drawList);
            }
        }
    }

    if (onionNext_) {
        const Cell* cell = activeCell();
        if (cell != nullptr) {
            const Frame* next = cell->frameOrNull(activeFrameIndex_ + 1);
            if (next != nullptr) {
                canvasRenderer_.drawOnionSkin(*next, activeFrameIndex_ + 1, false, 0.35f, canvasView_, areaMin, areaSize, drawList);
            }
        }
    }

    const Stroke* preview = isDrawingStroke_ ? &currentStroke_ : nullptr;
    canvasRenderer_.draw(*frame, activeLayerIndex_, preview, brushSettings_.opacity, canvasView_, areaMin, areaSize, drawList);

    handleCanvasInput(areaMin, areaSize);

    ImGui::Dummy(areaSize);
}

void App::fitCanvasToArea(ImVec2 areaSize)
{
    if (canvasViewInitialized_ || areaSize.x <= 1.0f || areaSize.y <= 1.0f) {
        return;
    }

    const float zoomX = areaSize.x / static_cast<float>(std::max(1, project_.canvas.width));
    const float zoomY = areaSize.y / static_cast<float>(std::max(1, project_.canvas.height));
    canvasView_.zoom = std::clamp(std::min(zoomX, zoomY) * 0.9f, 0.05f, 4.0f);
    canvasView_.panX = (areaSize.x - static_cast<float>(project_.canvas.width) * canvasView_.zoom) * 0.5f;
    canvasView_.panY = (areaSize.y - static_cast<float>(project_.canvas.height) * canvasView_.zoom) * 0.5f;
    canvasViewInitialized_ = true;
}

void App::finishStroke()
{
    if (!isDrawingStroke_) {
        return;
    }

    if (!currentStroke_.points.empty()) {
        pushUndoSnapshot();
        Layer* layer = activeLayer();
        if (layer != nullptr) {
            if (brushSettings_.tool == ui::ToolKind::Brush) {
                layer->strokes.push_back(currentStroke_);
                canvasRenderer_.bakeStroke(activeLayerIndex_, currentStroke_, 1.0f);
                lastMessage_ = "stroke baked";
            } else {
                removeIntersectingStrokes(currentStroke_);
                canvasRenderer_.markDirty(activeLayerIndex_);
                lastMessage_ = "eraser applied";
            }
        }
    }

    currentStroke_ = Stroke{};
    isDrawingStroke_ = false;
}

void App::cancelStroke()
{
    currentStroke_ = Stroke{};
    isDrawingStroke_ = false;
}

void App::removeIntersectingStrokes(const Stroke& eraserStroke)
{
    Layer* layer = activeLayer();
    if (layer == nullptr) {
        return;
    }

    const float radius = std::max(1.0f, eraserStroke.radiusPx);
    auto shouldRemove = [&](const Stroke& stroke) {
        for (const StrokePoint& eraserPoint : eraserStroke.points) {
            if (strokeNearEraserPoint(stroke, eraserPoint, radius)) {
                return true;
            }
        }
        return false;
    };

    layer->strokes.erase(std::remove_if(layer->strokes.begin(), layer->strokes.end(), shouldRemove), layer->strokes.end());
}

} // namespace perapera
