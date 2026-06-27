#pragma once
// This file's role: draw the drawing-mode Cell panel, popup cell edit/order controls, and expose cell display mode state.

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
};

CellPanelResult drawCellPanel(Project& project, int activeCellIndex);
CellDisplayMode currentCellDisplayMode();
int currentSoloCellIndex();

} // namespace perapera::ui
