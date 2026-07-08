#pragma once

#include <string>
#include <vector>

#include "core/Cell.h"
#include "core/Project.h"
#include "core/Timesheet.h"
#include "ui/panels/TimesheetPanel.h"

namespace perapera::app_drawing {

int countTimesheetEntries(const Timesheet& timesheet);
bool timesheetPanelKindUsesDrawingFrame(ui::TimesheetPanelEntryKind kind);
Cell* projectCellById(Project& project, const std::string& cellId, int* cellIndexOut = nullptr);

const ui::TimesheetPanelEditableEntry* selectedTimesheetPanelEntry(
    const ui::TimesheetPanelViewModel& data,
    const ui::TimesheetPanelState& state);

int autoCreateMissingDrawingFramesForTimesheetEntries(
    Project& project,
    const ui::TimesheetPanelState& state);

struct TimesheetPlaybackStep {
    int requestedFrame = 0;
    int direction = 1;
    bool canPlay = true;
};

struct TimesheetPlaybackRange {
    int startFrame = 0;
    int endFrame = 0;
};

int clampTimesheetPreviewFrame(int totalFrames, int zeroBasedFrame);

TimesheetPlaybackStep stepTimesheetPreviewFrameIndex(int totalFrames,
                                                     int selectedFrame,
                                                     int delta,
                                                     bool pingPong,
                                                     int currentDirection);

TimesheetPlaybackRange normalizeTimesheetPlaybackRange(int totalFrames,
                                                       int startFrame,
                                                       int endFrame);

TimesheetPlaybackStep stepTimesheetRangePreviewFrameIndex(int selectedFrame,
                                                          int delta,
                                                          TimesheetPlaybackRange range,
                                                          bool pingPong,
                                                          int currentDirection);

std::vector<int> buildTimesheetPlaybackOrderFrameIndicesForCell(const Timesheet& timesheet, const Cell& cell);

int adjacentPlaybackOrderFrameIndex(const std::vector<int>& playbackOrderFrameIndices,
                                    int activeFrameIndex,
                                    int direction);

ui::TimesheetPanelViewModel buildTimesheetPanelViewModel(const Project& project);

} // namespace perapera::app_drawing
