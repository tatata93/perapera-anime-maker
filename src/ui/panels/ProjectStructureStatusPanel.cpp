#include "ui/panels/ProjectStructureStatusPanel.h"

#include "core/ProjectStructureStatus.h"
#include "imgui.h"

namespace perapera::ui {

void drawProjectStructureStatusPanel(const Project& project, const Timesheet& timesheet)
{
    const ProjectStructureStatus status = buildProjectStructureStatus(project, timesheet);

    ImGui::SeparatorText("Project / Scene / Cut");
    ImGui::Text("Scene: %s  |  Cut: %s", status.sceneId.c_str(), status.cutId.c_str());
    ImGui::Text("Cells: %d  |  Cell order: %d  |  Timesheet tracks: %d  |  T: %d",
                status.activeCutCellCount,
                status.orderedCellCount,
                status.timesheetTrackCount,
                status.totalFrames);

    if (!status.categoryCounts.empty()) {
        ImGui::TextUnformatted("Cell categories:");
        for (const ProjectStructureCategoryCount& count : status.categoryCounts) {
            ImGui::SameLine();
            ImGui::Text("%s=%d", count.category.c_str(), count.count);
        }
    }

    ImGui::TextDisabled("Phase 2 Step 1-c: Project.cells are displayed as the current active Cut cells. No scene-management panel is added.");
}

} // namespace perapera::ui
