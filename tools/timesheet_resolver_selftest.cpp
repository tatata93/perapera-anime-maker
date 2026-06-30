// このファイルの役割:
// 正式タイムシートv1 core resolver の最小自己診断を行う。
// UI、保存、描画には接続せず、Hold / Null / Drawing / Key / Inbetween の解決規則だけを検証する。

#include "core/Timesheet.h"
#include "core/TimesheetResolver.h"

#include <iostream>
#include <string>
#include <string_view>

namespace {

int gFailureCount = 0;

void expectTrue(bool condition, std::string_view label)
{
    if (condition) {
        std::cout << "[OK] " << label << '\n';
        return;
    }

    ++gFailureCount;
    std::cerr << "[NG] " << label << '\n';
}

void expectInt(int actual, int expected, std::string_view label)
{
    if (actual == expected) {
        std::cout << "[OK] " << label << " = " << actual << '\n';
        return;
    }

    ++gFailureCount;
    std::cerr << "[NG] " << label << " expected=" << expected << " actual=" << actual << '\n';
}

void expectType(perapera::TimesheetEntryType actual, perapera::TimesheetEntryType expected, std::string_view label)
{
    if (actual == expected) {
        std::cout << "[OK] " << label << " = " << perapera::timesheetEntryTypeToString(actual) << '\n';
        return;
    }

    ++gFailureCount;
    std::cerr << "[NG] " << label
              << " expected=" << perapera::timesheetEntryTypeToString(expected)
              << " actual=" << perapera::timesheetEntryTypeToString(actual) << '\n';
}

perapera::Timesheet makeSampleTimesheet()
{
    perapera::Timesheet timesheet;
    timesheet.totalFrames = 8;
    timesheet.defaultExposure = 2;

    perapera::TimesheetCellTrack& a = perapera::ensureTimesheetTrack(timesheet, "cell-a", "Aセル");
    a.entries.push_back({1, perapera::TimesheetEntryType::Drawing, 1, "T1で作画F1"});
    a.entries.push_back({2, perapera::TimesheetEntryType::Hold, 0, "T2はホールド"});
    a.entries.push_back({3, perapera::TimesheetEntryType::Null, 0, "T3で空セル"});
    a.entries.push_back({4, perapera::TimesheetEntryType::Hold, 0, "T4は空セルを保持"});
    a.entries.push_back({5, perapera::TimesheetEntryType::Key, 2, "T5で原画F2"});
    a.entries.push_back({7, perapera::TimesheetEntryType::Inbetween, 3, "T7で中割F3"});

    perapera::TimesheetCellTrack& b = perapera::ensureTimesheetTrack(timesheet, "cell-b", "Bセル");
    b.entries.push_back({1, perapera::TimesheetEntryType::Drawing, 1, "先に作画F1"});
    b.entries.push_back({1, perapera::TimesheetEntryType::Null, 0, "同じTでは後の指定が勝つ"});
    b.entries.push_back({6, perapera::TimesheetEntryType::Drawing, 2, "T6で再表示"});

    perapera::TimesheetCellTrack& c = perapera::ensureTimesheetTrack(timesheet, "cell-c", "Cセル");
    c.entries.push_back({99, perapera::TimesheetEntryType::Drawing, 10, "範囲外Tは正規化で最終Tへ"});

    perapera::normalizeTimesheet(timesheet);
    return timesheet;
}

void runResolverSelfTest()
{
    perapera::Timesheet timesheet = makeSampleTimesheet();

    expectInt(timesheet.totalFrames, 8, "totalFrames正規化");
    expectInt(static_cast<int>(timesheet.tracks.size()), 3, "track数");
    expectInt(static_cast<int>(timesheet.tracks[1].entries.size()), 2, "同一T重複は正規化で後勝ち1件へ整理");

    {
        const perapera::ResolvedTimesheetCell resolved = perapera::resolveTimesheetCell(timesheet, "cell-a", 0);
        expectTrue(resolved.visible, "T0はT1へクランプされ、Aセルは表示");
        expectInt(resolved.drawingFrameNumber, 1, "A T0/T1 作画F");
        expectType(resolved.sourceType, perapera::TimesheetEntryType::Drawing, "A T0/T1 sourceType");
    }

    {
        const perapera::ResolvedTimesheetCell resolved = perapera::resolveTimesheetCell(timesheet, "cell-a", 2);
        expectTrue(resolved.visible, "A T2 ホールド後も表示");
        expectInt(resolved.drawingFrameNumber, 1, "A T2 作画F");
        expectType(resolved.sourceType, perapera::TimesheetEntryType::Hold, "A T2 sourceType");
    }

    {
        const perapera::ResolvedTimesheetCell resolved = perapera::resolveTimesheetCell(timesheet, "cell-a", 4);
        expectTrue(!resolved.visible, "A T4 空セルをホールドして非表示");
        expectInt(resolved.drawingFrameNumber, 0, "A T4 作画Fなし");
        expectType(resolved.sourceType, perapera::TimesheetEntryType::Hold, "A T4 sourceType");
    }

    {
        const perapera::ResolvedTimesheetCell resolved = perapera::resolveTimesheetCell(timesheet, "cell-a", 5);
        expectTrue(resolved.visible, "A T5 原画で表示");
        expectInt(resolved.drawingFrameNumber, 2, "A T5 作画F");
        expectType(resolved.sourceType, perapera::TimesheetEntryType::Key, "A T5 sourceType");
    }

    {
        const perapera::ResolvedTimesheetCell resolved = perapera::resolveTimesheetCell(timesheet, "cell-a", 8);
        expectTrue(resolved.visible, "A T8 中割F3を保持");
        expectInt(resolved.drawingFrameNumber, 3, "A T8 作画F");
        expectType(resolved.sourceType, perapera::TimesheetEntryType::Inbetween, "A T8 sourceType");
    }

    {
        const perapera::ResolvedTimesheetCell resolved = perapera::resolveTimesheetCell(timesheet, "cell-b", 1);
        expectTrue(!resolved.visible, "B T1 同一Tの後勝ちNullで非表示");
        expectInt(resolved.drawingFrameNumber, 0, "B T1 作画Fなし");
        expectType(resolved.sourceType, perapera::TimesheetEntryType::Null, "B T1 sourceType");
    }

    {
        const perapera::ResolvedTimesheetCell resolved = perapera::resolveTimesheetCell(timesheet, "cell-b", 6);
        expectTrue(resolved.visible, "B T6 Drawingで再表示");
        expectInt(resolved.drawingFrameNumber, 2, "B T6 作画F");
        expectType(resolved.sourceType, perapera::TimesheetEntryType::Drawing, "B T6 sourceType");
    }

    {
        const perapera::ResolvedTimesheetCell resolved = perapera::resolveTimesheetCell(timesheet, "cell-c", 8);
        expectTrue(resolved.visible, "C 範囲外T指定は最終Tへ正規化される");
        expectInt(resolved.drawingFrameNumber, 10, "C T8 作画F");
        expectType(resolved.sourceType, perapera::TimesheetEntryType::Drawing, "C T8 sourceType");
    }

    {
        const perapera::ResolvedTimesheetCell resolved = perapera::resolveTimesheetCell(timesheet, "missing-cell", 5);
        expectTrue(!resolved.visible, "存在しないセル列は非表示");
        expectInt(resolved.drawingFrameNumber, 0, "存在しないセル列 作画Fなし");
    }

    {
        const perapera::ResolvedTimesheetFrame resolvedFrame = perapera::resolveTimesheetFrame(timesheet, 8);
        expectInt(resolvedFrame.timelineFrame, 8, "ResolvedFrame T");
        expectInt(static_cast<int>(resolvedFrame.cells.size()), 3, "ResolvedFrame セル数");
        expectInt(resolvedFrame.cells[0].drawingFrameNumber, 3, "ResolvedFrame A 作画F");
        expectInt(resolvedFrame.cells[1].drawingFrameNumber, 2, "ResolvedFrame B 作画F");
        expectInt(resolvedFrame.cells[2].drawingFrameNumber, 10, "ResolvedFrame C 作画F");
    }
}

} // namespace

int main()
{
    std::cout << "Timesheet resolver self-test start\n";
    runResolverSelfTest();

    if (gFailureCount == 0) {
        std::cout << "Timesheet resolver self-test passed\n";
        return 0;
    }

    std::cerr << "Timesheet resolver self-test failed: " << gFailureCount << " failure(s)\n";
    return 1;
}
