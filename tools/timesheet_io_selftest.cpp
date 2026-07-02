// このファイルの役割:
// 正式タイムシートv1 IO の最小自己診断を行う。
// UI、Project保存、描画には接続せず、Timesheet JSON の保存/読み込み往復だけを検証する。

#include "core/Timesheet.h"
#include "core/TimesheetResolver.h"
#include "io/TimesheetIO.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>

namespace {

namespace fs = std::filesystem;

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

void expectString(std::string_view actual, std::string_view expected, std::string_view label)
{
    if (actual == expected) {
        std::cout << "[OK] " << label << " = " << actual << '\n';
        return;
    }

    ++gFailureCount;
    std::cerr << "[NG] " << label << " expected=" << expected << " actual=" << actual << '\n';
}

std::string readWholeFile(const fs::path& path)
{
    std::ifstream file(path, std::ios::binary);
    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

perapera::Timesheet makeSampleTimesheet()
{
    perapera::Timesheet timesheet;
    timesheet.totalFrames = 12;
    timesheet.defaultExposure = 2;

    perapera::TimesheetCellTrack& a = perapera::ensureTimesheetTrack(timesheet, "cell-a", "Aセル");
    a.defaultExposure = 2;
    a.entries.push_back({1, perapera::TimesheetEntryType::Key, 1, "原画1"});
    a.entries.push_back({2, perapera::TimesheetEntryType::Hold, 0, ""});
    a.entries.push_back({3, perapera::TimesheetEntryType::Inbetween, 2, "中割2"});
    a.entries.push_back({5, perapera::TimesheetEntryType::Null, 0, "空セル"});
    a.entries.push_back({7, perapera::TimesheetEntryType::Drawing, 3, "作画3"});

    perapera::TimesheetCellTrack& b = perapera::ensureTimesheetTrack(timesheet, "cell-b", "Bセル");
    b.entries.push_back({1, perapera::TimesheetEntryType::Drawing, 1, ""});
    b.entries.push_back({4, perapera::TimesheetEntryType::Drawing, 2, ""});

    timesheet.frameNotes.push_back({1, "おはよう", "PAN開始", "透過光", "BG-01"});
    timesheet.frameNotes.push_back({9, "", "TU", "", "BOOK手前"});

    perapera::normalizeTimesheet(timesheet);
    return timesheet;
}

void runIoSelfTest()
{
    const fs::path root = fs::temp_directory_path() / "perapera_timesheet_io_selftest";
    fs::remove_all(root);
    fs::create_directories(root);

    const fs::path cutFolder = root / "cut_001";
    const fs::path path = perapera::TimesheetIO::timesheetPathForCutFolder(cutFolder);

    perapera::Timesheet original = makeSampleTimesheet();

    std::string error;
    expectTrue(perapera::TimesheetIO::saveTimesheet(original, path, &error), "saveTimesheet succeeds");
    expectTrue(fs::exists(path), "timesheet.json exists");

    const std::string firstWrite = readWholeFile(path);
    expectTrue(firstWrite.find("perapera.timesheet.v1") != std::string::npos, "kind is written");
    expectTrue(firstWrite.find("Aセル") != std::string::npos, "Japanese displayName is written as UTF-8");
    expectTrue(firstWrite.find("cameraInstruction") != std::string::npos, "frame note columns are written");

    perapera::Timesheet loaded;
    expectTrue(perapera::TimesheetIO::loadTimesheet(path, loaded, &error), "loadTimesheet succeeds");

    expectInt(loaded.totalFrames, 12, "loaded totalFrames");
    expectInt(loaded.defaultExposure, 2, "loaded defaultExposure");
    expectInt(static_cast<int>(loaded.tracks.size()), 2, "loaded track count");
    expectString(loaded.tracks[0].cellId, "cell-a", "loaded A cellId");
    expectString(loaded.tracks[0].displayName, "Aセル", "loaded A displayName");
    expectInt(static_cast<int>(loaded.tracks[0].entries.size()), 5, "loaded A entries");
    expectInt(static_cast<int>(loaded.frameNotes.size()), 2, "loaded frame notes");
    expectString(loaded.frameNotes[0].dialogue, "おはよう", "loaded dialogue");
    expectString(loaded.frameNotes[0].shootingInstruction, "透過光", "loaded shooting instruction");

    const perapera::ResolvedTimesheetCell a4 = perapera::resolveTimesheetCell(loaded, "cell-a", 4);
    expectTrue(a4.visible, "loaded resolver A T4 visible");
    expectInt(a4.drawingFrameNumber, 2, "loaded resolver A T4 drawingFrameNumber");

    const perapera::ResolvedTimesheetCell a6 = perapera::resolveTimesheetCell(loaded, "cell-a", 6);
    expectTrue(!a6.visible, "loaded resolver A T6 keeps Null hidden");
    expectInt(a6.drawingFrameNumber, 0, "loaded resolver A T6 drawingFrameNumber");

    const perapera::ResolvedTimesheetCell a8 = perapera::resolveTimesheetCell(loaded, "cell-a", 8);
    expectTrue(a8.visible, "loaded resolver A T8 visible again");
    expectInt(a8.drawingFrameNumber, 3, "loaded resolver A T8 drawingFrameNumber");

    expectTrue(perapera::TimesheetIO::saveTimesheet(loaded, path, &error), "save loaded timesheet succeeds");
    const std::string secondWrite = readWholeFile(path);
    expectString(secondWrite, firstWrite, "stable JSON after round-trip");

    perapera::Timesheet missing;
    expectTrue(!perapera::TimesheetIO::loadTimesheet(root / "missing.json", missing, &error), "missing file load fails safely");
    expectInt(missing.totalFrames, 144, "missing load resets to safe default totalFrames");

    fs::remove_all(root);
}

} // namespace

int main()
{
    std::cout << "Timesheet IO self-test start\n";
    runIoSelfTest();

    if (gFailureCount == 0) {
        std::cout << "Timesheet IO self-test passed\n";
        return 0;
    }

    std::cerr << "Timesheet IO self-test failed: " << gFailureCount << " failure(s)\n";
    return 1;
}
