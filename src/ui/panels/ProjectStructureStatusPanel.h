#pragma once

// final_spec_v6 Phase 2 Step 1-c:
// Draw a small status block inside existing UI. This is not a scene-management panel.

#include "core/Project.h"
#include "core/Timesheet.h"

namespace perapera::ui {

void drawProjectStructureStatusPanel(const Project& project, const Timesheet& timesheet);

} // namespace perapera::ui
