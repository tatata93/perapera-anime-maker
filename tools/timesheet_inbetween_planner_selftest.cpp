// このファイルの役割:
// TimesheetInbetweenPlanner が原画間の空きTへ中割を配置できるかを検証する。

#include "core/Timesheet.h"
#include "core/TimesheetInbetweenPlanner.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void require(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << "FAILED: " << message << "\n";
        std::exit(1);
    }
}

perapera::Timesheet makeBaseSheet()
{
    perapera::Timesheet sheet;
    sheet.totalFrames = 12;
    perapera::TimesheetCellTrack& track = perapera::ensureTimesheetTrack(sheet, "cell_a", "キャラA");

    perapera::TimesheetEntry key1;
    key1.timelineFrame = 1;
    key1.type = perapera::TimesheetEntryType::Key;
    key1.drawingFrameNumber = 1;
    track.entries.push_back(key1);

    perapera::TimesheetEntry key2;
    key2.timelineFrame = 7;
    key2.type = perapera::TimesheetEntryType::Key;
    key2.drawingFrameNumber = 2;
    track.entries.push_back(key2);

    perapera::normalizeTimesheet(sheet);
    return sheet;
}

void requireFrames(const std::vector<int>& actual, const std::vector<int>& expected, const char* message)
{
    require(actual.size() == expected.size(), message);
    for (std::size_t i = 0; i < expected.size(); ++i) {
        require(actual[i] == expected[i], message);
    }
}

} // namespace

int main()
{
    using perapera::TimesheetEntry;
    using perapera::TimesheetEntryType;

    {
        perapera::Timesheet sheet = makeBaseSheet();
        const perapera::TimesheetInbetweenPlacementPlan plan =
            perapera::planTimesheetInbetweenPlacements(sheet, "cell_a", 1, 7, 1);
        require(plan.ok, "one inbetween plan should be OK");
        requireFrames(plan.timelineFrames, {4}, "one inbetween should use middle T4");
    }

    {
        perapera::Timesheet sheet = makeBaseSheet();
        const perapera::TimesheetInbetweenPlacementPlan plan =
            perapera::planTimesheetInbetweenPlacements(sheet, "cell_a", 1, 7, 2);
        require(plan.ok, "two inbetween plan should be OK");
        requireFrames(plan.timelineFrames, {3, 5}, "two inbetweens should use T3/T5");
    }

    {
        perapera::Timesheet sheet = makeBaseSheet();
        const perapera::TimesheetInbetweenPlacementPlan plan =
            perapera::planTimesheetInbetweenPlacementsForKomaStep(sheet, "cell_a", 1, 7, 2);
        require(plan.ok, "2-koma inbetween plan should be OK");
        require(plan.komaStep == 2, "2-koma plan should preserve koma step");
        requireFrames(plan.timelineFrames, {3, 5}, "2-koma plan should use T3/T5");
    }

    {
        perapera::Timesheet sheet = makeBaseSheet();
        const perapera::TimesheetInbetweenPlacementPlan plan =
            perapera::planTimesheetInbetweenPlacementsForKomaStep(sheet, "cell_a", 1, 7, 3);
        require(plan.ok, "3-koma inbetween plan should be OK");
        requireFrames(plan.timelineFrames, {4}, "3-koma plan should use T4");
    }

    {
        perapera::Timesheet sheet = makeBaseSheet();
        const perapera::TimesheetInbetweenPlacementPlan plan =
            perapera::planTimesheetInbetweenPlacementsForKomaStep(sheet, "cell_a", 1, 7, 1);
        require(plan.ok, "1-koma inbetween plan should be OK");
        requireFrames(plan.timelineFrames, {2, 3, 4, 5, 6}, "1-koma plan should fill every T slot");
    }

    {
        perapera::Timesheet sheet = makeBaseSheet();
        const perapera::TimesheetInbetweenPlacementPlan plan =
            perapera::planTimesheetInbetweenPlacements(sheet, "cell_a", 1, 7, 3);
        require(plan.ok, "three inbetween plan should be OK");
        requireFrames(plan.timelineFrames, {3, 4, 6}, "three inbetweens should use stable rounded positions");
    }

    {
        perapera::Timesheet sheet = makeBaseSheet();
        perapera::TimesheetCellTrack* track = perapera::findTimesheetTrack(sheet, "cell_a");
        require(track != nullptr, "track should exist");
        TimesheetEntry existing;
        existing.timelineFrame = 3;
        existing.type = TimesheetEntryType::Inbetween;
        existing.drawingFrameNumber = 3;
        track->entries.push_back(existing);
        perapera::normalizeTimesheet(sheet);

        const perapera::TimesheetInbetweenPlacementPlan plan =
            perapera::planTimesheetInbetweenPlacements(sheet, "cell_a", 1, 7, 2);
        require(plan.ok, "planner should avoid occupied T and still place if enough empty slots exist");
        require(plan.timelineFrames.size() == 2U, "occupied avoidance should still return two slots");
        require(plan.timelineFrames[0] != 3 && plan.timelineFrames[1] != 3, "occupied T3 must not be reused");
    }

    {
        perapera::Timesheet sheet = makeBaseSheet();
        perapera::TimesheetCellTrack* track = perapera::findTimesheetTrack(sheet, "cell_a");
        require(track != nullptr, "track should exist");
        TimesheetEntry existing;
        existing.timelineFrame = 3;
        existing.type = TimesheetEntryType::Inbetween;
        existing.drawingFrameNumber = 3;
        track->entries.push_back(existing);
        perapera::normalizeTimesheet(sheet);

        const perapera::TimesheetInbetweenPlacementPlan plan =
            perapera::planTimesheetInbetweenPlacementsForKomaStep(sheet, "cell_a", 1, 7, 2);
        require(plan.ok, "2-koma planner should add the missing target if one target is occupied");
        requireFrames(plan.timelineFrames, {5}, "2-koma planner should skip occupied T3 and add T5 only");
    }

    {
        perapera::Timesheet sheet = makeBaseSheet();
        const perapera::TimesheetInbetweenPlacementPlan plan =
            perapera::planTimesheetInbetweenPlacements(sheet, "cell_a", 1, 7, 6);
        require(!plan.ok, "requesting more inbetweens than empty T slots should fail");
    }

    std::cout << "Timesheet inbetween planner self-test passed\n";
    return 0;
}
