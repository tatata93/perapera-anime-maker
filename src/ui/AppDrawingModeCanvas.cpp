#include "ui/App.h"

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <string>
#include <vector>

#include <imgui.h>

#include "core/CellOrderResolver.h"
#include "core/TimesheetResolver.h"
#include "core/TimesheetSceneResolver.h"
#include "ui/AppDrawingModeEraser.h"
#include "ui/AppDrawingModeOverlay.h"
#include "ui/AppDrawingModeTimesheet.h"
#include "ui/panels/CellPanel.h"

namespace perapera {
namespace {

const char* u8c(const char8_t* text)
{
    return reinterpret_cast<const char*>(text);
}

} // namespace

using app_drawing::adjacentPlaybackOrderFrameIndex;
using app_drawing::buildTimesheetPlaybackOrderFrameIndicesForCell;
using app_drawing::countTimesheetEntries;
using app_drawing::drawLightweightEraserPreview;
using app_drawing::drawOnionFrameDirect;
using app_drawing::splitStrokeByEraser;
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
        int previousOnionFrameIndex = activeFrameIndex_ - 1;
        int nextOnionFrameIndex = activeFrameIndex_ + 1;

        // Timesheet Rebuild Step 7.9:
        // タイムシート入力がある場合、オニオンも作画F番号順ではなくT順の再生順を優先する。
        // 例: T1=F1, T3=F3, T5=F4, T7=F2 なら F1→F3→F4→F2 の前後を出す。
        if (onionCell != nullptr && countTimesheetEntries(workingTimesheet_) > 0) {
            const std::vector<int> onionPlaybackOrder =
                buildTimesheetPlaybackOrderFrameIndicesForCell(workingTimesheet_, *onionCell);
            const int previousByPlaybackOrder = adjacentPlaybackOrderFrameIndex(onionPlaybackOrder, activeFrameIndex_, -1);
            const int nextByPlaybackOrder = adjacentPlaybackOrderFrameIndex(onionPlaybackOrder, activeFrameIndex_, 1);
            if (previousByPlaybackOrder >= 0) {
                previousOnionFrameIndex = previousByPlaybackOrder;
            }
            if (nextByPlaybackOrder >= 0) {
                nextOnionFrameIndex = nextByPlaybackOrder;
            }
        }

        const Frame* previous = (onionCell != nullptr && previousOnionFrameIndex >= 0)
            ? onionCell->frameOrNull(previousOnionFrameIndex)
            : nullptr;
        const Frame* next = (onionCell != nullptr && nextOnionFrameIndex >= 0)
            ? onionCell->frameOrNull(nextOnionFrameIndex)
            : nullptr;
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

    // Timesheet Rebuild Step 6.6:
    // タイムシートに1件以上入力がある場合だけ、T選択をキャンバス表示へ反映する。
    // ただし指パラ/通常タイムライン再生中は activeFrameIndex_ の再生表示を優先する。
    // activeFrameIndex_ は作画F編集対象として維持し、タイムシートT表示とは混ぜない。
    const int timesheetEntryCount = countTimesheetEntries(workingTimesheet_);
    const bool hasTimesheetEntries = timesheetEntryCount > 0;
    const bool useTimesheetPreview = hasTimesheetEntries && !isPlayingFrames_;
    const int timesheetTimelineFrame = useTimesheetPreview
        ? clampTimesheetFrame(workingTimesheet_, timesheetPanelState_.selectedTimelineFrame + 1)
        : activeFrameIndex_ + 1;

    auto drawCellFrame = [&](int cellIndex, const Cell& drawCell, const Frame& drawFrame, int drawFrameIndex, const CellPlacement* placementOverride = nullptr) {
        const bool isActiveCell = cellIndex == activeCellIndex_;
        const bool isActiveEditingFrame = isActiveCell && drawFrameIndex == activeFrameIndex_;
        const int drawActiveLayer = isActiveEditingFrame ? activeLayerIndex_ : -1;
        const Stroke* drawPreview = isActiveEditingFrame ? preview : nullptr;
        const float drawOpacity = std::clamp(drawCell.opacity, 0.0f, 1.0f);
        if (drawOpacity <= 0.0f) {
            return;
        }
        const CellPlacement* drawPlacement = placementOverride != nullptr ? placementOverride : &drawCell.placement;
        canvasRenderer_.draw(drawFrame,
                             drawFrameIndex,
                             drawActiveLayer,
                             drawPreview,
                             drawOpacity,
                             canvasView_,
                             canvasDisplayMode,
                             areaMin,
                             areaSize,
                             drawList,
                             drawPlacement);
    };

    auto appendTimesheetSceneInput = [&](std::vector<TimesheetSceneCellInput>& inputs,
                                          int cellIndex,
                                          const Cell* cell,
                                          bool requireCellVisible) {
        if (cell == nullptr) {
            return;
        }
        inputs.push_back(TimesheetSceneCellInput{cell, cellIndex, requireCellVisible});
    };

