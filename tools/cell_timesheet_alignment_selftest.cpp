// This file's role: self-test that normal Cell categories remain ordinary Timesheet tracks.
// final_spec_v6 Phase 2-pre Step T2-d.
#include "core/Cell.h"
#include "core/Project.h"
#include "core/Timesheet.h"

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

namespace {

struct SeedCell {
    const char* id;
    const char* name;
    const char* category;
};

bool fail(const std::string& message)
{
    std::cerr << "cell_timesheet_alignment_selftest failed: " << message << "\n";
    return false;
}

perapera::Cell makeCell(const SeedCell& seed, int zOrder)
{
    perapera::Cell cell;
    cell.id = seed.id;
    cell.name = seed.name;
    cell.category = seed.category;
    cell.visible = true;
    cell.opacity = 1.0f;
    cell.zOrder = zOrder;
    return cell;
}

bool containsTrackFor(const perapera::Timesheet& timesheet, const std::string& cellId)
{
    return perapera::findTimesheetTrack(timesheet, cellId) != nullptr;
}

} // namespace

int main()
{
    using namespace perapera;

    const std::vector<SeedCell> seeds{
        {"cell_A", "A", "character"},
        {"cell_BG", "BG", "background"},
        {"cell_layout", "Layout", "layout"},
        {"cell_book", "BOOK", "book"},
        {"cell_fx", "FX", "effect"},
        {"cell_ref", "Reference", "reference"},
    };

    Project project;
    project.cells.clear();
    project.cellOrder.clear();

    for (int index = 0; index < static_cast<int>(seeds.size()); ++index) {
        project.cells.push_back(makeCell(seeds[static_cast<std::size_t>(index)], index));
        project.cellOrder.push_back(seeds[static_cast<std::size_t>(index)].id);
    }

    Timesheet timesheet;
    timesheet.totalFrames = 24;
    timesheet.defaultExposure = 1;
    timesheet.tracks.clear();

    for (const Cell& cell : project.cells) {
        TimesheetCellTrack& track = ensureTimesheetTrack(timesheet, cell.id, cell.name);
        track.defaultExposure = 1;
        track.entries.push_back(TimesheetEntry{1, TimesheetEntryType::Key, 1, cell.category});
    }

    normalizeTimesheet(timesheet);

    if (timesheet.tracks.size() != project.cells.size()) {
        return fail("track count does not match project cell count") ? 0 : 1;
    }

    for (std::size_t index = 0; index < project.cells.size(); ++index) {
        const Cell& cell = project.cells[index];
        const TimesheetCellTrack& track = timesheet.tracks[index];

        if (track.cellId != cell.id) {
            return fail("track order/cellId mismatch at index " + std::to_string(index)) ? 0 : 1;
        }

        if (track.displayName != cell.name) {
            return fail("track displayName mismatch for " + cell.id) ? 0 : 1;
        }

        if (!containsTrackFor(timesheet, cell.id)) {
            return fail("findTimesheetTrack could not find " + cell.id) ? 0 : 1;
        }

        if (track.entries.empty()) {
            return fail("track has no entries for " + cell.id) ? 0 : 1;
        }

        if (track.entries.front().timelineFrame != 1) {
            return fail("entry timelineFrame should remain 1-based for " + cell.id) ? 0 : 1;
        }
    }

    TimesheetCellTrack& bgTrack = ensureTimesheetTrack(timesheet, "cell_BG", "BG renamed safely");
    if (bgTrack.cellId != "cell_BG") {
        return fail("ensureTimesheetTrack should reuse an existing BG track") ? 0 : 1;
    }
    if (timesheet.tracks.size() != project.cells.size()) {
        return fail("ensureTimesheetTrack duplicated an existing BG track") ? 0 : 1;
    }

    std::cout << "cell_timesheet_alignment_selftest OK: normal cell categories remain normal Timesheet tracks.\n";
    return 0;
}
