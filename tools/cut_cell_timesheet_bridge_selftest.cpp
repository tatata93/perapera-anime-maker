#include "core/CutCellTimesheetBridge.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

perapera::Cell makeCell(const std::string& id, const std::string& name, const std::string& category)
{
    perapera::Cell cell;
    cell.id = id;
    cell.name = name;
    cell.category = category;
    return cell;
}

void require(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(1);
    }
}

} // namespace

int main()
{
    perapera::Project project;
    project.cells.clear();
    project.cellOrder.clear();

    const std::vector<perapera::Cell> cells = {
        makeCell("cell_A", "A", "character"),
        makeCell("cell_BG", "BG", "background"),
        makeCell("cell_LO", "Layout", "layout"),
        makeCell("cell_BOOK", "BOOK", "book"),
        makeCell("cell_FX", "FX", "effect"),
        makeCell("cell_REF", "Reference", "reference"),
    };

    for (const perapera::Cell& cell : cells) {
        project.cells.push_back(cell);
    }

    project.cellOrder = {
        "cell_BG",
        "cell_A",
        "cell_LO",
        "cell_BOOK",
        "cell_FX",
        "cell_REF",
    };

    perapera::Timesheet timesheet;
    timesheet.totalFrames = 48;

    perapera::TimesheetCellTrack& preexistingBg =
        perapera::ensureTimesheetTrack(timesheet, "cell_BG", "Existing BG");
    preexistingBg.entries.push_back(perapera::TimesheetEntry{
        1,
        perapera::TimesheetEntryType::Drawing,
        1,
        "bg start",
    });

    const perapera::CutCellTimesheetBridgeReport report =
        perapera::ensureTimesheetTracksForActiveCutCells(project, timesheet);

    require(report.cellCount == 6U, "active cut bridge should expose 6 cells");
    require(report.orderedCellCount == 6U, "all active cut cells should be order-resolved");
    require(report.trackCountBefore == 1U, "one preexisting BG track expected");
    require(report.trackCountAfter == 6U, "all cells should have a timesheet track");
    require(report.createdTrackCount == 5U, "existing BG track should not be duplicated");
    require(report.ensuredCellIds.size() == 6U, "all normal categories should be ensured");

    const std::vector<std::string> orderedIds = perapera::activeCutCellIdsForTimesheet(project);
    require(orderedIds.size() == 6U, "ordered ids should include 6 cells");
    require(orderedIds[0] == "cell_BG", "timesheet order should follow Project.cellOrder first");
    require(orderedIds[1] == "cell_A", "character cell should remain in order");

    for (const perapera::Cell& cell : cells) {
        require(
            perapera::isTimelineManagedCellCategory(cell.category),
            "category should be timeline-managed");
        const perapera::TimesheetCellTrack* track = perapera::findTimesheetTrack(timesheet, cell.id);
        require(track != nullptr, "cell should have a timesheet track");
        require(track->cellId == cell.id, "track cellId should match cell id");
    }

    const perapera::TimesheetCellTrack* bgTrack = perapera::findTimesheetTrack(timesheet, "cell_BG");
    require(bgTrack != nullptr, "BG track should exist");
    require(bgTrack->displayName == "Existing BG", "existing display name should be preserved");
    require(bgTrack->entries.size() == 1U, "existing BG entries should be preserved");

    const perapera::TimesheetCellTrack* layoutTrack = perapera::findTimesheetTrack(timesheet, "cell_LO");
    require(layoutTrack != nullptr, "Layout track should exist");
    require(layoutTrack->displayName == "Layout", "new track should use cell name");

    std::cout << "perapera_cut_cell_timesheet_bridge_selftest passed\n";
    return 0;
}
