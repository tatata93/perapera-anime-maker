#include "ui/App.h"

#include <algorithm>
#include <cstddef>
#include <string>

#include "core/TimesheetResolver.h"
#include "ui/AppDrawingModeTimesheet.h"

namespace perapera {

using app_drawing::countTimesheetEntries;
using app_drawing::projectCellById;
using app_drawing::selectedTimesheetPanelEntry;
using app_drawing::TimesheetPlaybackRange;
using app_drawing::TimesheetPlaybackStep;
using app_drawing::timesheetPanelKindUsesDrawingFrame;

void App::setTimesheetPreviewFrame(const ui::TimesheetPanelViewModel& timesheetPanelData,
                                   int zeroBasedFrame,
                                   const char* reason)
{
    const int nextFrame = app_drawing::clampTimesheetPreviewFrame(timesheetPanelData.totalFrames, zeroBasedFrame);
    if (timesheetPanelState_.selectedTimelineFrame == nextFrame) {
        return;
    }

    timesheetPanelState_.selectedTimelineFrame = nextFrame;
    canvasRenderer_.markAllDirty();
    lastMessage_ = std::string(reason) + " T" + std::to_string(nextFrame + 1) +
        " / edit F" + std::to_string(activeFrameIndex_ + 1);
}

bool App::syncEditingTargetToSelectedTimesheetCell(const ui::TimesheetPanelViewModel& timesheetPanelData,
                                                   const char* reason,
                                                   bool allowResolvedHold)
{
    if (timesheetPanelData.cells.empty() || project_.cells.empty()) {
        return false;
    }

    const int safeSelectedCellColumn = std::clamp(
        timesheetPanelState_.selectedCellColumn,
        0,
        static_cast<int>(timesheetPanelData.cells.size()) - 1);
    const ui::TimesheetPanelCellColumn& selectedColumn =
        timesheetPanelData.cells[static_cast<std::size_t>(safeSelectedCellColumn)];

    int selectedProjectCellIndex = -1;
    Cell* selectedCell = projectCellById(project_, selectedColumn.cellId, &selectedProjectCellIndex);
    if (selectedCell == nullptr || selectedCell->frames.empty()) {
        return false;
    }

    int desiredDrawingFrameNumber = 0;
    const ui::TimesheetPanelEditableEntry* selectedEntry =
        selectedTimesheetPanelEntry(timesheetPanelData, timesheetPanelState_);
    if (selectedEntry != nullptr && timesheetPanelKindUsesDrawingFrame(selectedEntry->kind)) {
        desiredDrawingFrameNumber = selectedEntry->drawingFrameNumber;
    } else if (allowResolvedHold && countTimesheetEntries(workingTimesheet_) > 0) {
        const int selectedT = clampTimesheetFrame(workingTimesheet_, timesheetPanelState_.selectedTimelineFrame + 1);
        const ResolvedTimesheetCell resolved =
            resolveTimesheetCell(workingTimesheet_, selectedColumn.cellId, selectedT);
        if (resolved.visible && resolved.drawingFrameNumber > 0) {
            desiredDrawingFrameNumber = resolved.drawingFrameNumber;
        }
    }

    if (desiredDrawingFrameNumber < 1 ||
        desiredDrawingFrameNumber > static_cast<int>(selectedCell->frames.size())) {
        return false;
    }

    const int desiredFrameIndex = desiredDrawingFrameNumber - 1;
    const bool changed = activeCellIndex_ != selectedProjectCellIndex ||
        activeFrameIndex_ != desiredFrameIndex;

    activeCellIndex_ = selectedProjectCellIndex;
    activeFrameIndex_ = desiredFrameIndex;
    activeLayerIndex_ = 0;
    clampSelection();

    if (changed) {
        canvasRenderer_.markAllDirty();
        lastMessage_ = std::string(reason) +
            ": edit target follows selected T -> cell " +
            std::to_string(activeCellIndex_ + 1) +
            " / F" + std::to_string(activeFrameIndex_ + 1);
    }
    return true;
}

void App::stepTimesheetPreviewFrame(const ui::TimesheetPanelViewModel& timesheetPanelData, int delta)
{
    const TimesheetPlaybackStep step = app_drawing::stepTimesheetPreviewFrameIndex(
        timesheetPanelData.totalFrames,
        timesheetPanelState_.selectedTimelineFrame,
        delta,
        timesheetPlaybackPingPong_,
        timesheetPlaybackDirection_);
    if (!step.canPlay) {
        isPlayingTimesheet_ = false;
        timesheetPlaybackAccumulator_ = 0.0f;
        return;
    }

    timesheetPlaybackDirection_ = step.direction;
    setTimesheetPreviewFrame(timesheetPanelData, step.requestedFrame, "timesheet playback");
}

void App::normalizeTimesheetPlaybackRange(const ui::TimesheetPanelViewModel& timesheetPanelData)
{
    const TimesheetPlaybackRange range = app_drawing::normalizeTimesheetPlaybackRange(
        timesheetPanelData.totalFrames,
        timesheetPlaybackRangeStartFrame_,
        timesheetPlaybackRangeEndFrame_);
    timesheetPlaybackRangeStartFrame_ = range.startFrame;
    timesheetPlaybackRangeEndFrame_ = range.endFrame;
}

void App::setTimesheetPlaybackRangeFromOneBased(const ui::TimesheetPanelViewModel& timesheetPanelData,
                                                int startT,
                                                int endT,
                                                const char* reason)
{
    const int lastT = std::max(1, timesheetPanelData.totalFrames);
    startT = std::clamp(startT, 1, lastT);
    endT = std::clamp(endT, 1, lastT);
    if (startT > endT) {
        std::swap(startT, endT);
    }
    timesheetPlaybackRangeStartFrame_ = startT - 1;
    timesheetPlaybackRangeEndFrame_ = endT - 1;
    normalizeTimesheetPlaybackRange(timesheetPanelData);
    setTimesheetPreviewFrame(timesheetPanelData, timesheetPlaybackRangeStartFrame_, reason);
    lastMessage_ = std::string(reason) + " T" + std::to_string(timesheetPlaybackRangeStartFrame_ + 1) +
        "-T" + std::to_string(timesheetPlaybackRangeEndFrame_ + 1);
}

void App::stepTimesheetRangePreviewFrame(const ui::TimesheetPanelViewModel& timesheetPanelData, int delta)
{
    normalizeTimesheetPlaybackRange(timesheetPanelData);
    const TimesheetPlaybackStep step = app_drawing::stepTimesheetRangePreviewFrameIndex(
        timesheetPanelState_.selectedTimelineFrame,
        delta,
        TimesheetPlaybackRange{timesheetPlaybackRangeStartFrame_, timesheetPlaybackRangeEndFrame_},
        timesheetPlaybackPingPong_,
        timesheetPlaybackDirection_);
    timesheetPlaybackDirection_ = step.direction;
    setTimesheetPreviewFrame(timesheetPanelData, step.requestedFrame, "timesheet range playback");
}

} // namespace perapera
