// このファイルの役割:
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
#include "ui/panels/CellPanel.h"
#include "ui/panels/ColorPanel.h"
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
    return std::max(2.0f, eraserRadius + strokeRadius * 0.35f);
}
void keepLocalEraserGap(std::vector<bool>& hitFlags,
                        std::size_t closestIndex,
                        float spacing,
                        float eraserRadius,
                        float strokeRadius)
{
    if (hitFlags.empty()) {
        return;
    }
    std::fill(hitFlags.begin(), hitFlags.end(), false);
    const float localGapRadius = std::max(2.0f, eraserRadius * 1.15f + strokeRadius * 0.25f);
    const int gapSamples = std::max(1, static_cast<int>(std::ceil(localGapRadius / std::max(0.5f, spacing))));
    const int begin = std::max(0, static_cast<int>(closestIndex) - gapSamples);
    const int end = std::min(static_cast<int>(hitFlags.size()) - 1,
                             static_cast<int>(closestIndex) + gapSamples);
    for (int i = begin; i <= end; ++i) {
        hitFlags[static_cast<std::size_t>(i)] = true;
    }
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
    const float spacing = std::max(1.0f, std::min(stroke.radiusPx, eraserRadius) * 0.5f);
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
    const float targetLength = strokeApproxLength(stroke);
    const float eraserPathLength = strokeApproxLength(eraserStroke);
    const bool shortEraserGesture = eraserPathLength <= std::max(4.0f, eraserRadius * 1.5f);
    const bool mostlyHit = hitCount >= static_cast<int>(sampledPoints.size() * 0.90f);
    if (shortEraserGesture && sampledPoints.size() >= 3U &&
        targetLength > std::max(8.0f, eraserRadius * 2.5f) && mostlyHit) {
        keepLocalEraserGap(hitFlags, closestIndex, spacing, eraserRadius, stroke.radiusPx);
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
    if (anyErasedPoint && result.empty() && shortEraserGesture) {
        keepLocalEraserGap(hitFlags, closestIndex, spacing, eraserRadius, stroke.radiusPx);
        currentPart.clear();
        for (std::size_t i = 0; i < sampledPoints.size(); ++i) {
            if (hitFlags[i]) {
                appendStrokePart(result, stroke, currentPart);
            } else {
                currentPart.push_back(sampledPoints[i]);
            }
        }
        appendStrokePart(result, stroke, currentPart);
    }
    if (!anyErasedPoint || result.empty()) {
        result.clear();
        result.push_back(stroke);
        return result;
    }
    changed = true;
    return result;
}
ImU32 onionColor(bool isPrevious, float opacity)
{
    const int alpha = static_cast<int>(std::lround(std::clamp(opacity, 0.0f, 1.0f) * 255.0f));
    return isPrevious ? IM_COL32(35, 170, 255, alpha) : IM_COL32(255, 95, 70, alpha);
}
void drawOnionStrokeDirect(const Stroke& stroke, bool isPrevious, float opacity, const CanvasView& view, ImVec2 areaMin, ImVec2 areaSize, ImDrawList* drawList)
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
void drawOnionFrameDirect(const Frame& frame, bool isPrevious, float opacity, const CanvasView& view, ImVec2 areaMin, ImVec2 areaSize, ImDrawList* drawList)
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

    // 消しゴムプレビューは、確定後の全ストローク分割を毎フレーム再計算しない。
    // 代わりに現在なぞっている軌跡だけをキャンバス背景色で上から薄く塗り、
    // 「このあたりが消える」という結果を軽く見せる。
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
Frame previewFrameWithEraser(const Frame& frame, int activeLayerIndex, const Stroke& eraserStroke)
{
    Frame preview = frame;
    if (eraserStroke.points.empty() || activeLayerIndex < 0 || activeLayerIndex >= static_cast<int>(preview.layers.size())) {
        return preview;
    }
    Layer& layer = preview.layers[static_cast<std::size_t>(activeLayerIndex)];
    std::vector<Stroke> rewrittenStrokes;
    rewrittenStrokes.reserve(layer.strokes.size());
    bool changed = false;
    const float radius = std::max(1.0f, eraserStroke.radiusPx);
    for (const Stroke& stroke : layer.strokes) {
        std::vector<Stroke> parts = splitStrokeByEraser(stroke, eraserStroke, radius, changed);
        rewrittenStrokes.insert(rewrittenStrokes.end(),
                                std::make_move_iterator(parts.begin()),
                                std::make_move_iterator(parts.end()));
    }
    if (changed) {
        layer.strokes = std::move(rewrittenStrokes);
    }
    return preview;
}
} // namespace
void App::drawDrawingMode()
{
    const bool isColoringMode = currentMode_ == AppMode::Coloring;
    if (isColoringMode) {
        selectPaintLayerForColoring(true);
    }
    clampSelection();
    handleFrameShortcuts();
    updateFramePlayback();
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
    if (currentMode_ == AppMode::Coloring) {
        ImGui::TextUnformatted(u8c(u8"③.5 彩色モード"));
        ImGui::TextWrapped(u8c(u8"Paintレイヤーを編集対象にし、Normal/ColorTrace/ShadowGuideを参照表示します。"));
        if (ImGui::Button(u8c(u8"Paintレイヤーへ移動"), ImVec2(-1.0f, 0.0f))) {
            selectPaintLayerForColoring(true);
        }
        ImGui::Separator();
    }

    ui::drawBrushPanel(brushSettings_);
    ImGui::Separator();
    ui::drawColorPanel(colorPanelState_, brushSettings_);
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
    const ui::CellPanelResult cellPanelResult = ui::drawCellPanel(project_, activeCellIndex_);
    const auto resetPreviewReadiness = [&]() {
        previewWarmCursor_ = activeFrameIndex_;
        previewReadyFlags_.clear();
        previewReadyCount_ = 0;
        previewReadyScanCursor_ = activeFrameIndex_;
    };
    if (cellPanelResult.projectStructureChanged) {
        activeCellIndex_ = cellPanelResult.selectedCellIndex;
        clampSelection();
        resetPreviewReadiness();
        canvasRenderer_.clearLayerCaches();
        lastMessage_ = cellPanelResult.selectionChanged ? "active cell changed" : "cell structure changed";
    } else if (cellPanelResult.selectionChanged) {
        activeCellIndex_ = cellPanelResult.selectedCellIndex;
        clampSelection();
        resetPreviewReadiness();
        canvasRenderer_.markAllDirty();
        lastMessage_ = "active cell changed";
    } else if (cellPanelResult.displayChanged) {
        canvasRenderer_.markAllDirty();
        lastMessage_ = "cell display changed";
    }

    ImGui::Separator();

    Frame* frame = activeFrame();
    Cell* cell = activeCell();

    if (frame == nullptr || cell == nullptr) {
        ImGui::TextUnformatted("No active cell or frame.");
        return;
    }
    ImGui::TextDisabled(currentMode_ == AppMode::Coloring ? "Phase 2-pre CellPanel v1 coloring mode" : "Phase 2-pre CellPanel v1 drawing mode");
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
    drawFingerPlaybackControls();
    drawLightTableControls();
    ImGui::Separator();
    ImGui::BeginChild("DrawingTimeline_v23_scroll", ImVec2(0.0f, 0.0f), true,
                      ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_AlwaysHorizontalScrollbar);
    const int prevFrameIndex = activeFrameIndex_;
    const ui::TimelinePanelAction timelineAction =
        ui::drawTimelinePanel(*cell, activeFrameIndex_, onionPrevious_, onionNext_);
    if (activeFrameIndex_ != prevFrameIndex) {
        canvasRenderer_.markAllDirty();
    }
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
    const bool isColoringMode = currentMode_ == AppMode::Coloring;

    // Phase 1.5 Step 18:
    // 作画・彩色どちらでも、実際の紙に近い白いキャンバス背景にする。
    // パネルやアプリ全体のダークテーマは維持し、描画範囲だけ白くする。
    const ImU32 background = IM_COL32(255, 255, 255, 255);
    drawList->AddRectFilled(areaMin, ImVec2(areaMin.x + areaSize.x, areaMin.y + areaSize.y), background);
    const bool drawAssistOverlays = !isPlayingFrames_ || !playbackSkipAssistOverlays_;
    if (drawAssistOverlays) {
        const Cell* onionCell = activeCell();
        const Frame* previous = (onionCell != nullptr && activeFrameIndex_ > 0)
            ? onionCell->frameOrNull(activeFrameIndex_ - 1)
            : nullptr;
        const Frame* next = (onionCell != nullptr) ? onionCell->frameOrNull(activeFrameIndex_ + 1) : nullptr;
        if (onionPrevious_ && previous != nullptr) {
            drawOnionFrameDirect(*previous, true, 0.72f, canvasView_, areaMin, areaSize, drawList);
        }
        if (onionNext_ && next != nullptr) {
            drawOnionFrameDirect(*next, false, 0.72f, canvasView_, areaMin, areaSize, drawList);
        }
        drawLightTableOverlay(areaMin, areaSize, drawList);
    }
    const bool myPaintRealtimePreview = isMyPaintStrokeActive_;
    const Stroke* preview = (isDrawingStroke_
        && brushSettings_.tool == ui::ToolKind::Brush
        && !myPaintRealtimePreview) ? &currentStroke_ : nullptr;
        const CanvasDisplayMode canvasDisplayMode = isColoringMode ? CanvasDisplayMode::Coloring : CanvasDisplayMode::Drawing;
    const ui::CellDisplayMode cellDisplayMode = ui::currentCellDisplayMode();
    const int soloCellIndex = ui::currentSoloCellIndex();

    auto drawCellFrame = [&](int cellIndex, const Cell& drawCell, const Frame& drawFrame, int drawFrameIndex) {
        const bool isActiveCell = cellIndex == activeCellIndex_;
        const int drawActiveLayer = isActiveCell ? activeLayerIndex_ : -1;
        const Stroke* drawPreview = isActiveCell ? preview : nullptr;
        const float drawOpacity = std::clamp(drawCell.opacity, 0.0f, 1.0f);
        if (drawOpacity <= 0.0f) {
            return;
        }
        canvasRenderer_.draw(drawFrame,
                             drawFrameIndex,
                             drawActiveLayer,
                             drawPreview,
                             drawOpacity,
                             canvasView_,
                             canvasDisplayMode,
                             areaMin,
                             areaSize,
                             drawList);
    };

    if (cellDisplayMode == ui::CellDisplayMode::ActiveOnly || project_.cells.empty()) {
        drawCellFrame(activeCellIndex_, *activeCell(), *frame, activeFrameIndex_);
    } else if (cellDisplayMode == ui::CellDisplayMode::SoloSelected) {
        const int safeSoloIndex = std::clamp(soloCellIndex >= 0 ? soloCellIndex : activeCellIndex_, 0, static_cast<int>(project_.cells.size()) - 1);
        const Cell& soloCell = project_.cells[static_cast<std::size_t>(safeSoloIndex)];
        if (!soloCell.frames.empty()) {
            const int soloFrameIndex = std::clamp(activeFrameIndex_, 0, static_cast<int>(soloCell.frames.size()) - 1);
            const Frame& soloFrame = soloCell.frames[static_cast<std::size_t>(soloFrameIndex)];
            drawCellFrame(safeSoloIndex, soloCell, soloFrame, soloFrameIndex);
        }
    } else {
        for (int cellIndex = 0; cellIndex < static_cast<int>(project_.cells.size()); ++cellIndex) {
            const Cell& drawCell = project_.cells[static_cast<std::size_t>(cellIndex)];
            if (!drawCell.visible || drawCell.opacity <= 0.0f || drawCell.frames.empty()) {
                continue;
            }
            const int drawFrameIndex = std::clamp(activeFrameIndex_, 0, static_cast<int>(drawCell.frames.size()) - 1);
            const Frame& drawFrame = drawCell.frames[static_cast<std::size_t>(drawFrameIndex)];
            drawCellFrame(cellIndex, drawCell, drawFrame, drawFrameIndex);
        }
    }
    warmPlaybackFrameCache();
    if (isDrawingStroke_ && brushSettings_.tool == ui::ToolKind::Eraser && !currentStroke_.points.empty()) {
        drawLightweightEraserPreview(currentStroke_, canvasView_, areaMin, areaSize, drawList);
    }
    const bool eraserCursor = isDrawingStroke_
        && brushSettings_.tool == ui::ToolKind::Eraser
        && !currentStroke_.points.empty();
    if (eraserCursor) {
        const StrokePoint& point = currentStroke_.points.back();
        const ImVec2 screenPoint = canvasView_.canvasToScreen(point.x, point.y, areaMin, areaSize);
        const float radius = std::max(2.0f, currentStroke_.radiusPx * std::clamp(canvasView_.zoom, 0.05f, 32.0f));
        drawList->AddCircle(screenPoint, radius, IM_COL32(255, 80, 70, 210), 32, 2.0f);
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

    const bool useMyPaintRealtime = isMyPaintStrokeActive_;

    if (!currentStroke_.points.empty()) {
        pushUndoSnapshot();
        Layer* layer = activeLayer();
        if (layer != nullptr) {
            if (brushSettings_.tool == ui::ToolKind::Brush) {
                if (useMyPaintRealtime) {
                    // MyPaint逐次処理: ピクセルはaddPoint()で書き込み済み。
                    // bakeStrokeは呼ばない。点列の保存とendStroke()のみ。
                    myPaintEngine_.endStroke();
                    isMyPaintStrokeActive_ = false;
                    layer->strokes.push_back(currentStroke_);

                    // markAllDirtyは不要。ここでは直接描画済みCanvasBitmapとProject点列のrevisionだけ同期する。
                    canvasRenderer_.markActiveBitmapClean(activeLayerIndex_, *layer);
                    lastMessage_ = "MyPaint stroke committed";
                } else {
                    // Simple互換: 確定時にbakeStrokeで焼き込む。
                    layer->strokes.push_back(currentStroke_);
                    canvasRenderer_.bakeStrokeOnLayer(activeLayerIndex_, currentStroke_, currentStroke_.opacity);
                    canvasRenderer_.markAllDirty();
                    lastMessage_ = "stroke committed to active frame";
                }
            } else {
                removeIntersectingStrokes(currentStroke_);
                canvasRenderer_.markAllDirty();
            }
        }
    } else if (useMyPaintRealtime) {
        myPaintEngine_.endStroke();
    }

    currentStroke_ = Stroke{};
    isDrawingStroke_ = false;
    isMyPaintStrokeActive_ = false;
}
void App::cancelStroke()
{
    const bool useMyPaintRealtime = isMyPaintStrokeActive_;
    if (useMyPaintRealtime) {
        myPaintEngine_.endStroke();
        // 未確定の直接描画ピクセルを捨てるため、保存済みストロークから再構築する。
        canvasRenderer_.markAllDirty();
    }
    currentStroke_ = Stroke{};
    isDrawingStroke_ = false;
    isMyPaintStrokeActive_ = false;
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
        lastMessage_ = "eraser applied: partial stroke split v22";
    } else {
        lastMessage_ = "eraser applied: no hit, radius=" + std::to_string(radius) +
                       ", strokes=" + std::to_string(beforeCount);
    }
}
} // namespace perapera
