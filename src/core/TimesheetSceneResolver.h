// このファイルの役割:
// タイムシートで解決したT表示を、キャンバス表示と出力処理が共通利用できる形へ変換する。
// UIやPNG書き出しへ直接依存せず、CellとTimesheetだけから「どのセルのどの作画Fを表示するか」を作る。

#pragma once

#include <vector>

#include "core/Cell.h"
#include "core/TimesheetResolver.h"

namespace perapera {

struct TimesheetSceneCellInput {
    const Cell* cell = nullptr;
    int cellIndex = -1;
    bool requireCellVisible = true;
};

struct ResolvedTimesheetSceneCell {
    const Cell* cell = nullptr;
    int cellIndex = -1;
    bool visible = false;
    int drawingFrameNumber = 0;                    // 1始まり。visible=falseなら0。
    int drawingFrameIndex = -1;                    // 0始まり。visible=falseなら-1。
    CellPlacement placement;
    TimesheetEntryType sourceType = TimesheetEntryType::Hold;
};

struct ResolvedTimesheetSceneFrame {
    int timelineFrame = 1;
    std::vector<ResolvedTimesheetSceneCell> cells;
};

ResolvedTimesheetSceneFrame resolveTimesheetSceneFrame(
    const Timesheet& timesheet,
    const std::vector<TimesheetSceneCellInput>& inputs,
    int timelineFrame);

} // namespace perapera
