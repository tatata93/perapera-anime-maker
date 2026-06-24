// このファイルの役割:
// 作画モードのレイアウト、CanvasRenderer接続、作画入力の確定処理を実装する。

#include "ui/App.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iterator>
#include <utility>
#include <vector>

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

float distanceSquared(const StrokePoint& a, const StrokePoint& b)
{
    const float dx = a.x - b.x;
    const float dy = a.y - b.y;
    return dx * dx + dy * dy;
}

float distancePointToSegmentSquared(const StrokePoint& point,
                                    const StrokePoint& segmentA,
                                    const StrokePoint& segmentB)
{
    const float vx = segmentB.x - segmentA.x;
    const float vy = segmentB.y - segmentA.y;
    const float wx = point.x - segmentA.x;
    const float wy = point.y - segmentA.y;
    const float lengthSq = vx * vx + vy * vy;
    if (lengthSq <= 0.0001f) {
        return distanceSquared(point, segmentA);
    }

    const float t = std::clamp((wx * vx + wy * vy) / lengthSq, 0.0f, 1.0f);
    const StrokePoint closest{segmentA.x + vx * t, segmentA.y + vy * t, 1.0f};
    return distanceSquared(point, closest);
}

std::vector<StrokePoint> resampleStrokeForEraser(const Stroke& stroke, float spacing)
{
    std::vector<StrokePoint> result;
    if (stroke.points.empty()) {
        return result;
    }
    if (stroke.points.size() == 1U) {
        result.push_back(stroke.points.front());
        return result;
    }

    result.push_back(stroke.points.front());
    const float safeSpacing = std::max(0.75f, spacing);
    for (std::size_t index = 1; index < stroke.points.size(); ++index) {
        const StrokePoint& previous = stroke.points[index - 1U];
        const StrokePoint& current = stroke.points[index];
        const float dx = current.x - previous.x;
        const float dy = current.y - previous.y;
        const float length = std::sqrt(dx * dx + dy * dy);
        const int steps = std::max(1, static_cast<int>(std::ceil(length / safeSpacing)));
        for (int step = 1; step <= steps; ++step) {
            const float t = static_cast<float>(step) / static_cast<float>(steps);
            result.push_back(StrokePoint{previous.x + dx * t,
                                         previous.y + dy * t,
                                         previous.pressure + (current.pressure - previous.pressure) * t});
        }
    }
    return result;
}

bool pointHitByEraser(const StrokePoint& point,
                      float strokeRadius,
                      const Stroke& eraserStroke,
                      float eraserRadius)
{
    const float hitRadius = std::max(1.0f, eraserRadius + strokeRadius);
    const float hitRadiusSq = hitRadius * hitRadius;
    if (eraserStroke.points.empty()) {
        return false;
    }
    if (eraserStroke.points.size() == 1U) {
        return distanceSquared(point, eraserStroke.points.front()) <= hitRadiusSq;
    }

    for (std::size_t index = 1; index < eraserStroke.points.size(); ++index) {
        if (distancePointToSegmentSquared(point,
                                          eraserStroke.points[index - 1U],
                                          eraserStroke.points[index]) <= hitRadiusSq) {
            return true;
        }
    }
    return false;
}

void appendStrokePart(std::vector<Stroke>& output,
                      const Stroke& source,
                      std::vector<StrokePoint>& points)
{
    if (points.empty()) {
        return;
    }

    Stroke part = source;
    part.points = points;
    output.push_back(std::move(part));
    points.clear();
}

std::vector<Stroke> splitStrokeByEraser(const Stroke& stroke,
                                        const Stroke& eraserStroke,
                                        float eraserRadius,
                                        bool& changed)
{
    std::vector<Stroke> result;
    const float spacing = std::max(0.75f, std::min(stroke.radiusPx, eraserRadius) * 0.45f);
    const std::vector<StrokePoint> sampledPoints = resampleStrokeForEraser(stroke, spacing);
    std::vector<StrokePoint> currentPart;
    bool anyErasedPoint = false;

    for (const StrokePoint& point : sampledPoints) {
        if (pointHitByEraser(point, stroke.radiusPx, eraserStroke, eraserRadius)) {
            anyErasedPoint = true;
            appendStrokePart(result, stroke, currentPart);
        } else {
            currentPart.push_back(point);
        }
    }

    appendStrokePart(result, stroke, currentPart);
    if (!anyErasedPoint) {
        result.clear();
        result.push_back(stroke);
        return result;
    }

    changed = true;
    return result;
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

    ImGui::BeginChild("DrawingRightSidebar_v5", ImVec2(rightWidth, 0.0f), true,
        ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_HorizontalScrollbar);
    ImGui::PushID("DrawingRightSidebar_v5Content_v4");
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

    ImGui::TextDisabled("Step 1-4 stability pass v13");

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
    } else if (exportAction == ui::ExportPanelAction::VerifyProjectRoundTrip) {
        saveLoadRoundTripCheck();
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

    ImGui::BeginChild("DrawingTimeline_v5", ImVec2(0.0f, 0.0f), true);
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
                canvasRenderer_.markAllDirty();
                lastMessage_ = "stroke committed to active frame";
            } else {
                removeIntersectingStrokes(currentStroke_);
                canvasRenderer_.markAllDirty();
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
    if (layer == nullptr || eraserStroke.points.empty()) {
        return;
    }

    const float radius = std::max(1.0f, eraserStroke.radiusPx);
    std::vector<Stroke> rewrittenStrokes;
    rewrittenStrokes.reserve(layer->strokes.size());

    bool changed = false;
    for (const Stroke& stroke : layer->strokes) {
        std::vector<Stroke> parts = splitStrokeByEraser(stroke, eraserStroke, radius, changed);
        rewrittenStrokes.insert(rewrittenStrokes.end(),
                                std::make_move_iterator(parts.begin()),
                                std::make_move_iterator(parts.end()));
    }

    if (changed) {
        layer->strokes = std::move(rewrittenStrokes);
        lastMessage_ = "eraser applied: partial stroke split";
    } else {
        lastMessage_ = "eraser applied: no hit";
    }
}

} // namespace perapera