    auto buildTimesheetSceneInputsForCurrentDisplay = [&]() {
        std::vector<TimesheetSceneCellInput> inputs;
        if (cellDisplayMode == ui::CellDisplayMode::ActiveOnly || project_.cells.empty()) {
            appendTimesheetSceneInput(inputs, activeCellIndex_, activeCell(), false);
            return inputs;
        }

        if (cellDisplayMode == ui::CellDisplayMode::SoloSelected) {
            const int safeSoloIndex = std::clamp(
                soloCellIndex >= 0 ? soloCellIndex : activeCellIndex_,
                0,
                static_cast<int>(project_.cells.size()) - 1);
            appendTimesheetSceneInput(inputs,
                                      safeSoloIndex,
                                      &project_.cells[static_cast<std::size_t>(safeSoloIndex)],
                                      false);
            return inputs;
        }

        const std::vector<int> orderedCellIndices = resolvedProjectCellOrderIndices(project_);
        inputs.reserve(orderedCellIndices.size());
        for (const int cellIndex : orderedCellIndices) {
            appendTimesheetSceneInput(inputs,
                                      cellIndex,
                                      &project_.cells[static_cast<std::size_t>(cellIndex)],
                                      true);
        }
        return inputs;
    };

    auto drawEditingFrameDisplay = [&]() {
        if (cellDisplayMode == ui::CellDisplayMode::ActiveOnly || project_.cells.empty()) {
            drawCellFrame(activeCellIndex_, *activeCell(), *frame, activeFrameIndex_);
            return;
        }

        if (cellDisplayMode == ui::CellDisplayMode::SoloSelected) {
            const int safeSoloIndex = std::clamp(
                soloCellIndex >= 0 ? soloCellIndex : activeCellIndex_,
                0,
                static_cast<int>(project_.cells.size()) - 1);
            const Cell& soloCell = project_.cells[static_cast<std::size_t>(safeSoloIndex)];
            if (!soloCell.frames.empty()) {
                const int soloFrameIndex = std::clamp(activeFrameIndex_, 0, static_cast<int>(soloCell.frames.size()) - 1);
                const Frame& soloFrame = soloCell.frames[static_cast<std::size_t>(soloFrameIndex)];
                drawCellFrame(safeSoloIndex, soloCell, soloFrame, soloFrameIndex);
            }
            return;
        }

        for (const int cellIndex : resolvedProjectCellOrderIndices(project_)) {
            const Cell& drawCell = project_.cells[static_cast<std::size_t>(cellIndex)];
            if (!drawCell.visible || drawCell.opacity <= 0.0f || drawCell.frames.empty()) {
                continue;
            }
            const int drawFrameIndex = std::clamp(activeFrameIndex_, 0, static_cast<int>(drawCell.frames.size()) - 1);
            const Frame& drawFrame = drawCell.frames[static_cast<std::size_t>(drawFrameIndex)];
            drawCellFrame(cellIndex, drawCell, drawFrame, drawFrameIndex);
        }
    };

    int timesheetResolvedSceneCellCount = 0;
    bool timesheetPreviewEditMismatch = false;
    bool timesheetPreviewActiveCellDrawable = false;
    bool timesheetPreviewHiddenForEditing = false;
    int timesheetPreviewActiveDrawingFrameNumber = 0;
    if (useTimesheetPreview) {
        const ResolvedTimesheetSceneFrame resolvedScene = resolveTimesheetSceneFrame(
            workingTimesheet_,
            buildTimesheetSceneInputsForCurrentDisplay(),
            timesheetTimelineFrame);
        timesheetResolvedSceneCellCount = static_cast<int>(resolvedScene.cells.size());

        if (const Cell* editingCell = activeCell(); editingCell != nullptr && !editingCell->frames.empty()) {
            const ResolvedTimesheetCell activeResolved =
                resolveTimesheetCell(workingTimesheet_, editingCell->id, timesheetTimelineFrame);
            timesheetPreviewActiveDrawingFrameNumber = activeResolved.drawingFrameNumber;
            timesheetPreviewActiveCellDrawable = activeResolved.visible &&
                activeResolved.drawingFrameNumber > 0 &&
                activeResolved.drawingFrameNumber <= static_cast<int>(editingCell->frames.size());
            timesheetPreviewEditMismatch = !timesheetPreviewActiveCellDrawable ||
                activeResolved.drawingFrameNumber != activeFrameIndex_ + 1;
        }

        // Timesheet Rebuild Step 7.10.5:
        // タイムラインでTS表示と違う作画Fを明示選択した時は、TS由来の作画Fを同時表示しない。
        // 作画時は「今選んでいる作画Fだけ」を見せるのを標準にする。
        timesheetPreviewHiddenForEditing = timesheetPreviewEditMismatch && timesheetPreferEditingFrameWhenMismatch_;

        if (timesheetPreviewHiddenForEditing) {
            drawEditingFrameDisplay();
        } else {
            for (const ResolvedTimesheetSceneCell& resolvedCell : resolvedScene.cells) {
                if (resolvedCell.cell == nullptr || resolvedCell.drawingFrameIndex < 0) {
                    continue;
                }
                const Frame& drawFrame = resolvedCell.cell->frames[static_cast<std::size_t>(resolvedCell.drawingFrameIndex)];
                drawCellFrame(resolvedCell.cellIndex,
                              *resolvedCell.cell,
                              drawFrame,
                              resolvedCell.drawingFrameIndex,
                              &resolvedCell.placement);
            }

            if (timesheetPreviewEditMismatch &&
                timesheetDrawActiveFrameOverPreview_ &&
                activeFrameIndex_ >= 0) {
                if (const Cell* editingCell = activeCell();
                    editingCell != nullptr &&
                    activeFrameIndex_ < static_cast<int>(editingCell->frames.size())) {
                    const Frame& editingFrame = editingCell->frames[static_cast<std::size_t>(activeFrameIndex_)];
                    drawCellFrame(activeCellIndex_, *editingCell, editingFrame, activeFrameIndex_);
                }
            }
        }
    } else {
        drawEditingFrameDisplay();
    }

