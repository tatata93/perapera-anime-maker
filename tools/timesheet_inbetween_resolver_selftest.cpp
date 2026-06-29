#include "core/TimesheetInbetweenResolver.h"

#include <cstdlib>
#include <iostream>
#include <string>

using namespace perapera;

namespace {

void require(bool condition, const std::string& message)
{
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(1);
    }
}

Timesheet makeSampleTimesheet()
{
    Timesheet timesheet;
    timesheet.totalFrames = 12;
    TimesheetCellTrack& track = ensureTimesheetTrack(timesheet, "cell_a", "Cell A");
    track.entries.push_back(TimesheetEntry{1, TimesheetEntryType::Key, 1, ""});
    track.entries.push_back(TimesheetEntry{3, TimesheetEntryType::Inbetween, 3, ""});
    track.entries.push_back(TimesheetEntry{5, TimesheetEntryType::Inbetween, 4, ""});
    track.entries.push_back(TimesheetEntry{7, TimesheetEntryType::Key, 2, ""});
    track.entries.push_back(TimesheetEntry{10, TimesheetEntryType::Key, 5, ""});
    normalizeTimesheet(timesheet);
    return timesheet;
}

} // namespace

int main()
{
    const Timesheet timesheet = makeSampleTimesheet();

    const ResolvedTimesheetInbetweenInterval middle =
        resolveTimesheetInbetweenInterval(timesheet, "cell_a", 4);
    require(middle.hasClosedKeyInterval, "T4 should have a closed key interval");
    require(middle.previousKey.timelineFrame == 1, "T4 previous key T should be 1");
    require(middle.previousKey.drawingFrameNumber == 1, "T4 previous key drawing should be F1");
    require(middle.nextKey.timelineFrame == 7, "T4 next key T should be 7");
    require(middle.nextKey.drawingFrameNumber == 2, "T4 next key drawing should be F2");
    require(middle.inbetweens.size() == 2U, "T4 should see two inbetweens");
    require(middle.inbetweens[0].timelineFrame == 3, "first inbetween should be at T3");
    require(middle.inbetweens[0].inbetweenIndex == 1, "first inbetween index should be 1");
    require(middle.inbetweens[0].inbetweenCount == 2, "inbetween count should be 2");
    require(middle.selectedPositionNumerator == 3, "T4 position numerator should be 3");
    require(middle.selectedPositionDenominator == 6, "T4 position denominator should be 6");
    require(middle.availableTimelineSlotCount == 5, "T1->T7 should have 5 inner slots");

    const ResolvedTimesheetInbetweenInterval selectedInbetween =
        resolveTimesheetInbetweenInterval(timesheet, "cell_a", 3);
    require(selectedInbetween.selectedIsInbetween, "T3 should be recognized as inbetween");
    require(!selectedInbetween.selectedIsKey, "T3 should not be key");

    const ResolvedTimesheetInbetweenInterval selectedKey =
        resolveTimesheetInbetweenInterval(timesheet, "cell_a", 7);
    require(selectedKey.selectedIsKey, "T7 should be key");
    require(selectedKey.previousKey.timelineFrame == 7, "T7 should start the next interval when a next key exists");
    require(selectedKey.nextKey.timelineFrame == 10, "T7 next key should be T10");

    const ResolvedTimesheetInbetweenInterval afterLast =
        resolveTimesheetInbetweenInterval(timesheet, "cell_a", 11);
    require(!afterLast.hasClosedKeyInterval, "T11 should not have a following key");
    require(afterLast.previousKey.timelineFrame == 10, "T11 previous key should be T10");
    require(!afterLast.nextKey.found, "T11 should have no next key");

    const ResolvedTimesheetInbetweenInterval missingCell =
        resolveTimesheetInbetweenInterval(timesheet, "missing", 4);
    require(!missingCell.hasClosedKeyInterval, "missing cell should not resolve interval");

    std::cout << "Timesheet inbetween resolver self-test passed\n";
    return 0;
}
