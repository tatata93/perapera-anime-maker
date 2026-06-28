// このファイルの役割:
// 正式タイムシートv1の表示解決処理を定義する。
// UIや描画へ直接依存せず、Tとセル列から「表示すべき作画F」を求める。

#pragma once

#include "core/Timesheet.h"

#include <string>
#include <string_view>
#include <vector>

namespace perapera {

struct ResolvedTimesheetCell {
    std::string cellId;
    bool visible = false;
    int drawingFrameNumber = 0;                    // 1始まり。visible=falseなら0。
    TimesheetEntryType sourceType = TimesheetEntryType::Hold;
    const TimesheetEntry* sourceEntry = nullptr;   // 解決に最後に効いた記入。なければnullptr。
};

struct ResolvedTimesheetFrame {
    int timelineFrame = 1;
    std::vector<ResolvedTimesheetCell> cells;
};

ResolvedTimesheetCell resolveTimesheetCell(
    const Timesheet& timesheet,
    std::string_view cellId,
    int timelineFrame);

ResolvedTimesheetFrame resolveTimesheetFrame(
    const Timesheet& timesheet,
    int timelineFrame);

} // namespace perapera
