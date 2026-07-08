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
using app_drawing::timesheetPanelKindUsesDrawingFrame;

void App::setTimesheetPreviewFrame(const ui::TimesheetPanelViewModel& timesheetPanelData,
                                   int zeroBasedFrame,
                                   const char* reason)
{
    const int lastFrame = std::max(0, timesheetPanelData.totalFrames - 1);
    const int nextFrame = std::clamp(zeroBasedFrame, 0, lastFrame);
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
    const int frameCount = std::max(1, timesheetPanelData.totalFrames);
    if (frameCount <= 1) {
        isPlayingTimesheet_ = false;
        timesheetPlaybackAccumulator_ = 0.0f;
        return;
    }

    int nextFrame = timesheetPanelState_.selectedTimelineFrame + delta;
    if (nextFrame < 0 || nextFrame >= frameCount) {
        if (timesheetPlaybackPingPong_) {
            timesheetPlaybackDirection_ = -timesheetPlaybackDirection_;
            nextFrame = timesheetPanelState_.selectedTimelineFrame + timesheetPlaybackDirection_;
        } else {
            nextFrame = nextFrame >= frameCount ? 0 : frameCount - 1;
        }
    }
    setTimesheetPreviewFrame(timesheetPanelData, nextFrame, "timesheet playback");
}

void App::normalizeTimesheetPlaybackRange(const ui::TimesheetPanelViewModel& timesheetPanelData)
{
    const int lastFrame = std::max(0, timesheetPanelData.totalFrames - 1);
    timesheetPlaybackRangeStartFrame_ = std::clamp(timesheetPlaybackRangeStartFrame_, 0, lastFrame);
    timesheetPlaybackRangeEndFrame_ = std::clamp(timesheetPlaybackRangeEndFrame_, 0, lastFrame);
    if (timesheetPlaybackRangeStartFrame_ > timesheetPlaybackRangeEndFrame_) {
        std::swap(timesheetPlaybackRangeStartFrame_, timesheetPlaybackRangeEndFrame_);
    }
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
    const int rangeStart = timesheetPlaybackRangeStartFrame_;
    const int rangeEnd = timesheetPlaybackRangeEndFrame_;
    if (rangeEnd <= rangeStart) {
        setTimesheetPreviewFrame(timesheetPanelData, rangeStart, "timesheet range playback");
        return;
    }

    int nextFrame = timesheetPanelState_.selectedTimelineFrame + delta;
    if (nextFrame < rangeStart || nextFrame > rangeEnd) {
        if (timesheetPlaybackPingPong_) {
            timesheetPlaybackDirection_ = -timesheetPlaybackDirection_;
            nextFrame = timesheetPanelState_.selectedTimelineFrame + timesheetPlaybackDirection_;
            nextFrame = std::clamp(nextFrame, rangeStart, rangeEnd);
        } else {
            nextFrame = delta >= 0 ? rangeStart : rangeEnd;
        }
    }
    setTimesheetPreviewFrame(timesheetPanelData, nextFrame, "timesheet range playback");
}

} // namespace perapera
