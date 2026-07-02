// このファイルの役割:
// タイムシート上の選択T/セルから、前後の原画と中割候補を検出する。
// 中割作成UI、ライトテーブル自動設定、原画間ループ確認の前段となる純粋なcore処理である。

#pragma once

#include "core/Timesheet.h"

#include <string>
#include <string_view>
#include <vector>

namespace perapera {

struct TimesheetKeyDrawingEndpoint {
    bool found = false;
    int timelineFrame = 0;       // T。1始まり。found=falseなら0。
    int drawingFrameNumber = 0;  // 作画F番号。1始まり。found=falseなら0。
};

struct TimesheetInbetweenDrawingInfo {
    int timelineFrame = 0;       // T。1始まり。
    int drawingFrameNumber = 0;  // 作画F番号。1始まり。
    int inbetweenIndex = 0;      // 原画間内での中割順。1始まり。
    int inbetweenCount = 0;      // 原画間内の中割総数。
};

struct ResolvedTimesheetInbetweenInterval {
    std::string cellId;
    int selectedTimelineFrame = 1;

    TimesheetKeyDrawingEndpoint previousKey;
    TimesheetKeyDrawingEndpoint nextKey;
    std::vector<TimesheetInbetweenDrawingInfo> inbetweens;

    bool hasClosedKeyInterval = false; // 前後原画が両方見つかった。
    bool selectedInsideInterval = false;
    bool selectedIsKey = false;
    bool selectedIsInbetween = false;

    // 選択Tが前後原画間のどの位置か。例: T3 が T1->T7 の間なら 2/6。
    int selectedPositionNumerator = 0;
    int selectedPositionDenominator = 0;

    // 前後原画間の空きT数。中割作成UIで配置候補数として使う。
    int availableTimelineSlotCount = 0;
    int usedInbetweenCount = 0;
};

ResolvedTimesheetInbetweenInterval resolveTimesheetInbetweenInterval(
    const Timesheet& timesheet,
    std::string_view cellId,
    int selectedTimelineFrame);

} // namespace perapera
