#include "core/Cell.h"
#include "core/Project.h"
#include "core/ProjectStructureStatus.h"
#include "core/Timesheet.h"

#include <cstdlib>
#include <iostream>
#include <string>

namespace {

perapera::Cell makeCell(std::string id, std::string name, std::string category)
{
    perapera::Cell cell;
    cell.id = std::move(id);
    cell.name = std::move(name);
    cell.category = std::move(category);
    return cell;
}

int countCategory(const perapera::ProjectStructureStatus& status, const std::string& category)
{
    for (const perapera::ProjectStructureCategoryCount& count : status.categoryCounts) {
        if (count.category == category) {
            return count.count;
        }
    }
    return 0;
}

void requireTrue(bool condition, const char* message)
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
    project.timeline.totalFrames = 96;
    project.cells.push_back(makeCell("cell_A", "A", "character"));
    project.cells.push_back(makeCell("cell_BG", "BG", "background"));
    project.cells.push_back(makeCell("cell_layout", "Layout", "layout"));
    project.cellOrder = {"cell_A", "cell_BG", "cell_layout"};

    perapera::Timesheet timesheet;
    timesheet.totalFrames = 48;
    perapera::TimesheetCellTrack trackA;
    trackA.cellId = "cell_A";
    timesheet.tracks.push_back(trackA);
    perapera::TimesheetCellTrack trackBg;
    trackBg.cellId = "cell_BG";
    timesheet.tracks.push_back(trackBg);

    const perapera::ProjectStructureStatus status = perapera::buildProjectStructureStatus(project, timesheet);

    requireTrue(status.sceneId == "scene_001", "compat scene id must be scene_001");
    requireTrue(status.cutId == "cut_001", "compat cut id must be cut_001");
    requireTrue(status.compatibilityBridge, "status must mark compatibility bridge");
    requireTrue(status.activeCutCellCount == 3, "active Cut cell count should mirror Project.cells");
    requireTrue(status.orderedCellCount == 3, "cell order count should mirror Project.cellOrder");
    requireTrue(status.timesheetTrackCount == 2, "timesheet track count should be visible");
    requireTrue(status.totalFrames == 48, "timesheet total frames should take priority");
    requireTrue(countCategory(status, "character") == 1, "character count should be 1");
    requireTrue(countCategory(status, "background") == 1, "background count should be 1");
    requireTrue(countCategory(status, "layout") == 1, "layout count should be 1");

    perapera::Timesheet emptyTimesheet;
    emptyTimesheet.totalFrames = 0;
    const perapera::ProjectStructureStatus fallbackStatus = perapera::buildProjectStructureStatus(project, emptyTimesheet);
    requireTrue(fallbackStatus.totalFrames == 96, "project timeline should be used when timesheet total frames is unset");

    std::cout << "project_structure_status_selftest passed\n";
    return 0;
}
