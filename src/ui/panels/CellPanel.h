#pragma once
// This file's role: draw the drawing-mode Cell panel and expose compact cell display mode state. v1.9e keeps compact cell controls and moves the timesheet editor into a detached ImGui window.

#include "core/Project.h"

namespace perapera::ui {

enum class CellDisplayMode {
    ActiveOnly,
    VisibleCells,
    SoloSelected,
};

struct CellPanelResult {
    int selectedCellIndex = 0;
    bool selectionChanged = false;
    bool displayChanged = false;
    bool projectStructureChanged = false;
    bool timesheetChanged = false;
};

CellPanelResult drawCellPanel(Project& project, int activeCellIndex);
CellDisplayMode currentCellDisplayMode();
int currentSoloCellIndex();

} // namespace perapera::ui
