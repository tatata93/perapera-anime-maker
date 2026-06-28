#pragma once
// このファイルの役割:
// TimesheetPanelの一時編集状態と正式core Timesheetモデルの変換境界を提供する。
// UI描画、保存、キャンバス反映、再生反映には直接関与しない。

#include "core/Timesheet.h"
#include "ui/panels/TimesheetPanel.h"

namespace perapera::ui {

TimesheetEntryType timesheetEntryTypeFromPanelKind(TimesheetPanelEntryKind kind) noexcept;
TimesheetPanelEntryKind panelKindFromTimesheetEntryType(TimesheetEntryType type) noexcept;

// TimesheetPanelState::entries の一時入力を、正式 Timesheet の疎データへ変換する。
// Empty は未記入なので保存対象にしない。
// UI側 timelineFrame は 0-based、core側 timelineFrame は 1-based として変換する。
Timesheet buildTimesheetFromPanelState(
    const TimesheetPanelViewModel& data,
    const TimesheetPanelState& state);

// 正式 Timesheet の疎データを TimesheetPanelState::entries へ戻す。
// 既存の一時entriesは置き換える。
// 選択Tやウィンドウ開閉状態は維持する。
void replacePanelEntriesFromTimesheet(
    const Timesheet& timesheet,
    TimesheetPanelState& state);

// 現在のセル列・総フレーム数に合わせて、TimesheetPanelStateを安全に正規化する。
// - 選択T / 選択セル列を範囲内にクランプする。
// - 存在しないセルID、範囲外T、Empty指定を一時entriesから取り除く。
// - 同じ T x セル に複数entryがある場合は、後のentryを採用する。
// 何かを変更した場合は true を返す。
bool normalizeTimesheetPanelStateForViewModel(
    const TimesheetPanelViewModel& data,
    TimesheetPanelState& state);

} // namespace perapera::ui
