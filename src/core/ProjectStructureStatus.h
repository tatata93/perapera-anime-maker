#pragma once

// final_spec_v6 Phase 2 Step 1-c:
// Lightweight status data for showing where the current compatibility project
// sits in the future Project -> Scene -> Cut -> Cell tree.
// This file is intentionally small. Do not grow large UI files with this logic.

#include <string>
#include <vector>

#include "core/Project.h"
#include "core/ProjectActiveCutBridge.h"
#include "core/Timesheet.h"

namespace perapera {

struct ProjectStructureCategoryCount {
    std::string category;
    int count = 0;
};

struct ProjectStructureStatus {
    std::string sceneId = "scene_001";
    std::string sceneName = "Scene 001";
    std::string cutId = "cut_001";
    std::string cutName = "Cut 001";
    bool compatibilityBridge = true;
    int activeCutCellCount = 0;
    int orderedCellCount = 0;
    int timesheetTrackCount = 0;
    int totalFrames = 1;
    std::vector<ProjectStructureCategoryCount> categoryCounts;
};

inline ProjectStructureCategoryCount* findProjectStructureCategoryCount(
    std::vector<ProjectStructureCategoryCount>& counts,
    const std::string& category)
{
    for (ProjectStructureCategoryCount& count : counts) {
        if (count.category == category) {
            return &count;
        }
    }
    return nullptr;
}

inline void incrementProjectStructureCategoryCount(
    std::vector<ProjectStructureCategoryCount>& counts,
    const std::string& category)
{
    const std::string safeCategory = category.empty() ? "character" : category;
    if (ProjectStructureCategoryCount* existing = findProjectStructureCategoryCount(counts, safeCategory)) {
        ++existing->count;
        return;
    }
    ProjectStructureCategoryCount created;
    created.category = safeCategory;
    created.count = 1;
    counts.push_back(created);
}

inline ProjectStructureStatus buildProjectStructureStatus(const Project& project, const Timesheet& timesheet)
{
    ProjectStructureStatus status;
    status.activeCutCellCount = static_cast<int>(activeCutCells(project).size());
    status.orderedCellCount = static_cast<int>(project.cellOrder.size());
    status.timesheetTrackCount = static_cast<int>(timesheet.tracks.size());
    status.totalFrames = timesheet.totalFrames > 0
        ? timesheet.totalFrames
        : (project.timeline.totalFrames > 0 ? project.timeline.totalFrames : 1);

    for (const Cell& cell : activeCutCells(project)) {
        incrementProjectStructureCategoryCount(status.categoryCounts, cell.category);
    }

    return status;
}

} // namespace perapera
