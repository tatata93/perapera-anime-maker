#include "ui/AppDrawingModeTimesheet.h"

#include <iostream>
#include <stdexcept>
#include <vector>

using perapera::Cell;
using perapera::Frame;
using perapera::Project;
using perapera::Timesheet;
using perapera::TimesheetCellTrack;
using perapera::TimesheetEntry;
using perapera::TimesheetEntryType;

namespace {

void require(bool condition, const char* message)
{
    if (!condition) {
        throw std::runtime_error(message);
    }
}

Cell makeCell(const std::string& id, int frameCount)
{
    Cell cell;
    cell.id = id;
    cell.name = id;
    for (int frame = 1; frame <= frameCount; ++frame) {
        cell.frames.push_back(Frame::createDefault(frame));
    }
    return cell;
}

Timesheet makeTimesheet()
{
    Timesheet timesheet;
    TimesheetCellTrack track;
    track.cellId = "cell_a";
    track.entries.push_back(TimesheetEntry{3, TimesheetEntryType::Drawing, 2});
    track.entries.push_back(TimesheetEntry{1, TimesheetEntryType::Drawing, 1});
    track.entries.push_back(TimesheetEntry{2, TimesheetEntryType::Hold, 0});
    track.entries.push_back(TimesheetEntry{4, TimesheetEntryType::Key, 2});
    track.entries.push_back(TimesheetEntry{5, TimesheetEntryType::Inbetween, 3});
    timesheet.tracks.push_back(track);
    return timesheet;
}

void testCountsAndSelection()
{
    const Timesheet timesheet = makeTimesheet();
    require(perapera::app_drawing::countTimesheetEntries(timesheet) == 5,
            "countTimesheetEntries should count every track entry");
    require(perapera::app_drawing::timesheetPanelKindUsesDrawingFrame(perapera::ui::TimesheetPanelEntryKind::Drawing),
            "drawing panel entries should use drawing frames");
    require(!perapera::app_drawing::timesheetPanelKindUsesDrawingFrame(perapera::ui::TimesheetPanelEntryKind::Hold),
            "hold panel entries should not use drawing frames");

    perapera::ui::TimesheetPanelViewModel data;
    data.cells.push_back(perapera::ui::TimesheetPanelCellColumn{"cell_a", "A", ""});
    data.cells.push_back(perapera::ui::TimesheetPanelCellColumn{"cell_b", "B", ""});

    perapera::ui::TimesheetPanelState state;
    state.selectedCellColumn = 9;
    state.selectedTimelineFrame = 12;
    state.entries.push_back(perapera::ui::TimesheetPanelEditableEntry{
        12, "cell_b", perapera::ui::TimesheetPanelEntryKind::Key, 4});

    const perapera::ui::TimesheetPanelEditableEntry* selected =
        perapera::app_drawing::selectedTimesheetPanelEntry(data, state);
    require(selected != nullptr, "selectedTimesheetPanelEntry should clamp selected column");
    require(selected->cellId == "cell_b", "selectedTimesheetPanelEntry should match clamped cell");
}

void testProjectLookupAndAutoCreate()
{
    Project project;
    project.cells.push_back(makeCell("cell_a", 1));
    project.cells.push_back(makeCell("cell_b", 2));

    int cellIndex = -1;
    Cell* found = perapera::app_drawing::projectCellById(project, "cell_b", &cellIndex);
    require(found == &project.cells[1], "projectCellById should find requested cell");
    require(cellIndex == 1, "projectCellById should return cell index");

    perapera::ui::TimesheetPanelState state;
    state.entries.push_back(perapera::ui::TimesheetPanelEditableEntry{
        0, "cell_a", perapera::ui::TimesheetPanelEntryKind::Drawing, 3});
    state.entries.push_back(perapera::ui::TimesheetPanelEditableEntry{
        0, "cell_a", perapera::ui::TimesheetPanelEntryKind::Hold, 8});
    state.entries.push_back(perapera::ui::TimesheetPanelEditableEntry{
        0, "cell_b", perapera::ui::TimesheetPanelEntryKind::Key, 241});

    const int created = perapera::app_drawing::autoCreateMissingDrawingFramesForTimesheetEntries(project, state);
    require(created == 2, "autoCreate should create only missing valid drawing frames");
    require(project.cells[0].frames.size() == 3U, "autoCreate should extend target cell frames");
    require(project.cells[1].frames.size() == 2U, "autoCreate should ignore over-limit drawing frame numbers");
}

void testPlaybackOrder()
{
    const Timesheet timesheet = makeTimesheet();
    const Cell cell = makeCell("cell_a", 3);
    const std::vector<int> order =
        perapera::app_drawing::buildTimesheetPlaybackOrderFrameIndicesForCell(timesheet, cell);

    require(order.size() == 3U, "playback order should include unique drawable frame indices");
    require(order[0] == 0 && order[1] == 1 && order[2] == 2,
            "playback order should follow sorted timesheet timing");
    require(perapera::app_drawing::adjacentPlaybackOrderFrameIndex(order, 1, -1) == 0,
            "previous playback frame should be found");
    require(perapera::app_drawing::adjacentPlaybackOrderFrameIndex(order, 1, 1) == 2,
            "next playback frame should be found");
    require(perapera::app_drawing::adjacentPlaybackOrderFrameIndex(order, 0, -1) == -1,
            "previous at beginning should be absent");
    require(perapera::app_drawing::adjacentPlaybackOrderFrameIndex(order, 9, 1) == -1,
            "unknown active frame should be absent");
}

void testPlaybackStepping()
{
    require(perapera::app_drawing::clampTimesheetPreviewFrame(5, -4) == 0,
            "preview frame clamp should stop below zero");
    require(perapera::app_drawing::clampTimesheetPreviewFrame(5, 99) == 4,
            "preview frame clamp should stop above final frame");
    require(perapera::app_drawing::clampTimesheetPreviewFrame(0, 99) == 0,
            "preview frame clamp should tolerate empty frame counts");

    perapera::app_drawing::TimesheetPlaybackStep step =
        perapera::app_drawing::stepTimesheetPreviewFrameIndex(5, 4, 1, false, 1);
    require(step.canPlay && step.requestedFrame == 0 && step.direction == 1,
            "linear preview playback should wrap after the final frame");

    step = perapera::app_drawing::stepTimesheetPreviewFrameIndex(5, 4, 1, true, 1);
    require(step.canPlay && step.requestedFrame == 3 && step.direction == -1,
            "ping-pong preview playback should bounce at the final frame");

    step = perapera::app_drawing::stepTimesheetPreviewFrameIndex(1, 0, 1, false, 1);
    require(!step.canPlay && step.requestedFrame == 0,
            "preview playback should stop when only one frame exists");

    perapera::app_drawing::TimesheetPlaybackRange range =
        perapera::app_drawing::normalizeTimesheetPlaybackRange(10, 8, 2);
    require(range.startFrame == 2 && range.endFrame == 8,
            "playback range normalization should swap reversed ranges");

    range = perapera::app_drawing::normalizeTimesheetPlaybackRange(4, -5, 99);
    require(range.startFrame == 0 && range.endFrame == 3,
            "playback range normalization should clamp to timeline bounds");

    step = perapera::app_drawing::stepTimesheetRangePreviewFrameIndex(
        8, 1, perapera::app_drawing::TimesheetPlaybackRange{2, 8}, false, 1);
    require(step.requestedFrame == 2 && step.direction == 1,
            "range playback should wrap to range start");

    step = perapera::app_drawing::stepTimesheetRangePreviewFrameIndex(
        8, 1, perapera::app_drawing::TimesheetPlaybackRange{2, 8}, true, 1);
    require(step.requestedFrame == 7 && step.direction == -1,
            "range ping-pong playback should bounce inside range");

    step = perapera::app_drawing::stepTimesheetRangePreviewFrameIndex(
        4, 1, perapera::app_drawing::TimesheetPlaybackRange{4, 4}, true, 1);
    require(step.requestedFrame == 4 && step.direction == 1,
            "single-frame range playback should stay on the range frame");
}

} // namespace

int main()
{
    try {
        testCountsAndSelection();
        testProjectLookupAndAutoCreate();
        testPlaybackOrder();
        testPlaybackStepping();
    } catch (const std::exception& e) {
        std::cerr << "perapera_app_drawing_mode_timesheet_selftest failed: " << e.what() << '\n';
        return 1;
    }

    std::cout << "perapera_app_drawing_mode_timesheet_selftest passed\n";
    return 0;
}
