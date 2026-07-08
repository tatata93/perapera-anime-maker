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

std::vector<int> buildTimesheetPlaybackOrderFrameIndicesForCell(const Timesheet& timesheet, const Cell& cell);

int adjacentPlaybackOrderFrameIndex(const std::vector<int>& playbackOrderFrameIndices,
                                    int activeFrameIndex,
                                    int direction);

ui::TimesheetPanelViewModel buildTimesheetPanelViewModel(const Project& project);

} // namespace perapera::app_drawing
