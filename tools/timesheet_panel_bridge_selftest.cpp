#include "ui/panels/TimesheetPanelBridge.h"

#include <cstdlib>
#include <iostream>
#include <string>

namespace {

void require(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(1);
    }
}

const perapera::TimesheetCellTrack* findTrack(const perapera::Timesheet& timesheet, const std::string& cellId)
{
    return perapera::findTimesheetTrack(timesheet, cellId);
}

} // namespace

int main()
{
    using namespace perapera;
    using namespace perapera::ui;

    TimesheetPanelViewModel view;
    view.totalFrames = 12;
    view.fps = 24;
    view.cells.push_back(TimesheetPanelCellColumn{"char_a", "キャラA", "character"});
    view.cells.push_back(TimesheetPanelCellColumn{"char_b", "キャラB", "character"});

    TimesheetPanelState state;
    state.entries.push_back(TimesheetPanelEditableEntry{0, "char_a", TimesheetPanelEntryKind::Key, 1});
    state.entries.push_back(TimesheetPanelEditableEntry{1, "char_a", TimesheetPanelEntryKind::Hold, 999});
    state.entries.push_back(TimesheetPanelEditableEntry{2, "char_b", TimesheetPanelEntryKind::Null, 999});
    state.entries.push_back(TimesheetPanelEditableEntry{3, "char_a", TimesheetPanelEntryKind::Drawing, 5});
    state.entries.push_back(TimesheetPanelEditableEntry{4, "char_b", TimesheetPanelEntryKind::Inbetween, 3});
    state.entries.push_back(TimesheetPanelEditableEntry{5, "char_a", TimesheetPanelEntryKind::Empty, 7});
    state.entries.push_back(TimesheetPanelEditableEntry{6, "unknown", TimesheetPanelEntryKind::Drawing, 2});
    state.entries.push_back(TimesheetPanelEditableEntry{20, "char_a", TimesheetPanelEntryKind::Drawing, 9});

    const Timesheet timesheet = buildTimesheetFromPanelState(view, state);
    require(timesheet.totalFrames == 12, "total frame count should be preserved");
    require(timesheet.tracks.size() == 2, "tracks should match known panel cells");

    const TimesheetCellTrack* trackA = findTrack(timesheet, "char_a");
    const TimesheetCellTrack* trackB = findTrack(timesheet, "char_b");
    require(trackA != nullptr, "char_a track should exist");
    require(trackB != nullptr, "char_b track should exist");
    require(trackA->displayName == "キャラA", "track display name should be copied");
    require(trackA->entries.size() == 3, "char_a should keep key, hold, drawing entries only");
    require(trackB->entries.size() == 2, "char_b should keep null and inbetween entries");

    require(trackA->entries[0].timelineFrame == 1, "panel T0 should become core T1");
    require(trackA->entries[0].type == TimesheetEntryType::Key, "key kind should convert to core key");
    require(trackA->entries[0].drawingFrameNumber == 1, "key drawing number should be preserved");

    require(trackA->entries[1].timelineFrame == 2, "hold timeline should be converted");
    require(trackA->entries[1].type == TimesheetEntryType::Hold, "hold kind should convert to core hold");
    require(trackA->entries[1].drawingFrameNumber == 0, "hold drawing number should be cleared");

    require(trackB->entries[0].timelineFrame == 3, "null timeline should be converted");
    require(trackB->entries[0].type == TimesheetEntryType::Null, "null kind should convert to core null");
    require(trackB->entries[0].drawingFrameNumber == 0, "null drawing number should be cleared");

    require(trackB->entries[1].type == TimesheetEntryType::Inbetween, "inbetween kind should convert");
    require(trackB->entries[1].drawingFrameNumber == 3, "inbetween drawing number should be preserved");

    TimesheetPanelState dirtyState;
    dirtyState.selectedTimelineFrame = 99;
    dirtyState.selectedCellColumn = 99;
    dirtyState.editDrawingFrameNumber = 0;
    dirtyState.entries.push_back(TimesheetPanelEditableEntry{0, "char_a", TimesheetPanelEntryKind::Drawing, 0});
    dirtyState.entries.push_back(TimesheetPanelEditableEntry{0, "char_a", TimesheetPanelEntryKind::Key, 4});
    dirtyState.entries.push_back(TimesheetPanelEditableEntry{20, "char_a", TimesheetPanelEntryKind::Drawing, 2});
    dirtyState.entries.push_back(TimesheetPanelEditableEntry{1, "missing", TimesheetPanelEntryKind::Drawing, 2});
    dirtyState.entries.push_back(TimesheetPanelEditableEntry{2, "char_b", TimesheetPanelEntryKind::Empty, 2});
    dirtyState.entries.push_back(TimesheetPanelEditableEntry{3, "char_b", TimesheetPanelEntryKind::Hold, 8});

    const bool normalized = normalizeTimesheetPanelStateForViewModel(view, dirtyState);
    require(normalized, "dirty panel state should report normalization");
    require(dirtyState.selectedTimelineFrame == 11, "selected timeline should clamp to last frame");
    require(dirtyState.selectedCellColumn == 1, "selected cell column should clamp to last cell");
    require(dirtyState.editDrawingFrameNumber == 1, "edit drawing number should clamp to one");
    require(dirtyState.entries.size() == 2, "normalization should drop unknown, out-of-range and empty entries");
    require(dirtyState.entries[0].timelineFrame == 0, "duplicate entry should remain on T0");
    require(dirtyState.entries[0].cellId == "char_a", "duplicate entry should keep char_a");
    require(dirtyState.entries[0].kind == TimesheetPanelEntryKind::Key, "later duplicate entry should win");
    require(dirtyState.entries[0].drawingFrameNumber == 4, "later duplicate drawing number should win");
    require(dirtyState.entries[1].cellId == "char_b", "hold entry should remain for known cell");
    require(dirtyState.entries[1].drawingFrameNumber == 0, "hold drawing number should be cleared");

    TimesheetPanelState restored;
    replacePanelEntriesFromTimesheet(timesheet, restored);
    require(restored.entries.size() == 5, "restored panel entries should round-trip non-empty known entries");
    require(restored.entries[0].timelineFrame == 0, "core T1 should become panel T0");
    require(restored.entries[0].kind == TimesheetPanelEntryKind::Key, "core key should restore to panel key");
    require(restored.entries[1].kind == TimesheetPanelEntryKind::Hold, "core hold should restore to panel hold");
    require(restored.entries[2].kind == TimesheetPanelEntryKind::Drawing, "core drawing should restore to panel drawing");

    std::cout << "Timesheet panel bridge self-test passed\n";
    return 0;
}
