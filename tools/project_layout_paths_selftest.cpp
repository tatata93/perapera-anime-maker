#include "io/ProjectLayoutPaths.h"

#include <filesystem>
#include <iostream>
#include <string>
#include <system_error>

namespace {

bool expect(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        return false;
    }
    return true;
}

std::string generic(const std::filesystem::path& path) {
    return path.generic_string();
}

} // namespace

int main() {
    const std::filesystem::path root = "project_root";
    bool ok = true;

    ok &= expect(generic(perapera::scenesDirectory(root)) == "project_root/scenes", "scenes root path");
    ok &= expect(generic(perapera::sceneDirectory(root, "scene_001")) == "project_root/scenes/scene_001", "scene folder path");
    ok &= expect(generic(perapera::sceneJsonPath(root, "scene_001")) == "project_root/scenes/scene_001/scene.json", "scene json path");
    ok &= expect(generic(perapera::cutsDirectory(root, "scene_001")) == "project_root/scenes/scene_001/cuts", "cuts root path");
    ok &= expect(generic(perapera::cutDirectory(root, "scene_001", "cut_001")) == "project_root/scenes/scene_001/cuts/cut_001", "cut folder path");
    ok &= expect(generic(perapera::cutJsonPath(root, "scene_001", "cut_001")) == "project_root/scenes/scene_001/cuts/cut_001/cut.json", "cut json path");
    ok &= expect(generic(perapera::cutTimesheetJsonPath(root, "scene_001", "cut_001")) == "project_root/scenes/scene_001/cuts/cut_001/timesheet.json", "timesheet json path");
    ok &= expect(generic(perapera::cellsDirectory(root, "scene_001", "cut_001")) == "project_root/scenes/scene_001/cuts/cut_001/cells", "cells root path");
    ok &= expect(generic(perapera::cellDirectory(root, "scene_001", "cut_001", "cell_BG")) == "project_root/scenes/scene_001/cuts/cut_001/cells/cell_BG", "cell folder path");
    ok &= expect(generic(perapera::cellJsonPath(root, "scene_001", "cut_001", "cell_BG")) == "project_root/scenes/scene_001/cuts/cut_001/cells/cell_BG/cell.json", "cell json path");
    ok &= expect(generic(perapera::framesDirectory(root, "scene_001", "cut_001", "cell_BG")) == "project_root/scenes/scene_001/cuts/cut_001/cells/cell_BG/frames", "frames root path");
    ok &= expect(generic(perapera::frameDirectory(root, "scene_001", "cut_001", "cell_BG", 0)) == "project_root/scenes/scene_001/cuts/cut_001/cells/cell_BG/frames/frame_001", "frame folder path");

    ok &= expect(perapera::normalizeLayoutId("scene 001", "fallback") == "scene_001", "sanitize space");
    ok &= expect(perapera::normalizeLayoutId("", "fallback") == "fallback", "sanitize empty fallback");
    ok &= expect(perapera::normalizeLayoutId("cell/A", "fallback") == "cell_A", "sanitize slash");

    const auto tempRoot = std::filesystem::temp_directory_path() / "perapera_project_layout_paths_selftest";
    std::error_code ec;
    std::filesystem::remove_all(tempRoot, ec);
    ec.clear();

    std::filesystem::create_directories(perapera::cellsDirectory(tempRoot, "scene_001", "cut_001"), ec);
    ok &= expect(!ec, "create cut skeleton");
    ok &= expect(std::filesystem::is_directory(perapera::cellsDirectory(tempRoot, "scene_001", "cut_001")), "cells dir exists");

    ec.clear();
    std::filesystem::create_directories(perapera::framesDirectory(tempRoot, "scene_001", "cut_001", "cell_BG"), ec);
    ok &= expect(!ec, "create cell skeleton");
    ok &= expect(std::filesystem::is_directory(perapera::framesDirectory(tempRoot, "scene_001", "cut_001", "cell_BG")), "frames dir exists");

    std::filesystem::remove_all(tempRoot, ec);

    if (!ok) {
        return 1;
    }

    std::cout << "perapera_project_layout_paths_selftest passed\n";
    return 0;
}
