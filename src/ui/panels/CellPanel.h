#pragma once
// This file's role: draw the drawing-mode Cell panel and expose compact cell display mode state. v1.8c uses single-line cell rows and compact selected-cell controls.

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
