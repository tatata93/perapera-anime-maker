#pragma once
// このファイルの役割:
// 正式タイムシートUIの表示専用スケルトンを描画する。
// Step 3では編集・保存・キャンバス反映は行わず、縦型タイムシート表と独立ウィンドウの土台だけを提供する。

#include <string>
#include <vector>

namespace perapera::ui {

struct TimesheetPanelCellColumn {
    std::string cellId;
    std::string displayName;
    std::string category;
};

struct TimesheetPanelViewModel {
    int totalFrames = 144;
    int fps = 24;
    std::vector<TimesheetPanelCellColumn> cells;
};

struct TimesheetPanelState {
    bool windowOpen = true;
    bool showDetachedWindow = true;
    bool showInstructionColumns = true;
    int selectedTimelineFrame = 0; // 0-based
    float rowHeight = 22.0f;
};

struct TimesheetPanelResult {
    bool windowOpenChanged = false;
    bool timelineFrameChanged = false;
    int selectedTimelineFrame = 0; // 0-based
};

// 表示専用の縦型タイムシートウィンドウを描画する。
// dataは後続Stepで正式Cut/Timesheetから作る想定。Step 3では仮の表示モデルだけを受け取る。
TimesheetPanelResult drawTimesheetPanel(const TimesheetPanelViewModel& data, TimesheetPanelState& state);

} // namespace perapera::ui
