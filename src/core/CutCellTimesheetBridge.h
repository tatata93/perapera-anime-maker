#pragma once

// Role:
// Phase 2 Step 1-d migration helper.
// Keep normal Cells and Cut-owned Timesheet tracks aligned while the project
// is still using the legacy Project.cells / Project.cellOrder storage path.
//
// Rules:
// - Do not filter by Cell category. character/background/layout/book/effect/reference
//   are all normal cells and may have timing in the Timesheet.
// - Do not change the physical save format in this helper.
// - Do not reintroduce Scene Plate or a scene-management panel.

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

#include "core/Project.h"
#include "core/ProjectActiveCutBridge.h"
#include "core/Timesheet.h"

namespace perapera {

struct CutCellTimesheetBridgeReport {
    std::size_t cellCount = 0;
    std::size_t trackCountBefore = 0;
    std::size_t trackCountAfter = 0;
    std::size_t createdTrackCount = 0;
    std::size_t orderedCellCount = 0;
    std::vector<std::string> ensuredCellIds;
};

[[nodiscard]] inline bool isTimelineManagedCellCategory(std::string_view category) noexcept
{
    return category == "character" || category == "background" || category == "layout" ||
        category == "book" || category == "effect" || category == "reference" || category.empty();
}

namespace detail {

[[nodiscard]] inline bool containsString(const std::vector<std::string>& values, const std::string& value)
{
    for (const std::string& current : values) {
        if (current == value) {
            return true;
        }
    }
    return false;
}

inline void appendActiveCutCellIdIfPresent(
    const ConstActiveCutCellsView& activeCells,
    std::vector<std::string>& result,
    const std::string& cellId)
{
    if (cellId.empty() || containsString(result, cellId)) {
        return;
    }

    const Cell* cell = activeCells.cellById(cellId);
    if (cell == nullptr) {
        return;
    }

    result.push_back(cellId);
}

[[nodiscard]] inline std::vector<std::string> orderedActiveCutCellIds(const Project& project)
{
    std::vector<std::string> result;
    const ConstActiveCutCellsView activeCells = activeCutCells(project);
    if (!activeCells.valid() || activeCells.cells == nullptr || activeCells.cellOrder == nullptr) {
        return result;
    }

    result.reserve(activeCells.cells->size());

    for (const std::string& cellId : *activeCells.cellOrder) {
        appendActiveCutCellIdIfPresent(activeCells, result, cellId);
    }

    for (const Cell& cell : *activeCells.cells) {
        appendActiveCutCellIdIfPresent(activeCells, result, cell.id);
    }

    return result;
}

} // namespace detail

[[nodiscard]] inline std::vector<std::string> activeCutCellIdsForTimesheet(const Project& project)
{
    return detail::orderedActiveCutCellIds(project);
}

inline CutCellTimesheetBridgeReport ensureTimesheetTracksForActiveCutCells(
    const Project& project,
    Timesheet& timesheet)
{
    CutCellTimesheetBridgeReport report;
    report.trackCountBefore = timesheet.tracks.size();

    const ConstActiveCutCellsView activeCells = activeCutCells(project);
    if (!activeCells.valid() || activeCells.cells == nullptr) {
        report.trackCountAfter = timesheet.tracks.size();
        return report;
    }

    report.cellCount = activeCells.cells->size();
    const std::vector<std::string> orderedCellIds = activeCutCellIdsForTimesheet(project);
    report.orderedCellCount = orderedCellIds.size();

    for (const std::string& cellId : orderedCellIds) {
        const Cell* cell = activeCells.cellById(cellId);
        if (cell == nullptr || cell->id.empty()) {
            continue;
        }

        if (!isTimelineManagedCellCategory(cell->category)) {
            continue;
        }

        const bool existed = findTimesheetTrack(timesheet, cell->id) != nullptr;
        TimesheetCellTrack& track = ensureTimesheetTrack(timesheet, cell->id, cell->name);
        if (track.displayName.empty()) {
            track.displayName = cell->name;
        }

        if (!existed) {
            ++report.createdTrackCount;
        }
        report.ensuredCellIds.push_back(cell->id);
    }

    report.trackCountAfter = timesheet.tracks.size();
    return report;
}

} // namespace perapera
