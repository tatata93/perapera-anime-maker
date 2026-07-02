#include "core/Scene.h"
#include "core/Timesheet.h"
#include "io/CutIO.h"
#include "io/ProjectLayoutPaths.h"
#include "io/SceneCutLayoutIO.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>
#include <system_error>

namespace {

bool expect(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        return false;
    }
    return true;
}

nlohmann::json readJson(const std::filesystem::path& path)
{
    std::ifstream file(path, std::ios::binary);
    nlohmann::json value;
    file >> value;
    return value;
}

} // namespace

int main()
{
    namespace paths = perapera::ProjectLayoutPaths;
    namespace io = perapera::SceneCutLayoutIO;

    bool ok = true;
    const auto root = std::filesystem::temp_directory_path() / "perapera_scene_cut_layout_io_selftest";
    std::error_code ec;
    std::filesystem::remove_all(root, ec);

    perapera::Scene scene;
    scene.id = "scene_001";
    scene.name = "Scene 1";
    scene.memo = "minimal scene/cut layout selftest";

    perapera::Cut cut;
    cut.id = "cut_001";
    cut.name = "Cut 1";
    cut.frameRate = 24;
    cut.totalFrames = 12;
    cut.cellZOrderKeys = {"cell_A", "cell_BG"};
    cut.timesheet.totalFrames = 12;
    perapera::ensureTimesheetTrack(cut.timesheet, "cell_A", "A");
    perapera::ensureTimesheetTrack(cut.timesheet, "cell_BG", "BG");

    io::SceneCutLayoutSaveResult result;
    std::string error;
    ok &= expect(io::saveMinimalSceneAndCut(root, scene, cut, &result, &error), "save minimal scene and cut");
    if (!ok) {
        std::cerr << error << '\n';
        return 1;
    }

    ok &= expect(std::filesystem::exists(paths::sceneJsonPath(root, "scene_001")), "scene.json exists");
    ok &= expect(std::filesystem::exists(paths::cutJsonPath(root, "scene_001", "cut_001")), "cut.json exists");
    ok &= expect(std::filesystem::exists(paths::timesheetJsonPath(root, "scene_001", "cut_001")), "timesheet.json exists");
    ok &= expect(std::filesystem::is_directory(paths::cellsRoot(root, "scene_001", "cut_001")), "cells folder exists");

    const nlohmann::json sceneJson = readJson(result.sceneJsonPath);
    ok &= expect(sceneJson.value("kind", std::string{}) == "perapera.scene.v1", "scene kind");
    ok &= expect(sceneJson.value("formatVersion", 0) == 1, "scene format version");
    ok &= expect(sceneJson.value("id", std::string{}) == "scene_001", "scene id");
    ok &= expect(sceneJson.at("cutOrder").at(0).get<std::string>() == "cut_001", "scene cut order");

    const nlohmann::json cutJson = readJson(result.cutJsonPath);
    ok &= expect(cutJson.value("kind", std::string{}) == "perapera.cut.v1", "cut kind");
    ok &= expect(cutJson.value("id", std::string{}) == "cut_001", "cut id");
    ok &= expect(cutJson.value("timesheetFile", std::string{}) == "timesheet.json", "cut timesheet file");

    perapera::Cut loaded;
    ok &= expect(perapera::CutIO::loadCut(result.cutFolder, loaded, &error), "load cut back");
    ok &= expect(loaded.id == "cut_001", "loaded cut id");
    ok &= expect(loaded.timesheet.tracks.size() == 2U, "loaded timesheet tracks");

    std::filesystem::remove_all(root, ec);

    if (!ok) {
        return 1;
    }

    std::cout << "perapera_scene_cut_layout_io_selftest passed\n";
    return 0;
}
