// このファイルの役割:
// レイヤー・フレーム操作など、Projectを変更する小さな操作を実装する。
// フレーム追加は現在表示中の絵を勝手に切り替えず、タイムラインから新規フレームを選ばせる。

#include "ui/App.h"

#include <algorithm>
#include <cstddef>
#include <iomanip>
#include <sstream>

#include "ui/AppProjectIOSupport.h"

namespace perapera {
namespace {

std::string frameNameForIndex(int zeroBasedIndex)
{
    std::ostringstream name;
    name << "frame_" << std::setw(3) << std::setfill('0') << (zeroBasedIndex + 1);
    return name.str();
}

std::string layerIdForNumber(int layerNumber)
{
    std::ostringstream id;
    id << "layer_" << std::setw(3) << std::setfill('0') << layerNumber;
    return id.str();
}

int nextAvailableLayerNumber(const Frame& frame)
{
    for (int layerNumber = 1;; ++layerNumber) {
        const std::string candidate = layerIdForNumber(layerNumber);
        bool used = false;
        for (const Layer& layer : frame.layers) {
            if (layer.layerId == candidate) {
                used = true;
                break;
            }
        }
        if (!used) {
            return layerNumber;
        }
    }
}

void renumberFrames(Cell& cell)
{
    for (int index = 0; index < static_cast<int>(cell.frames.size()); ++index) {
        cell.frames[static_cast<std::size_t>(index)].name = frameNameForIndex(index);
    }
}

void shiftTimesheetExposuresForFrameInsert(Project& project, const std::string& cellId, int insertIndex)
{
    if (cellId.empty()) {
        return;
    }
    for (TimesheetFrame& timesheetFrame : project.timesheet.frames) {
        TimesheetCellExposure* exposure = timesheetFrame.exposureForCell(cellId);
        if (exposure != nullptr && exposure->drawingFrameIndex >= insertIndex) {
            ++exposure->drawingFrameIndex;
        }
    }
}

void shiftTimesheetExposuresForFrameDelete(Project& project, const std::string& cellId, int deleteIndex)
{
    if (cellId.empty()) {
        return;
    }
    for (TimesheetFrame& timesheetFrame : project.timesheet.frames) {
        TimesheetCellExposure* exposure = timesheetFrame.exposureForCell(cellId);
        if (exposure == nullptr) {
            continue;
        }
        if (exposure->drawingFrameIndex > deleteIndex) {
            --exposure->drawingFrameIndex;
        } else if (exposure->drawingFrameIndex == deleteIndex) {
            exposure->drawingFrameIndex = std::max(0, deleteIndex - 1);
        }
    }
}

void setTimesheetExposureForActiveSlot(Project& project,
                                       const Cell& cell,
                                       int timelineFrameIndex,
                                       int drawingFrameIndex)
{
    if (cell.id.empty()) {
        return;
    }
    const int totalFrames = std::max(1, project.timeline.totalFrames);
    project.timesheet.ensureFrameCount(totalFrames);
    project.timesheet.ensureCell(cell.id, 0);
    timelineFrameIndex = std::clamp(timelineFrameIndex, 0, totalFrames - 1);
    drawingFrameIndex = std::clamp(drawingFrameIndex, 0, std::max(0, static_cast<int>(cell.frames.size()) - 1));
    TimesheetCellExposure* exposure = project.timesheet.exposureOrNull(timelineFrameIndex, cell.id);
    if (exposure == nullptr) {
        return;
    }
    exposure->drawingFrameIndex = drawingFrameIndex;
    if (exposure->kind == TimesheetExposureKind::Null) {
        exposure->kind = TimesheetExposureKind::Hold;
    }
}

} // namespace

void App::addLayer()
{
    Frame* frame = activeFrame();
    if (frame == nullptr) {
        lastMessage_ = "layer add failed: no active frame";
        return;
    }

    pushUndoSnapshot();
    const int newNumber = nextAvailableLayerNumber(*frame);
    frame->layers.push_back(Layer::createDefault(newNumber));
    appio::normalizeCellStructure(project_);
    activeLayerIndex_ = static_cast<int>(frame->layers.size()) - 1;
    canvasRenderer_.clearLayerCaches();
    lastMessage_ = "layer added";
}

void App::deleteActiveLayer()
{
    Frame* frame = activeFrame();
    if (frame == nullptr || frame->layers.size() <= 1U) {
        lastMessage_ = "cannot delete last layer";
        return;
    }

    pushUndoSnapshot();
    activeLayerIndex_ = std::clamp(activeLayerIndex_, 0, static_cast<int>(frame->layers.size()) - 1);
    frame->layers.erase(frame->layers.begin() + activeLayerIndex_);
    appio::normalizeCellStructure(project_);
    clampSelection();
    canvasRenderer_.clearLayerCaches();
    lastMessage_ = "layer deleted";
}

void App::clearActiveLayer()
{
    Layer* layer = activeLayer();
    if (layer == nullptr) {
        lastMessage_ = "layer clear failed: no active layer";
        return;
    }

    pushUndoSnapshot();
    layer->strokes.clear();
    canvasRenderer_.markAllDirty();
    lastMessage_ = "layer cleared";
}

void App::syncActiveTimesheetExposureToDrawingFrame()
{
    Cell* cell = activeCell();
    if (cell == nullptr) {
        return;
    }
    activeFrameIndex_ = std::clamp(activeFrameIndex_, 0, std::max(0, static_cast<int>(cell->frames.size()) - 1));
    setTimesheetExposureForActiveSlot(project_, *cell, activeTimelineFrameIndex_, activeFrameIndex_);
}

void App::addFrame()
{
    Cell* cell = activeCell();
    if (cell == nullptr) {
        lastMessage_ = "frame add failed: no active cell";
        return;
    }

    pushUndoSnapshot();

    const int frameCount = static_cast<int>(cell->frames.size());
    activeFrameIndex_ = frameCount <= 0 ? 0 : std::clamp(activeFrameIndex_, 0, frameCount - 1);
    const int insertIndex = frameCount <= 0 ? 0 : activeFrameIndex_ + 1;

    shiftTimesheetExposuresForFrameInsert(project_, cell->id, insertIndex);
    cell->frames.insert(cell->frames.begin() + insertIndex, Frame::createDefault(insertIndex + 1));
    renumberFrames(*cell);
    appio::normalizeCellStructure(project_);

    activeFrameIndex_ = insertIndex;
    activeFrameIndex_ = std::clamp(activeFrameIndex_, 0, static_cast<int>(cell->frames.size()) - 1);
    activeLayerIndex_ = 0;
    syncActiveTimesheetExposureToDrawingFrame();
    clampSelection();
    canvasRenderer_.clearLayerCaches();
    lastMessage_ = "blank frame added and selected.";
}

void App::duplicateFrame()
{
    Cell* cell = activeCell();
    Frame* frame = activeFrame();
    if (cell == nullptr || frame == nullptr) {
        lastMessage_ = "frame duplicate failed: no active frame";
        return;
    }

    pushUndoSnapshot();
    const int insertIndex = std::clamp(activeFrameIndex_ + 1, 0, static_cast<int>(cell->frames.size()));
    Frame duplicate = *frame;
    shiftTimesheetExposuresForFrameInsert(project_, cell->id, insertIndex);
    cell->frames.insert(cell->frames.begin() + insertIndex, duplicate);
    renumberFrames(*cell);
    appio::normalizeCellStructure(project_);
    activeFrameIndex_ = insertIndex;
    activeLayerIndex_ = 0;
    syncActiveTimesheetExposureToDrawingFrame();
    clampSelection();
    canvasRenderer_.clearLayerCaches();
    lastMessage_ = "frame duplicated: count=" + std::to_string(cell->frames.size());
}

void App::deleteActiveFrame()
{
    Cell* cell = activeCell();
    if (cell == nullptr) {
        lastMessage_ = "frame delete failed: no active cell";
        return;
    }
    if (cell->frames.size() <= 1U) {
        lastMessage_ = "cannot delete last frame";
        return;
    }

    pushUndoSnapshot();
    activeFrameIndex_ = std::clamp(activeFrameIndex_, 0, static_cast<int>(cell->frames.size()) - 1);
    const int deleteIndex = activeFrameIndex_;
    shiftTimesheetExposuresForFrameDelete(project_, cell->id, deleteIndex);
    cell->frames.erase(cell->frames.begin() + deleteIndex);
    renumberFrames(*cell);
    appio::normalizeCellStructure(project_);
    activeFrameIndex_ = std::clamp(activeFrameIndex_, 0, static_cast<int>(cell->frames.size()) - 1);
    activeLayerIndex_ = 0;
    syncActiveTimesheetExposureToDrawingFrame();
    clampSelection();
    canvasRenderer_.clearLayerCaches();
    lastMessage_ = "frame deleted: count=" + std::to_string(cell->frames.size());
}

} // namespace perapera
