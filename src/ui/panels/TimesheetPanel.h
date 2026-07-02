#pragma once
// このファイルの役割:
// 正式タイムシートUIの最小編集スケルトンを描画する。
// Step 4では編集結果をUI状態内の一時データとして保持し、保存・キャンバス反映・再生反映は行わない。

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

enum class TimesheetPanelEntryKind {
    Empty,      // 未記入。紙タイムシート上の空欄。表示解決では前状態を維持する想定。
    Hold,       // ホールドを明示する。
    Drawing,    // 通常の作画F指定。
    Null,       // 空セル。非表示にする。
    Key,        // 原画としての作画F指定。
    Inbetween,  // 中割としての作画F指定。
};

struct TimesheetPanelEditableEntry {
    int timelineFrame = 0; // UI内部では0-based。
    std::string cellId;
    TimesheetPanelEntryKind kind = TimesheetPanelEntryKind::Empty;
    int drawingFrameNumber = 0; // UI表示は1始まり。0は未指定。
};

struct TimesheetPanelState {
    bool windowOpen = true;
    bool showDetachedWindow = true;
    bool showInstructionColumns = true;
    int selectedTimelineFrame = 0; // 0-based
    int selectedCellColumn = 0;    // 0-based
    int editDrawingFrameNumber = 1;
    float rowHeight = 22.0f;

    // Step 4の一時編集データ。
    // まだProject/Cut/Timesheet保存には接続しない。
    std::vector<TimesheetPanelEditableEntry> entries;
};

struct TimesheetPanelResult {
    bool windowOpenChanged = false;
    bool timelineFrameChanged = false;
    bool cellSelectionChanged = false;
    bool entryChanged = false;
    int selectedTimelineFrame = 0; // 0-based
    int selectedCellColumn = 0;    // 0-based
};

// 縦型タイムシートウィンドウを描画する。
// Step 4では編集結果をstate.entriesだけに保持し、保存・表示解決・キャンバス反映は後続Stepで行う。
TimesheetPanelResult drawTimesheetPanel(const TimesheetPanelViewModel& data, TimesheetPanelState& state);

} // namespace perapera::ui
