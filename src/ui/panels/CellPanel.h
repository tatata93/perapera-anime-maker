#pragma once
// This file's role: draw the drawing-mode Cell panel and expose compact cell display mode state. v1.9h keeps compact cell controls and uses Japanese UI and keeps the detached timesheet window synchronized with the current timeline frame.

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
    bool timesheetChanged = false; // タイムシートの露出指定が変更された
    bool timelineFrameChanged = false; // タイムシートウィンドウ側で表示中タイムラインFを変更した
    bool drawingFrameChanged = false; // タイムシート欄から編集対象の作画Fを変更した
    int selectedTimelineFrame = 0; // 0-based
    int selectedDrawingFrameIndex = 0; // 0-based
};

CellPanelResult drawCellPanel(Project& project, int activeCellIndex, int activeTimelineFrame = 0);
CellDisplayMode currentCellDisplayMode();
int currentSoloCellIndex();

} // namespace perapera::ui
