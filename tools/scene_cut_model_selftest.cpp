// このファイルの役割:
// final_spec_v6 Phase 2 Step 1-a の Scene / active Cut 最小モデルを自己診断する。
// Project保存、UI、Scene Plate / シーンパネルには接続しない。

#include "core/Scene.h"
#include "core/Timesheet.h"

#include <iostream>
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

void expectString(std::string_view actual, std::string_view expected, std::string_view label)
{
    if (actual == expected) {
        std::cout << "[OK] " << label << " = " << actual << '\n';
        return;
    }

    ++gFailureCount;
    std::cerr << "[NG] " << label << " expected=" << expected << " actual=" << actual << '\n';
}

perapera::Cut makeCut(std::string_view id, std::string_view name, int totalFrames)
{
    perapera::Cut cut;
    cut.id = std::string{id};
    cut.name = std::string{name};
    cut.frameRate = 24;
    cut.totalFrames = totalFrames;
    cut.timesheet.totalFrames = totalFrames;

    perapera::TimesheetCellTrack& track =
        perapera::ensureTimesheetTrack(cut.timesheet, "cell_A", "Aセル");
    track.entries.push_back({1, perapera::TimesheetEntryType::Key, 1, "原画1"});

    perapera::normalizeTimesheet(cut.timesheet);
    return cut;
}

void runSceneCutModelSelfTest()
{
    perapera::Scene scene;
    scene.id = "scene-001";
    scene.name = "冒頭シーン";
    scene.memo = "Phase 2 Step 1-a minimal model";

    scene.cuts.push_back(makeCut("cut-001", "C001", 12));
    scene.cuts.push_back(makeCut("cut-002", "C002", 24));

    expectString(scene.id, "scene-001", "scene id");
    expectString(scene.name, "冒頭シーン", "scene name");
    expectInt(static_cast<int>(scene.cuts.size()), 2, "scene cut count");

    perapera::Cut* first = perapera::activeCut(scene, 0);
    expectTrue(first != nullptr, "activeCut 0 returns a cut");
    if (first != nullptr) {
        expectString(first->id, "cut-001", "activeCut 0 id");
        expectInt(first->totalFrames, 12, "activeCut 0 totalFrames");
        expectInt(first->timesheet.totalFrames, 12, "activeCut 0 timesheet follows cut");
    }

    perapera::Cut* clamped = perapera::activeCut(scene, 999);
    expectTrue(clamped != nullptr, "activeCut out-of-range clamps");
    if (clamped != nullptr) {
        expectString(clamped->id, "cut-002", "activeCut clamped id");
        expectInt(clamped->timesheet.totalFrames, 24, "activeCut clamped timesheet follows cut");
    }

    const std::vector<std::string> defaultOrder = perapera::resolvedCutOrder(scene);
    expectInt(static_cast<int>(defaultOrder.size()), 2, "resolved default cut order count");
    expectString(defaultOrder[0], "cut-001", "resolved default cut order first");
    expectString(defaultOrder[1], "cut-002", "resolved default cut order second");

    scene.cutOrder = {"cut-002", "cut-001"};
    const std::vector<std::string> explicitOrder = perapera::resolvedCutOrder(scene);
    expectInt(static_cast<int>(explicitOrder.size()), 2, "resolved explicit cut order count");
    expectString(explicitOrder[0], "cut-002", "resolved explicit cut order first");
    expectString(explicitOrder[1], "cut-001", "resolved explicit cut order second");

    perapera::Scene empty;
    expectTrue(perapera::activeCut(empty, 0) == nullptr, "activeCut empty scene is null");
    expectInt(static_cast<int>(perapera::resolvedCutOrder(empty).size()), 0, "empty scene cut order is empty");
}

} // namespace

int main()
{
    std::cout << "Scene/Cut model self-test start\n";

    runSceneCutModelSelfTest();

    if (gFailureCount == 0) {
        std::cout << "Scene/Cut model self-test passed\n";
        return 0;
    }

    std::cerr << "Scene/Cut model self-test failed: " << gFailureCount << " failure(s)\n";
    return 1;
}
