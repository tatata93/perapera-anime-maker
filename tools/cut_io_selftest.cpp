// このファイルの役割:
// 正式Cut IOの最小自己診断を行う。
// UI、Project保存、描画には接続せず、cut.json と timesheet.json の保存/読み込み往復だけを検証する。

#include "core/Cut.h"
#include "core/TimesheetResolver.h"
#include "io/CutIO.h"

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

perapera::Cut makeSampleCut()
{
    perapera::Cut cut;
    cut.id = "cut-001";
    cut.name = "第1カット";
    cut.frameRate = 24;
    cut.totalFrames = 18;
    cut.cellZOrderKeys = {"bg", "cell-a", "cell-b", "book"};

    cut.timesheet.totalFrames = cut.totalFrames;
    cut.timesheet.defaultExposure = 2;

    perapera::TimesheetCellTrack& a = perapera::ensureTimesheetTrack(cut.timesheet, "cell-a", "Aセル");
    a.defaultExposure = 2;
    a.entries.push_back({1, perapera::TimesheetEntryType::Key, 1, "原画1"});
    a.entries.push_back({3, perapera::TimesheetEntryType::Inbetween, 2, "中割"});
    a.entries.push_back({5, perapera::TimesheetEntryType::Null, 0, "空セル"});
    a.entries.push_back({9, perapera::TimesheetEntryType::Drawing, 3, "復帰"});

    perapera::TimesheetCellTrack& b = perapera::ensureTimesheetTrack(cut.timesheet, "cell-b", "Bセル");
    b.entries.push_back({2, perapera::TimesheetEntryType::Drawing, 1, "B開始"});
    b.entries.push_back({7, perapera::TimesheetEntryType::Hold, 0, "保持"});

    cut.timesheet.frameNotes.push_back({1, "おはよう", "FIX", "通常", "BG1"});
    cut.timesheet.frameNotes.push_back({9, "", "PAN開始", "透過光", "BOOK前面"});

    perapera::normalizeTimesheet(cut.timesheet);
    return cut;
}

void runCutIoSelfTest()
{
    const fs::path root = fs::temp_directory_path() / "perapera_cut_io_selftest";
    fs::remove_all(root);

    const fs::path cutFolder = root / "scene-001" / "cut-001";
    const fs::path cutJson = perapera::CutIO::cutJsonPathForCutFolder(cutFolder);
    const fs::path timesheetJson = perapera::CutIO::timesheetJsonPathForCutFolder(cutFolder);

    expectString(cutJson.filename().string(), "cut.json", "cut json filename");
    expectString(timesheetJson.filename().string(), "timesheet.json", "timesheet json filename");

    const perapera::Cut original = makeSampleCut();
    std::string error;
    expectTrue(perapera::CutIO::saveCut(original, cutFolder, &error), "saveCut succeeds");
    expectTrue(fs::exists(cutJson), "cut.json is written");
    expectTrue(fs::exists(timesheetJson), "timesheet.json is written");

    const std::string cutJsonText = readWholeFile(cutJson);
    const std::string timesheetJsonText = readWholeFile(timesheetJson);
    expectTrue(cutJsonText.find("perapera.cut.v1") != std::string::npos, "cut kind is written");
    expectTrue(cutJsonText.find("timesheet.json") != std::string::npos, "timesheet file reference is written");
    expectTrue(cutJsonText.find("第1カット") != std::string::npos, "Japanese cut name is written as UTF-8");
    expectTrue(cutJsonText.find("Aセル") == std::string::npos, "timesheet track display names are not mixed into cut.json");
    expectTrue(timesheetJsonText.find("Aセル") != std::string::npos, "timesheet track display names are written to timesheet.json");
    expectTrue(timesheetJsonText.find("透過光") != std::string::npos, "shooting instruction is written to timesheet.json");

    perapera::Cut loaded;
    expectTrue(perapera::CutIO::loadCut(cutFolder, loaded, &error), "loadCut succeeds");

    expectString(loaded.id, "cut-001", "loaded cut id");
    expectString(loaded.name, "第1カット", "loaded cut name");
    expectInt(loaded.frameRate, 24, "loaded frameRate");
    expectInt(loaded.totalFrames, 18, "loaded totalFrames");
    expectInt(static_cast<int>(loaded.cellZOrderKeys.size()), 4, "loaded cellZOrderKeys count");
    expectString(loaded.cellZOrderKeys[2], "cell-b", "loaded cellZOrderKeys order");
    expectInt(loaded.timesheet.totalFrames, 18, "loaded timesheet totalFrames follows cut");
    expectInt(static_cast<int>(loaded.timesheet.tracks.size()), 2, "loaded timesheet track count");
    expectString(loaded.timesheet.tracks[0].displayName, "Aセル", "loaded timesheet displayName");
    expectInt(static_cast<int>(loaded.timesheet.frameNotes.size()), 2, "loaded frame notes count");

    const perapera::ResolvedTimesheetCell a4 = perapera::resolveTimesheetCell(loaded.timesheet, "cell-a", 4);
    expectTrue(a4.visible, "loaded cut resolver A T4 visible");
    expectInt(a4.drawingFrameNumber, 2, "loaded cut resolver A T4 drawingFrameNumber");

    const perapera::ResolvedTimesheetCell a6 = perapera::resolveTimesheetCell(loaded.timesheet, "cell-a", 6);
    expectTrue(!a6.visible, "loaded cut resolver A T6 keeps Null hidden");
    expectInt(a6.drawingFrameNumber, 0, "loaded cut resolver A T6 drawingFrameNumber");

    const perapera::ResolvedTimesheetCell a10 = perapera::resolveTimesheetCell(loaded.timesheet, "cell-a", 10);
    expectTrue(a10.visible, "loaded cut resolver A T10 visible again");
    expectInt(a10.drawingFrameNumber, 3, "loaded cut resolver A T10 drawingFrameNumber");

    expectTrue(perapera::CutIO::saveCut(loaded, cutFolder, &error), "save loaded cut succeeds");
    expectString(readWholeFile(cutJson), cutJsonText, "stable cut.json after round-trip");
    expectString(readWholeFile(timesheetJson), timesheetJsonText, "stable timesheet.json after round-trip");

    perapera::Cut missing;
    expectTrue(!perapera::CutIO::loadCut(root / "missing-cut", missing, &error), "missing cut load fails safely");
    expectInt(missing.totalFrames, 144, "missing cut resets to safe default totalFrames");
    expectInt(missing.timesheet.totalFrames, 144, "missing cut resets timesheet totalFrames");

    fs::remove_all(root);
}

} // namespace

int main()
{
    std::cout << "Cut IO self-test start\n";
    runCutIoSelfTest();

    if (gFailureCount == 0) {
        std::cout << "Cut IO self-test passed\n";
        return 0;
    }

    std::cerr << "Cut IO self-test failed: " << gFailureCount << " failure(s)\n";
    return 1;
}
