// このファイルの役割:
// 作画モードのレイアウト、CanvasRenderer接続、作画入力の確定処理を実装する。
#include "ui/App.h"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iterator>
#include <limits>
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
float distancePointToStrokePathSquared(const StrokePoint& point, const Stroke& stroke)
{
    if (stroke.points.empty()) {
        return std::numeric_limits<float>::infinity();
    }
    if (stroke.points.size() == 1U) {
        return distanceSquared(point, stroke.points.front());
    }
    float best = std::numeric_limits<float>::infinity();
    for (std::size_t i = 1; i < stroke.points.size(); ++i) {
        best = std::min(best,
                        distancePointToSegmentSquared(point,
                                                       stroke.points[i - 1U],
                                                       stroke.points[i]));
    }
    return best;
}
float strokeApproxLength(const Stroke& stroke)
{
    if (stroke.points.size() < 2U) {
        return 0.0f;
    }
    float length = 0.0f;
    for (std::size_t i = 1; i < stroke.points.size(); ++i) {
        const float dx = stroke.points[i].x - stroke.points[i - 1U].x;
        const float dy = stroke.points[i].y - stroke.points[i - 1U].y;
        length += std::sqrt(dx * dx + dy * dy);
    }
    return length;
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
    const float safeSpacing = std::max(0.5f, spacing);
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
float eraserHitRadius(float strokeRadius, float eraserRadius)
{
    // ベクターストロークは中心線しか持たないので、判定には線幅の半分も含める。
    // ただし線幅全体を足すと広すぎるため、消しゴム半径 + 描画線半径の半分にする。
    return std::max(1.0f, eraserRadius + strokeRadius * 0.5f);
}
void appendStrokePart(std::vector<Stroke>& output, const Stroke& source, std::vector<StrokePoint>& points)
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
    if (stroke.points.empty() || eraserStroke.points.empty()) {
        result.push_back(stroke);
        return result;
    }
    const float spacing = std::max(0.75f, std::min(stroke.radiusPx, eraserRadius) * 0.35f);
    const std::vector<StrokePoint> sampledPoints = resampleStrokeForEraser(stroke, spacing);
    if (sampledPoints.empty()) {
        result.push_back(stroke);
        return result;
    }
    const float hitRadius = eraserHitRadius(stroke.radiusPx, eraserRadius);
    const float hitRadiusSq = hitRadius * hitRadius;
    std::vector<bool> hitFlags(sampledPoints.size(), false);
    int hitCount = 0;
    float bestDistanceSq = std::numeric_limits<float>::infinity();
    std::size_t closestIndex = 0U;
    for (std::size_t i = 0; i < sampledPoints.size(); ++i) {
        const float distanceSq = distancePointToStrokePathSquared(sampledPoints[i], eraserStroke);
        if (distanceSq < bestDistanceSq) {
            bestDistanceSq = distanceSq;
            closestIndex = i;
        }
        if (distanceSq <= hitRadiusSq) {
            hitFlags[i] = true;
            ++hitCount;
        }
    }
    if (hitCount == 0) {
        result.push_back(stroke);
        return result;
    }
    // 長い線で全サンプルがヒットした場合でも、ストローク全体削除にせず、
    // 消しゴム軌跡に最も近い周辺だけを抜く。短い線は全消ししても自然なので許可する。
    const float targetLength = strokeApproxLength(stroke);
    if (hitCount == static_cast<int>(sampledPoints.size()) &&
        sampledPoints.size() >= 3U &&
        targetLength > hitRadius * 4.0f) {
        std::fill(hitFlags.begin(), hitFlags.end(), false);
        const int gapSamples = std::max(1, static_cast<int>(std::ceil(hitRadius / spacing)));
        const int begin = std::max(0, static_cast<int>(closestIndex) - gapSamples);
        const int end = std::min(static_cast<int>(sampledPoints.size()) - 1,
                                 static_cast<int>(closestIndex) + gapSamples);
        for (int i = begin; i <= end; ++i) {
            hitFlags[static_cast<std::size_t>(i)] = true;
        }
    }
    std::vector<StrokePoint> currentPart;
    bool anyErasedPoint = false;
    for (std::size_t i = 0; i < sampledPoints.size(); ++i) {
        if (hitFlags[i]) {
            anyErasedPoint = true;
            appendStrokePart(result, stroke, currentPart);
        } else {
            currentPart.push_back(sampledPoints[i]);
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
ImU32 onionColor(bool isPrevious, float alpha)
{
    const int alphaByte = static_cast<int>(std::lround(std::clamp(alpha, 0.0f, 1.0f) * 255.0f));
    if (isPrevious) {
        return IM_COL32(0, 150, 255, std::max(alphaByte, 170));
    }
    return IM_COL32(255, 80, 50, std::max(alphaByte, 170));
}
void drawOnionStrokeDirect(const Stroke& stroke,
                           bool isPrevious,
                           float opacity,
                           const CanvasView& view,
                           ImVec2 areaMin,
                           ImVec2 areaSize,
                           ImDrawList* drawList)
{
    if (drawList == nullptr || stroke.points.empty()) {
        return;
    }
    const float alpha = std::clamp(opacity * stroke.color[3], 0.0f, 1.0f);
    const ImU32 color = onionColor(isPrevious, alpha);
    const float radius = std::max(3.0f, stroke.radiusPx * std::clamp(view.zoom, 0.05f, 32.0f));
    if (stroke.points.size() == 1U) {
        const StrokePoint& point = stroke.points.front();
        const ImVec2 screenPoint = view.canvasToScreen(point.x, point.y, areaMin, areaSize);
        drawList->AddCircleFilled(screenPoint, radius, color, 16);
        return;
    }
    for (std::size_t index = 1; index < stroke.points.size(); ++index) {
        const StrokePoint& previous = stroke.points[index - 1U];
        const StrokePoint& current = stroke.points[index];
        const ImVec2 p0 = view.canvasToScreen(previous.x, previous.y, areaMin, areaSize);
        const ImVec2 p1 = view.canvasToScreen(current.x, current.y, areaMin, areaSize);
        const float pressure = std::max(0.1f, (previous.pressure + current.pressure) * 0.5f);
        drawList->AddLine(p0, p1, color, std::max(2.5f, radius * pressure * 2.2f));
    }
}
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
int countFrameStrokes(const Frame* frame)
{
    if (frame == nullptr) {
        return 0;
    }
    int count = 0;
    for (const Layer& layer : frame->layers) {
        if (!layer.visible || layer.opacity <= 0.0f) {
            continue;
        }
        count += static_cast<int>(layer.strokes.size());
    }
    return count;
}
} // namespace
void App::drawDrawingMode()
{
    clampSelection();
    ImGui::BeginChild("DrawingWorkspace", ImVec2(0.0f, -28.0f), true);
    const float sideWidth = 245.0f;
    const float rightWidth = 315.0f;
    const float timelineHeight = 120.0f;
    const float centerHeight = std::max(160.0f,
                                        ImGui::GetContentRegionAvail().y - timelineHeight - ImGui::GetStyle().ItemSpacing.y);
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
    ImGui::TextDisabled("Step 1-4 stability pass v19");
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
    const Stroke* preview = isDrawingStroke_ ? &currentStroke_ : nullptr;
    canvasRenderer_.draw(*frame, activeLayerIndex_, preview, brushSettings_.opacity, canvasView_, areaMin, areaSize, drawList);
    const Cell* onionCell = activeCell();
    const Frame* previous = (onionCell != nullptr && activeFrameIndex_ > 0)
        ? onionCell->frameOrNull(activeFrameIndex_ - 1)
        : nullptr;
    const Frame* next = (onionCell != nullptr) ? onionCell->frameOrNull(activeFrameIndex_ + 1) : nullptr;
    // v19: ここは確実に見える経路にする。
    // 通常Texture描画の後にWindowDrawListへ重ね、さらに同じ座標系で描く。
    if (onionPrevious_ && previous != nullptr) {
        drawOnionFrameDirect(*previous, true, 0.70f, canvasView_, areaMin, areaSize, drawList);
    }
    if (onionNext_ && next != nullptr) {
        drawOnionFrameDirect(*next, false, 0.70f, canvasView_, areaMin, areaSize, drawList);
    }
    // オニオンが本当に参照できているかを画面上で判別できる軽い診断表示。
    if (onionPrevious_ && previous != nullptr) {
        const int previousStrokes = countFrameStrokes(previous);
        if (previousStrokes > 0) {
            drawList->AddText(ImVec2(areaMin.x + 10.0f, areaMin.y + 10.0f),
                              IM_COL32(0, 150, 255, 230),
                              "prev onion ON");
        }
    }
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
    const std::size_t beforeCount = layer->strokes.size();
    for (const Stroke& stroke : layer->strokes) {
        std::vector<Stroke> parts = splitStrokeByEraser(stroke, eraserStroke, radius, changed);
        rewrittenStrokes.insert(rewrittenStrokes.end(),
                                std::make_move_iterator(parts.begin()),
                                std::make_move_iterator(parts.end()));
    }
    if (changed) {
        layer->strokes = std::move(rewrittenStrokes);
        lastMessage_ = "eraser applied: partial stroke split v19";
    } else {
        lastMessage_ = "eraser applied: no hit, radius=" + std::to_string(radius) +
                       ", strokes=" + std::to_string(beforeCount);
    }
}
} // namespace perapera