    // Timesheet Rebuild Step 7.10:
    // T表示と編集対象Fがズレても、キャンバス入力は止めない。
    // ズレがある場合は編集中作画Fを上に重ねて、作画できる状態を優先する。
    bool allowCanvasInput = activeFrame() != nullptr;

    if (hasTimesheetEntries) {
        // Timesheet Rebuild Step 7.2:
        // キャンバス左上の状態表示は、前段の大きい説明ボックスではなく小型バッジに戻す。
        // 「表示セルN」は残しつつ、作画画面を邪魔しないサイズにする。
        const std::string timesheetPreviewModeText = useTimesheetPreview
            ? (timesheetPreviewHiddenForEditing ? std::string("EDIT") : std::string("ON"))
            : std::string("OFF");
        const std::string previewText = std::string(u8c(u8"TS ")) +
            timesheetPreviewModeText +
            std::string(u8c(u8" T")) +
            std::to_string(timesheetTimelineFrame) +
            std::string(u8c(u8" 編集F")) +
            std::to_string(activeFrameIndex_ + 1) +
            std::string(u8c(u8" 表示セルN=")) +
            std::to_string(timesheetPreviewHiddenForEditing ? 0 : timesheetResolvedSceneCellCount) +
            std::string(u8c(u8" entries=")) +
            std::to_string(timesheetEntryCount);
        const char* warningText = timesheetPreviewHiddenForEditing
            ? u8c(u8"タイムライン選択を優先: TS由来Fは非表示")
            : (timesheetPreviewEditMismatch
                ? u8c(u8"TS表示と編集Fが違うため、編集中Fを上に表示")
                : u8c(u8""));
        const ImVec2 textSize = ImGui::CalcTextSize(previewText.c_str());
        const ImVec2 warningSize = timesheetPreviewEditMismatch ? ImGui::CalcTextSize(warningText) : ImVec2(0.0f, 0.0f);
        const float overlayWidth = std::min(areaSize.x - 16.0f,
                                           std::max(textSize.x, warningSize.x) + 14.0f);
        const float overlayHeight = timesheetPreviewEditMismatch ? 42.0f : 24.0f;
        drawList->AddRectFilled(
            ImVec2(areaMin.x + 8.0f, areaMin.y + 8.0f),
            ImVec2(areaMin.x + 8.0f + overlayWidth, areaMin.y + 8.0f + overlayHeight),
            IM_COL32(255, 255, 255, 220),
            4.0f);
        drawList->AddRect(
            ImVec2(areaMin.x + 8.0f, areaMin.y + 8.0f),
            ImVec2(areaMin.x + 8.0f + overlayWidth, areaMin.y + 8.0f + overlayHeight),
            IM_COL32(80, 80, 80, 180),
            4.0f);
        drawList->AddText(
            ImVec2(areaMin.x + 14.0f, areaMin.y + 13.0f),
            IM_COL32(25, 25, 25, 255),
            previewText.c_str());
        if (timesheetPreviewEditMismatch) {
            drawList->AddText(
                ImVec2(areaMin.x + 14.0f, areaMin.y + 31.0f),
                IM_COL32(80, 80, 80, 255),
                warningText);
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
    // allowCanvasInput=false は作画入力だけを止める。ズーム/中ボタン移動は常に使える。
    handleCanvasInput(areaMin, areaSize, allowCanvasInput);
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
