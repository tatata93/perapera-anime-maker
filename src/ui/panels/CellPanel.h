#pragma once
// This file's role: draw the drawing-mode Cell panel and return selection/display/project changes.

#include "core/Project.h"

namespace perapera::ui {

struct CellPanelResult {
    int selectedCellIndex = 0;
    bool selectionChanged = false;
    bool displayChanged = false;
    bool projectStructureChanged = false;
};

CellPanelResult drawCellPanel(Project& project, int activeCellIndex);

} // namespace perapera::ui
