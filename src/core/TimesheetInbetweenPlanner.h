#pragma once
// このファイルの役割:
// 原画間へ中割を追加するとき、どのTへ配置するかを決める純粋なcore処理を提供する。
// 作画Fそのものの作成やUI選択変更はApp側の責務として、このファイルでは扱わない。

#include "core/Timesheet.h"

#include <string>
#include <string_view>
#include <vector>

namespace perapera {

struct TimesheetInbetweenPlacementPlan {
    bool ok = false;
    int requestedCount = 0;
    int komaStep = 0; // 0なら枚数指定、1以上ならnコマ打ち指定。
    int previousKeyTimelineFrame = 0;
    int nextKeyTimelineFrame = 0;
    std::vector<int> timelineFrames; // 配置先T。1始まり。
    std::string message;
};

// previousKeyTimelineFrame と nextKeyTimelineFrame の間に、requestedCount枚の中割を置く候補Tを返す。
// 既存entryがあるTは壊さず避ける。空きが足りなければ ok=false を返す。
// 互換用の枚数指定。UIでは基本的に nコマ打ち指定を使う。
TimesheetInbetweenPlacementPlan planTimesheetInbetweenPlacements(
    const Timesheet& timesheet,
    std::string_view cellId,
    int previousKeyTimelineFrame,
    int nextKeyTimelineFrame,
    int requestedCount);

// previousKeyTimelineFrame と nextKeyTimelineFrame の間に、nコマ打ちで中割を置く候補Tを返す。
// 例: T1→T7 の 2コマ打ちなら T3/T5、3コマ打ちなら T4。
// 既存entryがあるTは壊さず、そのTは追加対象から外す。追加できるTが無ければ ok=false。
TimesheetInbetweenPlacementPlan planTimesheetInbetweenPlacementsForKomaStep(
    const Timesheet& timesheet,
    std::string_view cellId,
    int previousKeyTimelineFrame,
    int nextKeyTimelineFrame,
    int komaStep);

} // namespace perapera
